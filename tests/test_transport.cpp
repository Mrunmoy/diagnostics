#include "diag/context.hpp"
#include "diag/transport.hpp"

#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

namespace
{

diag::Result nullSend(void *, const std::uint8_t *, std::size_t) noexcept
{
    return diag::Result::Ok;
}

diag::Result nullReceive(void *, std::uint8_t *, std::size_t, std::size_t &bytesRead) noexcept
{
    bytesRead = 0U;
    return diag::Result::Ok;
}

} // namespace

#if DIAG_FEATURE_TRANSPORT
TEST(DiagContextTransport, AttachesValidAdapter)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    diag::Transport      transport{diag::TransportOps{nullSend, nullReceive}, nullptr};

    ASSERT_TRUE(context.isInitialized());
    EXPECT_EQ(context.attachTransport(transport), diag::Result::Ok);
}

TEST(DiagContextTransport, RejectsIncompleteAdapter)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    diag::Transport      transport{};

    EXPECT_EQ(context.attachTransport(transport), diag::Result::InvalidArgument);

    transport.ops.send = nullSend;
    EXPECT_EQ(context.attachTransport(transport), diag::Result::InvalidArgument);
}
#else
TEST(DiagContextTransport, DisabledFeatureReportsNotSupported)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    diag::Transport      transport{diag::TransportOps{nullSend, nullReceive}, nullptr};

    EXPECT_EQ(context.attachTransport(transport), diag::Result::NotSupported);
}
#endif
