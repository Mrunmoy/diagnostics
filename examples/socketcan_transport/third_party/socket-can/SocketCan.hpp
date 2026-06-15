#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

// C runtime
#include <cerrno>
#include <cstring>

// POSIX / Linux
#include <fcntl.h>
#include <net/if.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// SocketCAN
#include <linux/can.h>
#include <linux/can/raw.h>

namespace SocketCAN
{

class CanError : public std::runtime_error
{
public:
    explicit CanError(const std::string& what)
        : std::runtime_error(what)
    {}
};

// Helper: convert errno to a descriptive exception.
inline void throwErrno(const std::string& where)
{
    throw CanError(where + ": " + std::strerror(errno));
}

// ---------------------------------------------------------------------------
// Classic CAN frame — up to 8 bytes of payload
// ---------------------------------------------------------------------------
struct Frame
{
    uint32_t id       = 0;              // 11-bit or 29-bit ID
    bool     extended = false;          // Standard vs extended frame
    bool     rtr      = false;          // Remote transmission request
    uint8_t  dlc      = 0;              // 0..8 bytes
    std::array<uint8_t, 8> data{};     // Classic CAN payload
};

// ---------------------------------------------------------------------------
// CAN FD frame — up to 64 bytes of payload
// ---------------------------------------------------------------------------
struct FdFrame
{
    uint32_t id       = 0;                          // 11-bit or 29-bit ID
    bool     extended = false;                      // Standard vs extended frame
    bool     brs      = false;                      // Bit-rate switch
    bool     esi      = false;                      // Error state indicator
    uint8_t  len      = 0;                          // 0..64 bytes
    std::array<uint8_t, CANFD_MAX_DLEN> data{};    // CAN FD payload (64 bytes)
};

// The FD payload contract: CAN FD carries up to 64 bytes.
static_assert(CANFD_MAX_DLEN == 64, "CAN FD payload must be 64 bytes");
static_assert(FdFrame{}.data.size() == CANFD_MAX_DLEN,
              "FdFrame payload must match CANFD_MAX_DLEN");

// ---------------------------------------------------------------------------
// Listener interfaces — implement to receive dispatched frames
// ---------------------------------------------------------------------------

class IClassicFrameListener
{
public:
    virtual ~IClassicFrameListener() = default;
    virtual void onClassicFrame(const Frame& frame) = 0;
};

class IFdFrameListener
{
public:
    virtual ~IFdFrameListener() = default;
    virtual void onFdFrame(const FdFrame& frame) = 0;
};

// ---------------------------------------------------------------------------
// BasicCanbus — a single CAN_RAW socket whose mode is fixed at compile time.
//
//   EnableFd == true   CAN_RAW_FD_FRAMES is enabled; the socket sends/receives
//                      both Classic and CAN FD frames. send(const FdFrame&) and
//                      addFdListener() are available.
//   EnableFd == false  classic-only socket (the kernel default). CAN FD frames
//                      are not delivered, and the FD-only members do not exist —
//                      calling them is a compile error.
//
// Use the CanFD / CanClassic aliases below rather than the template directly
// in most code.
// ---------------------------------------------------------------------------
template <bool EnableFd = true>
class BasicCanbus
{
public:
    // True if this socket type carries CAN FD frames (compile-time property).
    static constexpr bool kFdMode = EnableFd;

    // Maximum number of listeners of each type (zero-heap, fixed-size arrays).
    static constexpr std::size_t MaxListeners = 8;

    // Open and bind to a CAN interface (e.g. "vcan0", "can0").
    // loopback=false if you don't want to see your own frames.
    // When EnableFd is true but the interface/kernel cannot support CAN FD, the
    // socket falls back to classic-only and fdEnabled() reports false.
    explicit BasicCanbus(const std::string& ifname,
                         bool loopback = true,
                         bool blocking = true)
    {
        // Create socket: PF_CAN, raw CAN frames
        m_fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (m_fd < 0)
        {
            throwErrno("socket(PF_CAN, SOCK_RAW, CAN_RAW)");
        }

        // Enable CAN FD frames only for the FD socket type. CAN_RAW_FD_FRAMES is
        // off by default in the kernel; when unsupported by the interface/kernel,
        // fall back to classic-only.
        if constexpr (EnableFd)
        {
            int fdOn = 1;
            if (::setsockopt(m_fd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                             &fdOn, sizeof(fdOn)) == 0)
            {
                m_fdEnabled = true;
            }
            else
            {
                const int err = errno;
                if (err != ENOPROTOOPT && err != EOPNOTSUPP && err != EINVAL)
                {
                    closeIfOpen();
                    throwErrno("setsockopt(CAN_RAW_FD_FRAMES)");
                }
                // Otherwise: interface/kernel lacks CAN FD support — classic-only.
            }
        }

        // Optional: disable local loopback
        int loopbackVal = loopback ? 1 : 0;
        if (::setsockopt(m_fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK,
                         &loopbackVal, sizeof(loopbackVal)) < 0)
        {
            closeIfOpen();
            throwErrno("setsockopt(CAN_RAW_LOOPBACK)");
        }

        // Make non-blocking if requested
        if (!blocking)
        {
            int flags = ::fcntl(m_fd, F_GETFL, 0);
            if (flags < 0 || ::fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) < 0)
            {
                closeIfOpen();
                throwErrno("fcntl(O_NONBLOCK)");
            }
        }

