# Build And CI

Use `build.py` for local, container, and CI workflows. Generated artifacts must
stay under `build/`.

## Local Gate

```sh
./build.py all
```

The `all` command runs:

- `clang-format --dry-run --Werror`
- debug build and CTest
- ASAN/UBSAN build and CTest
- release library install to `build/install/diag`
- generated CMake package-consumption smoke test under `build/package-test`

Run an individual sanitizer test with:

```sh
./build.py test --preset linux-asan
```

## CMake Options

Pass CMake cache options after `--`:

```sh
./build.py build -- DIAG_BUILD_EXAMPLES=OFF
./build.py all -- DIAG_BUILD_EXAMPLES=OFF
```

Each option is forwarded to CMake as `-DKEY=VALUE`.

## Docker

The Docker image contains the host build tools needed by `build.py`, including
CMake, Ninja, clang-format, Python, and C/C++ compilers.

```sh
docker compose run --rm diagnostics-dev
```

This runs:

```sh
./build.py all --preset container-debug
```

The compose service defaults to UID/GID `1000:1000` so generated files remain
owned by a normal host user. Override this when needed:

```sh
DIAG_DOCKER_UID="$(id -u)" DIAG_DOCKER_GID="$(id -g)" \
    docker compose run --rm diagnostics-dev
```

## CI

GitHub Actions runs on pull requests and pushes targeting `c` and `cpp`.

Jobs:

- `Host ./build.py all`
- `Docker ./build.py all`

Repository branch rules should require pull requests for `main`, `c`, and `cpp`
and enable automatic Copilot code review for PRs targeting `c` and `cpp`.
