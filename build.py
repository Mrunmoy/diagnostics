#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
import textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DEFAULT_PRESET = "linux-debug"
DEFAULT_INSTALL_PREFIX = ROOT / "build" / "install" / "diag"
CLANG_FORMAT_VERSION = "14"


def run(command: list[str], *, env: dict[str, str] | None = None) -> None:
    print("+ " + " ".join(command), flush=True)
    merged_env = os.environ.copy()
    if env is not None:
        merged_env.update(env)
    subprocess.run(command, cwd=ROOT, check=True, env=merged_env)


def cmake_options(options: list[str]) -> list[str]:
    normalized: list[str] = []
    for option in options:
        if option == "--":
            continue
        if option.startswith("-D"):
            normalized.append(option)
        else:
            normalized.append(f"-D{option}")
    return normalized


def sanitizer_env() -> dict[str, str]:
    return {
        "ASAN_OPTIONS": "detect_leaks=0:abort_on_error=1",
        "UBSAN_OPTIONS": "print_stacktrace=1:halt_on_error=1",
    }


def build_dir_for_preset(preset: str) -> Path:
    return ROOT / "build" / preset


def preset_family(preset: str) -> str:
    if "-" not in preset:
        raise SystemExit(f"preset '{preset}' must use a family suffix")
    return preset.split("-", 1)[0]


def configure(preset: str, options: list[str]) -> None:
    run(["cmake", "--preset", preset, *cmake_options(options)])


def build_preset(preset: str, options: list[str]) -> None:
    configure(preset, options)
    env = sanitizer_env() if preset.endswith("-asan") else None
    run(["cmake", "--build", "--preset", preset], env=env)


def test_preset(preset: str, options: list[str]) -> None:
    build_preset(preset, options)
    if preset.endswith("-asan"):
        run([str(build_dir_for_preset(preset) / "tests" / "diag_tests")], env=sanitizer_env())
    else:
        run(["ctest", "--preset", preset, "--output-on-failure"])


def build(args: argparse.Namespace) -> None:
    build_preset(args.preset, args.cmake_options)


def test(args: argparse.Namespace) -> None:
    test_preset(args.preset, args.cmake_options)


def all_checks(args: argparse.Namespace) -> None:
    family = preset_family(args.preset)
    format_code(argparse.Namespace(check=True))
    test_preset(f"{family}-debug", args.cmake_options)
    test_preset(f"{family}-asan", args.cmake_options)
    install_library_for_preset(f"{family}-release", DEFAULT_INSTALL_PREFIX, args.cmake_options)
    package_test(DEFAULT_INSTALL_PREFIX)


def install_library_for_preset(preset: str, prefix: Path, extra_options: list[str]) -> None:
    options = [
        f"CMAKE_INSTALL_PREFIX={prefix}",
        *extra_options,
        "DIAG_BUILD_TESTS=OFF",
        "DIAG_BUILD_EXAMPLES=OFF",
    ]
    configure(preset, options)
    run(["cmake", "--build", "--preset", preset])
    run(["cmake", "--install", str(build_dir_for_preset(preset))])


def library(args: argparse.Namespace) -> None:
    install_library_for_preset(args.preset, args.prefix, args.cmake_options)


def package_test(prefix: Path) -> None:
    smoke_dir = ROOT / "build" / "package-test"
    source_dir = smoke_dir / "src"
    build_dir = smoke_dir / "build"

    if smoke_dir.exists():
        shutil.rmtree(smoke_dir)

    source_dir.mkdir(parents=True)
    (source_dir / "CMakeLists.txt").write_text(
        textwrap.dedent(
            """
            cmake_minimum_required(VERSION 3.22)
            project(diag_package_smoke CXX)

            find_package(diag CONFIG REQUIRED)

            get_target_property(diag_includes diag::diag INTERFACE_INCLUDE_DIRECTORIES)
            if(NOT diag_includes)
                message(FATAL_ERROR "diag::diag does not export include directories")
            endif()

            add_executable(diag_package_smoke main.cpp)
            target_link_libraries(diag_package_smoke PRIVATE diag::diag)
            """
        ).lstrip(),
        encoding="utf-8",
    )
    (source_dir / "main.cpp").write_text(
        textwrap.dedent(
            """
            #include <diag/diag.hpp>

            int main()
            {
                diag::ContextStorage storage{};
                diag::Context context{storage};
                return context.isInitialized() ? 0 : 1;
            }
            """
        ).lstrip(),
        encoding="utf-8",
    )

    run(
        [
            "cmake",
            "-S",
            str(source_dir),
            "-B",
            str(build_dir),
            "-G",
            "Ninja",
            "-DCMAKE_CXX_COMPILER=clang++-16",
            f"-DCMAKE_PREFIX_PATH={prefix}",
        ]
    )
    run(["cmake", "--build", str(build_dir)])
    run([str(build_dir / "diag_package_smoke")])