        // Resolve interface index
        struct ifreq ifr;
        std::memset(&ifr, 0, sizeof(ifr));
        if (ifname.size() >= sizeof(ifr.ifr_name))
        {
            closeIfOpen();
            throw CanError("Interface name too long: " + ifname);
        }
        std::strncpy(ifr.ifr_name, ifname.c_str(), sizeof(ifr.ifr_name) - 1);

        if (::ioctl(m_fd, SIOCGIFINDEX, &ifr) < 0)
        {
            closeIfOpen();
            throwErrno("ioctl(SIOCGIFINDEX)");
        }

        // Bind to the interface
        struct sockaddr_can addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.can_family  = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (::bind(m_fd, reinterpret_cast<struct sockaddr*>(&addr),
                   sizeof(addr)) < 0)
        {
            closeIfOpen();
            throwErrno("bind(sockaddr_can)");
        }
    }

    ~BasicCanbus()
    {
        closeIfOpen();
    }

    BasicCanbus(const BasicCanbus&) = delete;
    BasicCanbus& operator=(const BasicCanbus&) = delete;

    BasicCanbus(BasicCanbus&& other) noexcept
    {
        m_fd               = other.m_fd;
        other.m_fd         = -1;
        m_fdEnabled        = other.m_fdEnabled;
        m_classicListeners = other.m_classicListeners;
        m_classicCount     = other.m_classicCount;
        m_fdListeners      = other.m_fdListeners;
        m_fdCount          = other.m_fdCount;
        other.m_fdEnabled    = false;
        other.m_classicCount = 0;
        other.m_fdCount      = 0;
    }

    BasicCanbus& operator=(BasicCanbus&& other) noexcept
    {
        if (this != &other)
        {
            closeIfOpen();
            m_fd               = other.m_fd;
            other.m_fd         = -1;
            m_fdEnabled        = other.m_fdEnabled;
            m_classicListeners = other.m_classicListeners;
            m_classicCount     = other.m_classicCount;
            m_fdListeners      = other.m_fdListeners;
            m_fdCount          = other.m_fdCount;
            other.m_fdEnabled    = false;
            other.m_classicCount = 0;
            other.m_fdCount      = 0;
        }
        return *this;
    }

    // -----------------------------------------------------------------------
    // Send
    // -----------------------------------------------------------------------

    // Send a Classic CAN frame (blocking). Available in both modes.
    void send(const Frame& frame)
    {
        if (m_fd < 0)
            throw CanError("send() on closed Canbus");

        // Validate DLC (CAN classic supports only 0..8)
        if (frame.dlc > 8)
            throw CanError("Invalid DLC (>8) for CAN classic frame");

        if (frame.rtr)
        {
            // RTR frames must NOT contain data bytes
            for (std::size_t i = 0; i < frame.dlc; ++i)
            {
                if (frame.data[i] != 0)
                    throw CanError("RTR frame cannot contain data bytes");
            }
        }

        // Validate CAN ID ranges
        if (!frame.extended)
        {
            if (frame.id > 0x7FFu)
                throw CanError("Standard CAN ID out of range (>0x7FF)");
        }
        else
        {
            if (frame.id > 0x1FFFFFFFu)
                throw CanError("Extended CAN ID out of range (>29-bit)");
        }

        struct can_frame cf;
        std::memset(&cf, 0, sizeof(cf));

        // Encode CAN ID + flags
        if (frame.extended)
            cf.can_id = (frame.id & CAN_EFF_MASK) | CAN_EFF_FLAG;
        else
            cf.can_id = (frame.id & CAN_SFF_MASK);

        if (frame.rtr)
            cf.can_id |= CAN_RTR_FLAG;

        cf.can_dlc = (frame.dlc <= 8) ? frame.dlc : 8;
        for (uint8_t i = 0; i < cf.can_dlc; ++i)
            cf.data[i] = frame.data[i];

        ssize_t n = ::write(m_fd, &cf, sizeof(cf));
        if (n < 0)
            throwErrno("write(can_frame)");
        if (n != static_cast<ssize_t>(sizeof(cf)))
            throw CanError("Partial CAN frame write");
    }

