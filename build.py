#!/usr/bin/env python3
"""Developer build wrapper for the diagnostics library."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DEFAULT_PRESET = "linux-debug"
DEFAULT_LIBRARY_PRESET = "linux-release"
DEFAULT_INSTALL_PREFIX = ROOT / "dist" / "diag"


def run(command: list[str]) -> None:
    print("+ " + " ".join(command), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def cmake_options(options: list[str]) -> list[str]:
    return [f"-D{option}" for option in options]


def configure(preset: str, options: list[str]) -> None:
    run(["cmake", "--preset", preset, *cmake_options(options)])


def build(args: argparse.Namespace) -> None:
    configure(args.preset, args.option)
    run(["cmake", "--build", "--preset", args.preset])


def test(args: argparse.Namespace) -> None:
    build(args)
    run(["ctest", "--preset", args.preset, "--output-on-failure"])


def install_library(args: argparse.Namespace) -> None:
    options = [
        "DIAG_BUILD_TESTS=OFF",
        "DIAG_BUILD_EXAMPLES=OFF",
        f"CMAKE_INSTALL_PREFIX={args.prefix}",
        *args.option,
    ]

    configure(args.preset, options)
    run(["cmake", "--build", "--preset", args.preset])
    run(["cmake", "--install", str(build_dir_for_preset(args.preset))])


def clean(args: argparse.Namespace) -> None:
    if args.all:
        paths = [ROOT / "build", ROOT / "dist"]
    else:
        paths = [build_dir_for_preset(args.preset)]

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


def source_files() -> list[Path]:
    # C sources (c branch) plus C++ sources/headers (cpp branch and GoogleTest
    # test files). The same .clang-format applies to all of them.
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
        "--option",
        action="append",
        default=[],
        metavar="KEY=VALUE",
        help="Extra CMake cache option. May be used more than once.",
    )


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
        help="Remove all build and distribution outputs",
    )
    clean_parser.set_defaults(func=clean)

    format_parser = subcommands.add_parser("format", help="Run clang-format")
    format_parser.add_argument(
        "--check",
        action="store_true",
        help="Check formatting without modifying files",
    )
    format_parser.set_defaults(func=format_code)

    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

