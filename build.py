#!/usr/bin/env python3
"""Developer build wrapper for the diagnostics library."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import textwrap
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DEFAULT_PRESET = "linux-debug"
DEFAULT_LIBRARY_PRESET = "linux-release"
DEFAULT_INSTALL_PREFIX = ROOT / "build" / "install" / "diag"


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


def configure(preset: str, options: list[str]) -> None:
    run(["cmake", "--preset", preset, *cmake_options(options)])


def build_preset(preset: str, options: list[str]) -> None:
    configure(preset, options)
    run(["cmake", "--build", "--preset", preset])


def test_preset(preset: str, options: list[str]) -> None:
    build_preset(preset, options)
    env = sanitizer_env() if preset.endswith("-asan") else None
    run(["ctest", "--preset", preset, "--output-on-failure"], env=env)


def build(args: argparse.Namespace) -> None:
    build_preset(args.preset, args.cmake_options)


def test(args: argparse.Namespace) -> None:
    test_preset(args.preset, args.cmake_options)


def all_checks(args: argparse.Namespace) -> None:
    family = preset_family(args.preset)
    format_code(argparse.Namespace(check=True))
    test_preset(f"{family}-debug", args.cmake_options)
    test_preset(f"{family}-asan", args.cmake_options)
    install_library_for_preset(
        f"{family}-release",
        DEFAULT_INSTALL_PREFIX,
        args.cmake_options,
    )
    package_test(DEFAULT_INSTALL_PREFIX)


def install_library(args: argparse.Namespace) -> None:
    install_library_for_preset(args.preset, args.prefix, args.cmake_options)


def install_library_for_preset(preset: str, prefix: Path, extra_options: list[str]) -> None:
    options = [
        "DIAG_BUILD_TESTS=OFF",
        "DIAG_BUILD_EXAMPLES=OFF",
        f"CMAKE_INSTALL_PREFIX={prefix}",
        *extra_options,
    ]

    configure(preset, options)
    run(["cmake", "--build", "--preset", preset])
    run(["cmake", "--install", str(build_dir_for_preset(preset))])


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
            project(diag_package_smoke C)

            find_package(diag CONFIG REQUIRED)

            add_executable(diag_package_smoke main.c)
            target_link_libraries(diag_package_smoke PRIVATE diag::diag)
            """
        ).lstrip(),
        encoding="utf-8",
    )
    (source_dir / "main.c").write_text(
        textwrap.dedent(
            """
            #include <diag/diag.h>

            int main(void)
            {
                return DIAG_OK == 0 ? 0 : 1;
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
            f"-DCMAKE_PREFIX_PATH={prefix}",
        ]
    )
    run(["cmake", "--build", str(build_dir)])


def clean(args: argparse.Namespace) -> None:
    paths = [ROOT / "build"] if args.all else [build_dir_for_preset(args.preset)]

    for path in paths:
        if path.exists():
            print(f"removing {path.relative_to(ROOT)}")
            shutil.rmtree(path)


def format_code(args: argparse.Namespace) -> None:
    clang_format = shutil.which("clang-format")
    if clang_format is None:
        raise SystemExit("clang-format was not found on PATH")

    files = source_files()
    if not files:
        return

    if args.check:
        run([clang_format, "--dry-run", "--Werror", *map(str, files)])
    else:
        run([clang_format, "-i", *map(str, files)])


def build_dir_for_preset(preset: str) -> Path:
    return ROOT / "build" / preset


def preset_family(preset: str) -> str:
    if "-" not in preset:
        raise SystemExit(f"preset '{preset}' must use a family suffix, for example linux-debug")
    return preset.split("-", 1)[0]


def sanitizer_env() -> dict[str, str]:
    return {
        "ASAN_OPTIONS": "detect_leaks=0:abort_on_error=1",
        "UBSAN_OPTIONS": "print_stacktrace=1:halt_on_error=1",
    }


def source_files() -> list[Path]:
    patterns = [
        "include/**/*.h",
        "include/**/*.hpp",
        "src/**/*.c",
        "src/**/*.cpp",
        "tests/**/*.c",
        "tests/**/*.cpp",
        "examples/**/*.c",
        "examples/**/*.cpp",
    ]
    files: list[Path] = []
    for pattern in patterns:
        files.extend(ROOT.glob(pattern))
    return sorted(files)


def add_common_build_args(parser: argparse.ArgumentParser, default_preset: str) -> None:
    parser.add_argument(
        "--preset",
        default=default_preset,
        help=f"CMake preset to use. Default: {default_preset}",
    )
    parser.add_argument(
        "cmake_options",
        nargs=argparse.REMAINDER,
        help="Extra CMake cache options after --, for example -- DIAG_BUILD_EXAMPLES=OFF.",
    )


def normalize_remainder(args: argparse.Namespace) -> None:
    if hasattr(args, "cmake_options") and args.cmake_options[:1] == ["--"]:
        args.cmake_options = args.cmake_options[1:]


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build, test, format, clean, and install the diagnostics library."
    )
    subcommands = parser.add_subparsers(dest="command", required=True)

    build_parser = subcommands.add_parser("build", help="Configure and build")
    add_common_build_args(build_parser, DEFAULT_PRESET)
    build_parser.set_defaults(func=build)

    test_parser = subcommands.add_parser("test", help="Configure, build, and run tests")
    add_common_build_args(test_parser, DEFAULT_PRESET)
    test_parser.set_defaults(func=test)

    all_parser = subcommands.add_parser("all", help="Run the full local quality gate")
    add_common_build_args(all_parser, DEFAULT_PRESET)
    all_parser.set_defaults(func=all_checks)

    library_parser = subcommands.add_parser(
        "library",
        help="Build and install the reusable CMake library package",
    )
    add_common_build_args(library_parser, DEFAULT_LIBRARY_PRESET)
    library_parser.add_argument(
        "--prefix",
        type=Path,
        default=DEFAULT_INSTALL_PREFIX,
        help=f"Install output directory. Default: {DEFAULT_INSTALL_PREFIX}",
    )
    library_parser.set_defaults(func=install_library)

    install_parser = subcommands.add_parser(
        "install",
        help="Alias for library",
    )
    add_common_build_args(install_parser, DEFAULT_LIBRARY_PRESET)
    install_parser.add_argument(
        "--prefix",
        type=Path,
        default=DEFAULT_INSTALL_PREFIX,
        help=f"Install output directory. Default: {DEFAULT_INSTALL_PREFIX}",
    )
    install_parser.set_defaults(func=install_library)

    clean_parser = subcommands.add_parser("clean", help="Remove build outputs")
    clean_parser.add_argument(
        "--preset",
        default=DEFAULT_PRESET,
        help=f"Build preset directory to remove. Default: {DEFAULT_PRESET}",
    )
    clean_parser.add_argument(
        "--all",
        action="store_true",
        help="Remove all build outputs",
    )
    clean_parser.set_defaults(func=clean)

    format_parser = subcommands.add_parser("format", help="Run clang-format")
    format_parser.add_argument(
        "--check",
        action="store_true",
        help="Check formatting without modifying files",
    )
    format_parser.set_defaults(func=format_code)

    package_parser = subcommands.add_parser(
        "package-test",
        help="Build a generated CMake consumer against the installed package",
    )
    package_parser.add_argument(
        "--prefix",
        type=Path,
        default=DEFAULT_INSTALL_PREFIX,
        help=f"Installed package prefix. Default: {DEFAULT_INSTALL_PREFIX}",
    )
    package_parser.set_defaults(func=lambda args: package_test(args.prefix))

    args = parser.parse_args(argv)
    normalize_remainder(args)
    return args


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