    // Send a CAN FD frame (blocking). Only exists on the FD socket type;
    // calling it on a CanClassic is a compile error.
    template <bool E = EnableFd, std::enable_if_t<E, int> = 0>
    void send(const FdFrame& frame)
    {
        if (m_fd < 0)
            throw CanError("send() on closed Canbus");

        if (!m_fdEnabled)
            throw CanError("CAN FD not enabled on this socket (interface/kernel lacks CAN FD support)");

        if (frame.len > CANFD_MAX_DLEN)
            throw CanError("CAN FD frame len exceeds CANFD_MAX_DLEN (64)");

        // Validate CAN ID ranges
        if (!frame.extended)
        {
            if (frame.id > 0x7FFu)
                throw CanError("Standard CAN ID out of range (>0x7FF)");
        }
        else
        {
            if (frame.id > 0x1FFFFFFFu)
                throw CanError("Extended CAN ID out of range (>29-bit)");
        }

        struct canfd_frame cf;
        std::memset(&cf, 0, sizeof(cf));

        if (frame.extended)
            cf.can_id = (frame.id & CAN_EFF_MASK) | CAN_EFF_FLAG;
        else
            cf.can_id = frame.id & CAN_SFF_MASK;

        cf.len   = frame.len;
        cf.flags = 0;
        if (frame.brs) cf.flags |= CANFD_BRS;
        if (frame.esi) cf.flags |= CANFD_ESI;

        std::memcpy(cf.data, frame.data.data(), frame.len);

        ssize_t n = ::write(m_fd, &cf, sizeof(cf));
        if (n < 0)
            throwErrno("write(canfd_frame)");
        if (n != static_cast<ssize_t>(sizeof(cf)))
            throw CanError("Partial CAN FD frame write");
    }

    // -----------------------------------------------------------------------
    // Receive — direct (backward-compatible)
    // -----------------------------------------------------------------------

    // Receive a Classic CAN frame directly into 'frame'.
    // Returns true if a frame was received before timeout, false on timeout.
    // On the FD socket type, throws CanError if a CAN FD frame arrives — use the
    // listener-based receive() overload for mixed-type traffic.
    bool receive(Frame& frame,
                 std::chrono::milliseconds timeout =
                     std::chrono::milliseconds{-1}) // -1 => block forever
    {
        if (m_fd < 0)
            throw CanError("receive() on closed Canbus");

        int timeoutMs = -1;
        if (timeout.count() >= 0)
            timeoutMs = static_cast<int>(timeout.count());

        if (!waitReadable(timeoutMs))
            return false;

        // Read into a canfd_frame-sized buffer; the kernel returns
        // sizeof(can_frame) for Classic frames and sizeof(canfd_frame) for FD.
        struct canfd_frame cf;
        std::memset(&cf, 0, sizeof(cf));

        ssize_t n = ::read(m_fd, &cf, sizeof(cf));
        if (n < 0)
            throwErrno("read()");

        if constexpr (EnableFd)
        {
            if (n == static_cast<ssize_t>(sizeof(struct canfd_frame)))
                throw CanError("CAN FD frame received; use listener-based receive() for mixed traffic");
        }

        if (n != static_cast<ssize_t>(sizeof(struct can_frame)))
            throw CanError("Partial CAN frame read");

        // can_frame and canfd_frame share the same layout for the first
        // sizeof(can_frame) bytes, so a reinterpret_cast is safe here.
        const auto* classic = reinterpret_cast<const struct can_frame*>(&cf);

        decodeClassic(*classic, frame);
        return true;
    }

    // -----------------------------------------------------------------------
    // Receive — listener-dispatching
    // -----------------------------------------------------------------------

