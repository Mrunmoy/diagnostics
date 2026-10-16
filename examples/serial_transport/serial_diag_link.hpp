#ifndef SERIAL_TRANSPORT_SERIAL_DIAG_LINK_HPP
#define SERIAL_TRANSPORT_SERIAL_DIAG_LINK_HPP

#include "example_diag_tool.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace diag_serial
{

class SerialPort final
{
  public:
    explicit SerialPort(const std::string &path);
    ~SerialPort();

    SerialPort(const SerialPort &) = delete;
    SerialPort &operator=(const SerialPort &) = delete;

    SerialPort(SerialPort &&other) noexcept;
    SerialPort &operator=(SerialPort &&other) noexcept;

    bool readByte(std::uint8_t &outByte, std::chrono::milliseconds timeout);
    void writeAll(const std::uint8_t *data, std::size_t size);

  private:
    int m_fd = -1;

    void closeIfOpen() noexcept;
};

bool receiveFrame(SerialPort &port, example_diag::Frame &outFrame,
                  std::chrono::milliseconds timeout);
void sendFrame(SerialPort &port, const example_diag::Frame &frame);

} // namespace diag_serial

#endif
