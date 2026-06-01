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
        valgrind \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
