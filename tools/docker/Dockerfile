FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        clang-format \
        clang-tidy \
        cmake \
        gdb \
        git \
        ninja-build \
        pkg-config \
        python3 \
        valgrind \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
