FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        clang-16 \
        clang-format-14 \
        cmake \
        doxygen \
        git \
        gosu \
        libclang-rt-16-dev \
        mawk \
        ninja-build \
        passwd \
        python3 \
    && rm -rf /var/lib/apt/lists/*

COPY tools/docker/entrypoint.sh /usr/local/bin/diagnostics-entrypoint
RUN chmod +x /usr/local/bin/diagnostics-entrypoint

WORKDIR /workspace

ENTRYPOINT ["/usr/local/bin/diagnostics-entrypoint"]
