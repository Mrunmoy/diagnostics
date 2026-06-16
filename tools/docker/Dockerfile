FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        clang-16 \
        clang-format-14 \
        clang-tidy \
        cmake \
        doxygen \
        gdb \
        git \
        gosu \
        iproute2 \
        kmod \
        libclang-rt-16-dev \
        ninja-build \
        pkg-config \
        python3 \
        valgrind \
    && rm -rf /var/lib/apt/lists/*

COPY tools/docker/entrypoint.sh /usr/local/bin/diagnostics-entrypoint
RUN chmod +x /usr/local/bin/diagnostics-entrypoint

WORKDIR /workspace

ENTRYPOINT ["/usr/local/bin/diagnostics-entrypoint"]
