#!/bin/sh
set -eu

uid="${DIAG_DOCKER_UID:-1000}"
gid="${DIAG_DOCKER_GID:-1000}"
run_as_root="${DIAG_DOCKER_RUN_AS_ROOT:-0}"

group_id_exists()
{
    getent group | awk -F: -v gid="$1" '$3 == gid { found = 1 } END { exit found ? 0 : 1 }'
}

user_id_exists()
{
    getent passwd | awk -F: -v uid="$1" '$3 == uid { found = 1 } END { exit found ? 0 : 1 }'
}

if [ "$(id -u)" = "0" ] && [ "$run_as_root" != "1" ]; then
    if ! group_id_exists "$gid"; then
        group_name="diagnostics"
        if getent group "$group_name" >/dev/null 2>&1; then
            group_name="diagnostics-$gid"
        fi
        groupadd -g "$gid" "$group_name"
    fi

    if ! user_id_exists "$uid"; then
        user_name="diagnostics"
        if getent passwd "$user_name" >/dev/null 2>&1; then
            user_name="diagnostics-$uid"
        fi
        useradd -m -u "$uid" -g "$gid" "$user_name"
    fi

    exec gosu "$uid:$gid" "$@"
fi

exec "$@"
