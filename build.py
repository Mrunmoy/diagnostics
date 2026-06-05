#!/usr/bin/env python3
"""Developer build wrapper for the diagnostics library."""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import textwrap
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DEFAULT_PRESET = "linux-debug"
DEFAULT_LIBRARY_PRESET = "linux-release"
DEFAULT_INSTALL_PREFIX = ROOT / "build" / "install" / "diag"
DOXYFILE = ROOT / "Doxyfile"
FEATURE_NAMES = [
    "DTC",
    "LIFECYCLE",
    "IDENTITY",
    "STORAGE",
    "TRANSPORT",
    "CAPSULE",
]
SIZE_SECTIONS = [
    ".text",
    ".rodata",
    ".data",
    ".bss",
]
LAYOUT_MACROS = [
    (
        "context storage",
        ROOT / "include" / "diag" / "context.h",
        "DIAG_CONTEXT_STORAGE_SIZE",
    ),
    (
        "context alignment",
        ROOT / "include" / "diag" / "context.h",
        "DIAG_CONTEXT_STORAGE_ALIGN",
    ),
    (
        "identity encoded",
        ROOT / "include" / "diag" / "identity.h",
        "DIAG_IDENTITY_ENCODED_SIZE",
    ),
    ("capsule header", ROOT / "include" / "diag" / "capsule.h", "DIAG_CAPSULE_HEADER_SIZE"),
    (
        "capsule section entry",
        ROOT / "include" / "diag" / "capsule.h",
        "DIAG_CAPSULE_SECTION_ENTRY_SIZE",
    ),
    (
        "capsule max sections",
        ROOT / "include" / "diag" / "capsule.h",
        "DIAG_CAPSULE_MAX_SECTIONS",
    ),
    (
        "DTC payload header",
        ROOT / "src" / "diag_storage.c",
        "DIAG_DTC_CAPSULE_PAYLOAD_HEADER_SIZE",
    ),
    ("DTC record", ROOT / "src" / "diag_storage.c", "DIAG_DTC_CAPSULE_RECORD_SIZE"),
    (
        "lifecycle payload",
        ROOT / "src" / "diag_storage.c",
        "DIAG_LIFECYCLE_CAPSULE_PAYLOAD_SIZE",
    ),
]

# Formatting must be reproducible across local, Docker, and CI, so clang-format
# is pinned to one major version. Override with CLANG_FORMAT=/path/to/clang-format
# only if you have verified it matches this version.
CLANG_FORMAT_VERSION = "14"


def run(command: list[str], *, env: dict[str, str] | None = None) -> None:
    print("+ " + " ".join(command), flush=True)
    merged_env = os.environ.copy()
    if env is not None:
        merged_env.update(env)
    subprocess.run(command, cwd=ROOT, check=True, env=merged_env)


def capture(command: list[str], *, env: dict[str, str] | None = None) -> str:
    print("+ " + " ".join(command), flush=True)
    merged_env = os.environ.copy()
    if env is not None:
        merged_env.update(env)
    completed = subprocess.run(
        command,
        cwd=ROOT,
        capture_output=True,
        check=True,
        env=merged_env,
        text=True,
    )
    return completed.stdout


def cmake_options(options: list[str]) -> list[str]:
    normalized: list[str] = []
    expect_define_value = False
    for option in options:
        if option == "--":
            continue
        if expect_define_value:
            # Fold a standalone "-D" with its value: "-D FOO=ON" -> "-DFOO=ON".
            normalized.append(f"-D{option}")
            expect_define_value = False
        elif option == "-D":
            expect_define_value = True
        elif option.startswith("-D"):
            normalized.append(option)
        else:
            normalized.append(f"-D{option}")
    if expect_define_value:
        # Trailing bare "-D" with no value: preserve it rather than drop silently.
        normalized.append("-D")
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
    release_preset = f"{family}-release"
    install_library_for_preset(
        release_preset,
        DEFAULT_INSTALL_PREFIX,
        args.cmake_options,
    )
    print_size_report(release_preset)
    package_test(DEFAULT_INSTALL_PREFIX)


def install_library(args: argparse.Namespace) -> None:
    install_library_for_preset(args.preset, args.prefix, args.cmake_options)


def size(args: argparse.Namespace) -> None:
    build_library_for_size_report(args.preset, args.cmake_options)
    print_size_report(args.preset)


def docs(args: argparse.Namespace) -> None:
    doxygen = resolve_doxygen()
    run([doxygen, str(args.config)])


def install_library_for_preset(preset: str, prefix: Path, extra_options: list[str]) -> None:
    # The installed package must always be a minimal library: no tests, no
    # examples. CMake applies the last -D for a given variable, so the forced
    # OFF options go last and win over any forwarded -- option.
    options = [
        f"CMAKE_INSTALL_PREFIX={prefix}",
        *extra_options,
        "DIAG_BUILD_TESTS=OFF",
        "DIAG_BUILD_EXAMPLES=OFF",
    ]

    configure(preset, options)
    run(["cmake", "--build", "--preset", preset])
    run(["cmake", "--install", str(build_dir_for_preset(preset))])


def build_library_for_size_report(preset: str, extra_options: list[str]) -> None:
    options = [
        *extra_options,
        "DIAG_BUILD_TESTS=OFF",
        "DIAG_BUILD_EXAMPLES=OFF",
    ]

    configure(preset, options)
    run(["cmake", "--build", "--preset", preset])


