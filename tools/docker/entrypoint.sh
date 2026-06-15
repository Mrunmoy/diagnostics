#!/bin/sh
set -eu

if [ "$(id -u)" = "0" ]; then
    if command -v modprobe >/dev/null 2>&1; then
        modprobe vcan 2>/dev/null || true
    fi

    if command -v ip >/dev/null 2>&1; then
        if ! ip link show vcan0 >/dev/null 2>&1; then
            ip link add dev vcan0 type vcan 2>/dev/null || true
        fi
        ip link set up vcan0 2>/dev/null || true
    fi

    if [ -n "${DIAG_DOCKER_GID:-}" ] && [ -n "${DIAG_DOCKER_UID:-}" ]; then
        if ! getent group diagdev >/dev/null 2>&1; then
            groupadd -g "${DIAG_DOCKER_GID}" diagdev 2>/dev/null || true
        fi
        if ! id diagdev >/dev/null 2>&1; then
            useradd -u "${DIAG_DOCKER_UID}" -g "${DIAG_DOCKER_GID}" -M -d /workspace diagdev \
                2>/dev/null || true
        fi
        if [ -d /workspace/build ]; then
            chown -R "${DIAG_DOCKER_UID}:${DIAG_DOCKER_GID}" /workspace/build 2>/dev/null || true
        fi
        exec gosu "${DIAG_DOCKER_UID}:${DIAG_DOCKER_GID}" "$@"
    fi
fi

exec "$@"
