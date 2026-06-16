#include "diag/diag.hpp"

#include <cstdio>

int main()
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};

    if (!context.isInitialized())
    {
        std::fprintf(stderr, "context initialization failed\n");
        return 1;
    }

    if (context.dirtyFlags() != diag::DirtyFlags{})
    {
        std::fprintf(stderr, "fresh context must be clean\n");
        return 2;
    }

    return 0;
}
