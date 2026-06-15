#include "serial_diag_link.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace diag_serial
{
namespace
{

constexpr std::uint8_t kSync0 = 0xA5u;
constexpr std::uint8_t kSync1 = 0x5Au;
constexpr std::size_t  kHeaderSize = 4u;
constexpr std::size_t  kChecksumSize = 1u;

std::runtime_error makeErrnoError(const char *operation)
{
    return std::runtime_error(std::string(operation) + ": " + std::strerror(errno));
}

std::uint8_t checksum(const std::uint8_t *data, std::size_t size)
{
    std::uint8_t value = 0u;

    for (std::size_t i = 0u; i < size; ++i)
    {
        value = static_cast<std::uint8_t>(value + data[i]);
    }

    return value;
}

void configureRawSerial(int fd)
{
    struct termios options;

    if (::tcgetattr(fd, &options) != 0)
    {
        throw makeErrnoError("tcgetattr");
    }

    ::cfmakeraw(&options);
    options.c_cflag |= static_cast<tcflag_t>(CLOCAL | CREAD);
#ifdef CRTSCTS
    options.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
#endif
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;

    if (::cfsetispeed(&options, B115200) != 0 || ::cfsetospeed(&options, B115200) != 0)
    {
        throw makeErrnoError("cfsetspeed");
    }

    if (::tcsetattr(fd, TCSANOW, &options) != 0)
    {
        throw makeErrnoError("tcsetattr");
    }
}

} // namespace

SerialPort::SerialPort(const std::string &path)
{
    m_fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0)
    {
        throw makeErrnoError("open");
    }

    try
    {
        configureRawSerial(m_fd);
    }
    catch (...)
    {
        closeIfOpen();
        throw;
    }
}

SerialPort::~SerialPort()
{
    closeIfOpen();
}

SerialPort::SerialPort(SerialPort &&other) noexcept : m_fd(other.m_fd)
{
    other.m_fd = -1;
}

SerialPort &SerialPort::operator=(SerialPort &&other) noexcept
{
    if (this != &other)
    {
        closeIfOpen();
        m_fd = other.m_fd;
        other.m_fd = -1;
    }

    return *this;
}

bool SerialPort::readByte(std::uint8_t &outByte, std::chrono::milliseconds timeout)
{
    struct pollfd pfd;

    if (m_fd < 0)
    {
        throw std::runtime_error("readByte on closed serial port");
    }

    pfd.fd = m_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    const int ret = ::poll(&pfd, 1, static_cast<int>(timeout.count()));
    if (ret < 0)
    {
        throw makeErrnoError("poll");
    }
    if (ret == 0)
    {
        return false;
    }

    const ssize_t bytesRead = ::read(m_fd, &outByte, 1u);
    if (bytesRead < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return false;
        }
        throw makeErrnoError("read");
    }

    return bytesRead == 1;
}

void SerialPort::writeAll(const std::uint8_t *data, std::size_t size)
{
    std::size_t written = 0u;

    if (m_fd < 0)
    {
        throw std::runtime_error("writeAll on closed serial port");
    }

    while (written < size)
    {
        const ssize_t ret = ::write(m_fd, &data[written], size - written);
        if (ret < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            throw makeErrnoError("write");
        }

        written += static_cast<std::size_t>(ret);
    }
}

void SerialPort::closeIfOpen() noexcept
{
    if (m_fd >= 0)
    {
        (void)::close(m_fd);
        m_fd = -1;
    }
}

bool receiveFrame(SerialPort &port, struct example_diag_frame &outFrame,
                  std::chrono::milliseconds timeout)
{
    const auto   startedAt = std::chrono::steady_clock::now();
    std::uint8_t byte = 0u;
    std::array<std::uint8_t, EXAMPLE_DIAG_MAX_FRAME_SIZE> payload = {0};
    std::size_t                                           length = 0u;

    while (std::chrono::steady_clock::now() - startedAt < timeout)
    {
        if (!port.readByte(byte, std::chrono::milliseconds{25}))
        {
            continue;
        }
        if (byte != kSync0)
        {
            continue;
        }
        if (!port.readByte(byte, std::chrono::milliseconds{25}) || byte != kSync1)
        {
            continue;
        }
        if (!port.readByte(byte, std::chrono::milliseconds{25}))
        {
            continue;
        }
        length = byte;
        if (!port.readByte(byte, std::chrono::milliseconds{25}))
        {
            continue;
        }
        length |= static_cast<std::size_t>(byte) << 8u;
        if (length > payload.size())
        {
            continue;
        }

        for (std::size_t i = 0u; i < length; ++i)
        {
            if (!port.readByte(payload[i], std::chrono::milliseconds{25}))
            {
                return false;
            }
        }

        if (!port.readByte(byte, std::chrono::milliseconds{25}))
        {
            return false;
        }
        if (byte != checksum(payload.data(), length))
        {
            continue;
        }

        std::memcpy(outFrame.bytes, payload.data(), length);
        outFrame.size = length;
        return true;
    }

    return false;
}

void sendFrame(SerialPort &port, const struct example_diag_frame &frame)
{
    std::array<std::uint8_t, kHeaderSize + EXAMPLE_DIAG_MAX_FRAME_SIZE + kChecksumSize> wire = {0};
    const std::size_t wireSize = kHeaderSize + frame.size + kChecksumSize;

    if (frame.size > EXAMPLE_DIAG_MAX_FRAME_SIZE)
    {
        throw std::runtime_error("serial diagnostic frame is too large");
    }

    wire[0] = kSync0;
    wire[1] = kSync1;
    wire[2] = static_cast<std::uint8_t>(frame.size & 0xFFu);
    wire[3] = static_cast<std::uint8_t>((frame.size >> 8u) & 0xFFu);
    std::memcpy(&wire[kHeaderSize], frame.bytes, frame.size);
    wire[kHeaderSize + frame.size] = checksum(frame.bytes, frame.size);

    port.writeAll(wire.data(), wireSize);
}

} // namespace diag_serial