    // Read one frame and dispatch it to all registered listeners of the
    // matching type. Returns true if a frame was read, false on timeout.
    bool receive(std::chrono::milliseconds timeout =
                     std::chrono::milliseconds{-1}) // -1 => block forever
    {
        if (m_fd < 0)
            throw CanError("receive() on closed Canbus");

        int timeoutMs = -1;
        if (timeout.count() >= 0)
            timeoutMs = static_cast<int>(timeout.count());

        if (!waitReadable(timeoutMs))
            return false;

        struct canfd_frame cf;
        std::memset(&cf, 0, sizeof(cf));

        ssize_t n = ::read(m_fd, &cf, sizeof(cf));
        if (n < 0)
            throwErrno("read()");

        if (n == static_cast<ssize_t>(sizeof(struct can_frame)))
        {
            // Classic CAN frame
            const auto* classic = reinterpret_cast<const struct can_frame*>(&cf);

            Frame frame;
            decodeClassic(*classic, frame);

            for (std::size_t i = 0; i < m_classicCount; ++i)
                m_classicListeners[i]->onClassicFrame(frame);
        }
        else
        {
            if constexpr (EnableFd)
            {
                if (n == static_cast<ssize_t>(sizeof(struct canfd_frame)))
                {
                    // CAN FD frame
                    FdFrame frame;
                    bool eff       = (cf.can_id & CAN_EFF_FLAG) != 0;
                    frame.extended = eff;
                    frame.id       = eff ? (cf.can_id & CAN_EFF_MASK)
                                         : (cf.can_id & CAN_SFF_MASK);
                    frame.len = cf.len;
                    frame.brs = (cf.flags & CANFD_BRS) != 0;
                    frame.esi = (cf.flags & CANFD_ESI) != 0;
                    for (uint8_t i = 0; i < cf.len && i < CANFD_MAX_DLEN; ++i)
                        frame.data[i] = cf.data[i];

                    for (std::size_t i = 0; i < m_fdCount; ++i)
                        m_fdListeners[i]->onFdFrame(frame);
                }
                else
                {
                    throw CanError("Unexpected read size from CAN socket");
                }
            }
            else
            {
                throw CanError("Unexpected read size from CAN socket");
            }
        }

        return true;
    }

    // -----------------------------------------------------------------------
    // Listener registration
    // -----------------------------------------------------------------------

    // Register a Classic CAN frame listener (up to MaxListeners). Throws if full.
    void addClassicListener(IClassicFrameListener* listener)
    {
        if (listener == nullptr)
            throw CanError("Classic CAN listener cannot be null");
        if (m_classicCount >= MaxListeners)
            throw CanError("Maximum number of Classic CAN listeners reached");
        m_classicListeners[m_classicCount++] = listener;
    }

    // Register a CAN FD frame listener (up to MaxListeners). Only exists on the
    // FD socket type; calling it on a CanClassic is a compile error.
    template <bool E = EnableFd, std::enable_if_t<E, int> = 0>
    void addFdListener(IFdFrameListener* listener)
    {
        if (listener == nullptr)
            throw CanError("CAN FD listener cannot be null");
        if (m_fdCount >= MaxListeners)
            throw CanError("Maximum number of CAN FD listeners reached");
        m_fdListeners[m_fdCount++] = listener;
    }

    // Get underlying file descriptor if you want to use your own poll/epoll.
    int fd() const { return m_fd; }

    // True if CAN FD mode is active on this socket (requested and supported).
    // Always false for the classic socket type.
    bool fdEnabled() const { return m_fdEnabled; }

private:
    int  m_fd        = -1;
    bool m_fdEnabled = false;

    std::array<IClassicFrameListener*, MaxListeners> m_classicListeners{};
    std::size_t m_classicCount = 0;

    std::array<IFdFrameListener*, MaxListeners> m_fdListeners{};
    std::size_t m_fdCount = 0;

    void closeIfOpen() noexcept
    {
        if (m_fd >= 0)
        {
            ::close(m_fd);
            m_fd = -1;
        }
    }

    // Wait until the socket has data to read. Returns false on timeout.
    bool waitReadable(int timeoutMs)
    {
        struct pollfd pfd;
        pfd.fd      = m_fd;
        pfd.events  = POLLIN;
        pfd.revents = 0;

        int ret = ::poll(&pfd, 1, timeoutMs);
        if (ret < 0)
            throwErrno("poll()");
        if (ret == 0)
            return false;
        if (!(pfd.revents & POLLIN))
            throw CanError("poll(): unexpected revents");
        return true;
    }

    // Decode a kernel can_frame into our Frame type.
    static void decodeClassic(const struct can_frame& src, Frame& frame)
    {
        frame.rtr      = (src.can_id & CAN_RTR_FLAG) != 0;
        bool eff       = (src.can_id & CAN_EFF_FLAG) != 0;
        frame.extended = eff;
        frame.id       = eff ? (src.can_id & CAN_EFF_MASK)
                             : (src.can_id & CAN_SFF_MASK);
        frame.dlc = src.can_dlc;
        for (uint8_t i = 0; i < frame.dlc && i < 8; ++i)
            frame.data[i] = src.data[i];
    }
};

// ---------------------------------------------------------------------------
// Convenience aliases — the mode is part of the type.
// ---------------------------------------------------------------------------
using CanClassic = BasicCanbus<false>;  // Classic-only
using CanFD      = BasicCanbus<true>;   // Classic + CAN FD

} // namespace SocketCAN