def clean(args: argparse.Namespace) -> None:
    paths = [ROOT / "build"] if args.all else [build_dir_for_preset(args.preset)]
    for path in paths:
        if path.exists():
            print(f"removing {path.relative_to(ROOT)}")
            shutil.rmtree(path)


def source_files() -> list[Path]:
    patterns = [
        "include/**/*.hpp",
        "src/**/*.cpp",
        "tests/**/*.cpp",
        "examples/**/*.hpp",
        "examples/**/*.cpp",
    ]
    files: list[Path] = []
    for pattern in patterns:
        files.extend(ROOT.glob(pattern))
    return sorted(path for path in files if "build/" not in path.as_posix())


def resolve_clang_format() -> str:
    candidates = [
        os.environ.get("CLANG_FORMAT"),
        f"clang-format-{CLANG_FORMAT_VERSION}",
        "clang-format",
    ]
    for candidate in candidates:
        if not candidate:
            continue
        resolved = shutil.which(candidate)
        if resolved:
            version = subprocess.run(
                [resolved, "--version"], capture_output=True, text=True, check=False
            )
            if f"version {CLANG_FORMAT_VERSION}." in version.stdout:
                return resolved
    raise SystemExit(f"clang-format-{CLANG_FORMAT_VERSION} is required")


def format_code(args: argparse.Namespace) -> None:
    files = source_files()
    if not files:
        return
    clang_format = resolve_clang_format()
    if args.check:
        run([clang_format, "--dry-run", "--Werror", *map(str, files)])
    else:
        run([clang_format, "-i", *map(str, files)])


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build helper for generic diagnostics C++")
    subcommands = parser.add_subparsers(dest="command", required=True)

    def add_preset(parser_: argparse.ArgumentParser, default: str = DEFAULT_PRESET) -> None:
        parser_.add_argument("--preset", default=default)
        parser_.add_argument("cmake_options", nargs=argparse.REMAINDER)

    build_parser = subcommands.add_parser("build")
    add_preset(build_parser)
    build_parser.set_defaults(func=build)

    test_parser = subcommands.add_parser("test")
    add_preset(test_parser)
    test_parser.set_defaults(func=test)

    all_parser = subcommands.add_parser("all")
    add_preset(all_parser)
    all_parser.set_defaults(func=all_checks)

    library_parser = subcommands.add_parser("library")
    library_parser.add_argument("--preset", default="linux-release")
    library_parser.add_argument("--prefix", type=Path, default=DEFAULT_INSTALL_PREFIX)
    library_parser.add_argument("cmake_options", nargs=argparse.REMAINDER)
    library_parser.set_defaults(func=library)

    package_parser = subcommands.add_parser("package-test")
    package_parser.add_argument("--prefix", type=Path, default=DEFAULT_INSTALL_PREFIX)
    package_parser.set_defaults(func=lambda args: package_test(args.prefix))

    format_parser = subcommands.add_parser("format")
    format_parser.add_argument("--check", action="store_true")
    format_parser.set_defaults(func=format_code)

    clean_parser = subcommands.add_parser("clean")
    clean_parser.add_argument("--preset", default=DEFAULT_PRESET)
    clean_parser.add_argument("--all", action="store_true")
    clean_parser.set_defaults(func=clean)

    args = parser.parse_args(argv)
    if hasattr(args, "cmake_options") and args.cmake_options[:1] == ["--"]:
        args.cmake_options = args.cmake_options[1:]
    return args


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
