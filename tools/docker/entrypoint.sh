#!/bin/sh
set -eu

uid="${DIAG_DOCKER_UID:-1000}"
gid="${DIAG_DOCKER_GID:-1000}"

if [ "$(id -u)" = "0" ]; then
    if ! getent group diagnostics >/dev/null 2>&1; then
        groupadd -g "$gid" diagnostics
    fi
    if ! getent passwd diagnostics >/dev/null 2>&1; then
        useradd -m -u "$uid" -g "$gid" diagnostics
    fi
    exec gosu diagnostics "$@"
fi

exec "$@"
