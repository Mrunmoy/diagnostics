#!/bin/sh
set -eu

uid="${DIAG_DOCKER_UID:-1000}"
gid="${DIAG_DOCKER_GID:-1000}"
run_as_root="${DIAG_DOCKER_RUN_AS_ROOT:-0}"

if [ "$(id -u)" = "0" ] && [ "$run_as_root" != "1" ]; then
    if ! getent group "$gid" >/dev/null 2>&1; then
        group_name="diagnostics"
        if getent group "$group_name" >/dev/null 2>&1; then
            group_name="diagnostics-$gid"
        fi
        groupadd -g "$gid" "$group_name"
    fi

    if ! getent passwd "$uid" >/dev/null 2>&1; then
        user_name="diagnostics"
        if getent passwd "$user_name" >/dev/null 2>&1; then
            user_name="diagnostics-$uid"
        fi
        useradd -m -u "$uid" -g "$gid" "$user_name"
    fi

    exec gosu "$uid:$gid" "$@"
fi

exec "$@"
