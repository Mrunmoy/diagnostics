FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        ninja-build \
        python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .

RUN ./build.py build -- \
        DIAG_FEATURE_DTC=ON \
        DIAG_FEATURE_IDENTITY=ON \
        DIAG_FEATURE_STORAGE=ON \
        DIAG_FEATURE_CAPSULE=ON

FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /workspace/build/linux-debug/examples/diag_grafana_reader_example /app/
COPY examples/grafana_reader/service/metrics_server.py /app/

EXPOSE 9108
CMD ["python3", "/app/metrics_server.py", "--binary", "/app/diag_grafana_reader_example", "--host", "0.0.0.0", "--port", "9108"]