def print_size_report(preset: str) -> None:
    build_dir = build_dir_for_preset(preset)
    archive = build_dir / "libdiag.a"

    if not archive.exists():
        raise SystemExit(
            f"library archive was not found at {archive.relative_to(ROOT)}; build preset "
            f"'{preset}' first"
        )

    sections = archive_sections(archive)
    features = configured_features(build_dir)
    layout = diagnostic_layout()

    print("")
    print(f"Size report ({preset})")
    print(f"Library: {archive.relative_to(ROOT)}")
    print("")
    print("Sections (bytes)")
    for section in SIZE_SECTIONS:
        print(f"  {section:<8} {sections.get(section, 0):>8}")
    print(f"  {'total':<8} {sum(sections.values()):>8}")
    print("")
    print("Diagnostic layout (bytes)")
    for name, value in layout:
        unit = "count" if name == "capsule max sections" else "bytes"
        print(f"  {name:<24} {value:>8} {unit}")
    print("")
    print("Configured features")
    for name in FEATURE_NAMES:
        print(f"  {name:<10} {features.get(name, 'UNKNOWN')}")
    print("")
    print("Notes")
    print("  DTC RAM is caller-owned: capacity * sizeof(struct diag_dtc_snapshot).")
    print("  Capsule staging RAM is caller-owned and provided by the storage adapter.")


def archive_sections(archive: Path) -> dict[str, int]:
    size_tool = resolve_size_tool()
    output = capture([size_tool, "-A", str(archive)])
    sections = {section: 0 for section in SIZE_SECTIONS}

    for line in output.splitlines():
        columns = line.split()
        if len(columns) < 2:
            continue
        bucket = section_bucket(columns[0])
        if bucket:
            sections[bucket] += int(columns[1])

    return sections


def section_bucket(section: str) -> str | None:
    for name in SIZE_SECTIONS:
        if section == name or section.startswith(f"{name}."):
            return name
    return None


def configured_features(build_dir: Path) -> dict[str, str]:
    cache = build_dir / "CMakeCache.txt"
    if not cache.exists():
        raise SystemExit(f"CMake cache was not found at {cache.relative_to(ROOT)}")

    values: dict[str, str] = {}
    for line in cache.read_text(encoding="utf-8").splitlines():
        for name in FEATURE_NAMES:
            prefix = f"DIAG_FEATURE_{name}:BOOL="
            if line.startswith(prefix):
                values[name] = line.removeprefix(prefix)

    return values


def diagnostic_layout() -> list[tuple[str, int]]:
    return [(name, read_macro_u32(path, macro)) for name, path, macro in LAYOUT_MACROS]


def read_macro_u32(path: Path, name: str) -> int:
    pattern = re.compile(rf"^\s*#\s*define\s+{re.escape(name)}\s+\(?([0-9]+)u?\)?\s*$")

    for line in path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line)
        if match:
            return int(match.group(1))

    raise SystemExit(f"could not read numeric macro {name} from {path.relative_to(ROOT)}")


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


def resolve_clang_format() -> str:
    # Prefer an explicit override, then the version-suffixed binary, then a bare
    # clang-format. Whatever resolves must report the pinned major version.
    candidates = [
        os.environ.get("CLANG_FORMAT"),
        f"clang-format-{CLANG_FORMAT_VERSION}",
        "clang-format",
    ]
    for name in candidates:
        if not name:
            continue
        path = shutil.which(name) if "/" not in name else (name if Path(name).exists() else None)
        if not path:
            continue
        version = subprocess.run(
            [path, "--version"], capture_output=True, text=True, check=False
        ).stdout
        if f"version {CLANG_FORMAT_VERSION}." in version:
            return path

    raise SystemExit(
        f"clang-format {CLANG_FORMAT_VERSION} is required for reproducible formatting "
        f"but was not found. Install clang-format-{CLANG_FORMAT_VERSION}, or set "
        f"CLANG_FORMAT to a clang-format {CLANG_FORMAT_VERSION} binary."
    )


def resolve_doxygen() -> str:
    path = shutil.which("doxygen")
    if path:
        return path

    raise SystemExit(
        "doxygen is required to build API documentation. Install doxygen locally, "
        "or rebuild the devcontainer/Docker image from tools/docker/Dockerfile."
    )


def resolve_size_tool() -> str:
    path = shutil.which(os.environ.get("SIZE") or "size")
    if path:
        return path

    raise SystemExit(
        "GNU size is required to print the resource report. Install binutils, "
        "or set SIZE to a compatible size tool."
    )


def format_code(args: argparse.Namespace) -> None:
    clang_format = resolve_clang_format()

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

    size_parser = subcommands.add_parser(
        "size",
        help="Build the release library and print code/data/layout sizes",
    )
    add_common_build_args(size_parser, DEFAULT_LIBRARY_PRESET)
    size_parser.set_defaults(func=size)

    docs_parser = subcommands.add_parser("docs", help="Generate Doxygen API documentation")
    docs_parser.add_argument(
        "--config",
        type=Path,
        default=DOXYFILE,
        help=f"Doxygen configuration file. Default: {DOXYFILE}",
    )
    docs_parser.set_defaults(func=docs)

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
