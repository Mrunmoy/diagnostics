#!/usr/bin/env python3
"""Developer build wrapper for the diagnostics library."""

from __future__ import annotations

import argparse
import os
import re
import shlex
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
DEFAULT_SIZE_DTC_CAPACITY = 8
DEFAULT_SIZE_WRITE_ALIGNMENT = 4
DEFAULT_SIZE_SECTIONS = "dtc,lifecycle"
FEATURE_NAMES = [
    "DTC",
    "LIFECYCLE",
    "IDENTITY",
    "STORAGE",
    "TRANSPORT",
    "CAPSULE",
]
FEATURE_SOURCE_SYMBOLS = {
    "CAPSULE": ["diag_capsule_"],
    "DTC": ["diag_dtc_"],
    "IDENTITY": ["diag_identity_"],
    "LIFECYCLE": ["diag_lifecycle_"],
    "STORAGE": ["diag_storage_"],
    "TRANSPORT": ["diag_transport_"],
}
FEATURE_REQUIRED_SYMBOLS = {
    "CAPSULE": ["diag_capsule_crc32"],
    "DTC": ["diag_dtc_attach"],
    "IDENTITY": ["diag_identity_attach"],
    "LIFECYCLE": ["diag_lifecycle_attach"],
    "STORAGE": ["diag_storage_attach"],
    "TRANSPORT": ["diag_transport_attach"],
}
FEATURE_MATRIX_PROFILES = [
    {
        "name": "core-only",
        "features": {
            "DTC": False,
            "LIFECYCLE": False,
            "IDENTITY": False,
            "STORAGE": False,
            "TRANSPORT": False,
            "CAPSULE": False,
        },
        "dtc_capacity": 0,
        "sections": "",
    },
    {
        "name": "runtime-dtc",
        "features": {
            "DTC": True,
            "LIFECYCLE": False,
            "IDENTITY": False,
            "STORAGE": False,
            "TRANSPORT": False,
            "CAPSULE": False,
        },
        "dtc_capacity": 8,
        "sections": "",
    },
    {
        "name": "persistent-diagnostics",
        "features": {
            "DTC": True,
            "LIFECYCLE": True,
            "IDENTITY": False,
            "STORAGE": True,
            "TRANSPORT": False,
            "CAPSULE": True,
        },
        "dtc_capacity": 8,
        "sections": "dtc,lifecycle",
    },
    {
        "name": "full",
        "features": {
            "DTC": True,
            "LIFECYCLE": True,
            "IDENTITY": True,
            "STORAGE": True,
            "TRANSPORT": True,
            "CAPSULE": True,
        },
        "dtc_capacity": 8,
        "sections": "dtc,lifecycle",
    },
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
    print_size_report(release_preset, size_config_from_args(args))
    package_test(DEFAULT_INSTALL_PREFIX)
    feature_matrix(
        argparse.Namespace(
            preset=args.preset,
            cmake_options=args.cmake_options,
        )
    )


def install_library(args: argparse.Namespace) -> None:
    install_library_for_preset(args.preset, args.prefix, args.cmake_options)


def size(args: argparse.Namespace) -> None:
    build_library_for_size_report(args.preset, args.cmake_options)
    print_size_report(args.preset, size_config_from_args(args))


def feature_matrix(args: argparse.Namespace) -> None:
    family = preset_family(args.preset)

    print("")
    print("Feature matrix")
    for profile in FEATURE_MATRIX_PROFILES:
        name = str(profile["name"])
        profile_options = feature_profile_options(profile)
        debug_preset = f"{family}-debug"
        release_preset = f"{family}-release"
        options = [*args.cmake_options, *profile_options]

        print("")
        print(f"Profile: {name}")
        test_preset(debug_preset, options)
        build_library_for_size_report(release_preset, options)
        assert_feature_symbols_match_profile(release_preset, profile)
        print_size_report(release_preset, size_config_from_profile(profile))


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


def print_size_report(preset: str, config: dict[str, object]) -> None:
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
    abi_sizes, abi_warning = native_abi_sizes(build_dir, features)
    estimates = resource_estimates(config, features, layout, abi_sizes)

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
    print("Native ABI sizes (bytes)")
    if abi_sizes:
        for name, value in abi_sizes:
            print(f"  {name:<32} {value:>8}")
    else:
        print("  unavailable")
    print("")
    print("Configured resource estimate")
    for name, value, unit in estimates:
        print(f"  {name:<32} {str(value):>8} {unit}")
    print("")
    print("Configured features")
    for name in FEATURE_NAMES:
        print(f"  {name:<10} {features.get(name, 'UNKNOWN')}")
    print("")
    print("Notes")
    if abi_warning:
        print(f"  Native ABI probe unavailable: {abi_warning}")
    print("  DTC RAM is caller-owned: capacity * native sizeof(struct diag_dtc_snapshot).")
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


def archive_symbols(archive: Path) -> list[str]:
    nm_tool = resolve_nm_tool()
    output = capture([nm_tool, "-g", str(archive)])
    symbols: list[str] = []

    for line in output.splitlines():
        columns = line.split()
        if len(columns) >= 3:
            symbols.append(columns[-1])

    return symbols


def assert_feature_symbols_match_profile(preset: str, profile: dict[str, object]) -> None:
    archive = build_dir_for_preset(preset) / "libdiag.a"
    symbols = archive_symbols(archive)
    features = profile["features"]
    assert isinstance(features, dict)

    for feature in FEATURE_NAMES:
        if features.get(feature) is True:
            assert_enabled_feature_symbols_present(symbols, feature, profile)
            continue
        assert_disabled_feature_symbols_absent(archive, symbols, feature, profile)


def assert_enabled_feature_symbols_present(
    symbols: list[str], feature: str, profile: dict[str, object]
) -> None:
    missing = [symbol for symbol in FEATURE_REQUIRED_SYMBOLS[feature] if symbol not in symbols]
    if missing:
        joined = ", ".join(missing)
        raise SystemExit(
            f"profile '{profile['name']}' enabled {feature}, but libdiag.a does not export {joined}"
        )


def assert_disabled_feature_symbols_absent(
    archive: Path, symbols: list[str], feature: str, profile: dict[str, object]
) -> None:
    prefixes = FEATURE_SOURCE_SYMBOLS[feature]
    leaked = sorted(
        {symbol for symbol in symbols for prefix in prefixes if symbol.startswith(prefix)}
    )
    if leaked:
        joined = ", ".join(leaked[:5])
        raise SystemExit(
            f"profile '{profile['name']}' disabled {feature}, but {archive.relative_to(ROOT)} "
            f"still exports {joined}"
        )


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


def native_abi_sizes(build_dir: Path, features: dict[str, str]) -> tuple[list[tuple[str, int]], str | None]:
    probe = build_dir / "diag_size_probe.c"
    executable = build_dir / "diag_size_probe"
    compiler = cmake_cache_value(build_dir, "CMAKE_C_COMPILER") or "cc"
    c_flags = cmake_c_flags(build_dir)
    linker_flags = shlex.split(cmake_cache_value(build_dir, "CMAKE_EXE_LINKER_FLAGS") or "")
    feature_defines = [
        f"-DDIAG_FEATURE_{name}={1 if features.get(name) == 'ON' else 0}"
        for name in FEATURE_NAMES
        if features.get(name) in {"ON", "OFF"}
    ]

    probe.write_text(
        textwrap.dedent(
            """
            #include <stdio.h>

            #include "diag/context.h"
            #include "diag/dtc.h"
            #include "diag/lifecycle.h"
            #include "diag/storage.h"

            int main(void)
            {
                printf("context_storage %zu\\n", sizeof(struct diag_context_storage));
                printf("dtc_snapshot %zu\\n", sizeof(struct diag_dtc_snapshot));
                printf("dtc_config %zu\\n", sizeof(struct diag_dtc_config));
                printf("lifecycle_config %zu\\n", sizeof(struct diag_lifecycle_config));
                printf("lifecycle_snapshot %zu\\n", sizeof(struct diag_lifecycle_snapshot));
                printf("storage_adapter %zu\\n", sizeof(struct diag_storage));
                return 0;
            }
            """
        ).lstrip(),
        encoding="utf-8",
    )

    compile_command = [
        compiler,
        *c_flags,
        *feature_defines,
        "-std=c99",
        "-I",
        str(ROOT / "include"),
        str(probe),
        "-o",
        str(executable),
        *linker_flags,
    ]

    compile_result = run_probe_command(compile_command)
    if compile_result.returncode != 0:
        return [], summarize_probe_failure("compile failed", compile_result)

    probe_result = run_probe_command([str(executable)])
    if probe_result.returncode != 0:
        return [], summarize_probe_failure("execution failed", probe_result)

    sizes: list[tuple[str, int]] = []
    for line in probe_result.stdout.splitlines():
        name, value = line.split()
        sizes.append((name.replace("_", " "), int(value)))
    return sizes, None


def run_probe_command(command: list[str]) -> subprocess.CompletedProcess[str]:
    print("+ " + " ".join(shlex.quote(part) for part in command), flush=True)
    return subprocess.run(
        command,
        cwd=ROOT,
        capture_output=True,
        check=False,
        text=True,
    )


def summarize_probe_failure(reason: str, result: subprocess.CompletedProcess[str]) -> str:
    detail = (result.stderr or result.stdout).strip().splitlines()
    if detail:
        return f"{reason}: {detail[-1]}"
    return f"{reason} with exit code {result.returncode}"


def cmake_c_flags(build_dir: Path) -> list[str]:
    build_type = cmake_cache_value(build_dir, "CMAKE_BUILD_TYPE")
    flags = shlex.split(cmake_cache_value(build_dir, "CMAKE_C_FLAGS") or "")
    if build_type:
        flags.extend(
            shlex.split(cmake_cache_value(build_dir, f"CMAKE_C_FLAGS_{build_type.upper()}") or "")
        )
    return flags


def resource_estimates(
    config: dict[str, object],
    features: dict[str, str],
    layout: list[tuple[str, int]],
    abi_sizes: list[tuple[str, int]],
) -> list[tuple[str, object, str]]:
    layout_values = dict(layout)
    abi_values = dict(abi_sizes)
    dtc_capacity = int(config["dtc_capacity"])
    write_alignment = int(config["write_alignment"])
    requested_sections = set(config["sections"])
    has_dtc_section = features.get("DTC") == "ON" and "dtc" in requested_sections
    has_lifecycle_section = features.get("LIFECYCLE") == "ON" and "lifecycle" in requested_sections
    section_count = int(has_dtc_section) + int(has_lifecycle_section)
    dtc_payload = (
        layout_values["DTC payload header"] + (dtc_capacity * layout_values["DTC record"])
        if has_dtc_section
        else 0
    )
    lifecycle_payload = layout_values["lifecycle payload"] if has_lifecycle_section else 0
    dtc_snapshot_size = abi_values.get("dtc snapshot")
    context_storage_size = abi_values.get("context storage")
    dtc_runtime_buffer: object = 0
    if features.get("DTC") == "ON":
        dtc_runtime_buffer = (
            dtc_capacity * dtc_snapshot_size if dtc_snapshot_size is not None else "unavailable"
        )
    capsule_minimum = layout_values["capsule header"]
    capsule_minimum += section_count * layout_values["capsule section entry"]
    capsule_minimum += align_up(dtc_payload, write_alignment)
    capsule_minimum += align_up(lifecycle_payload, write_alignment)
    estimates: list[tuple[str, object, str]] = [
        ("DTC capacity", dtc_capacity, "records"),
        ("write alignment", write_alignment, "bytes"),
        ("persisted sections", section_count, "sections"),
        ("minimum capsule staging", capsule_minimum, "bytes"),
        ("DTC runtime buffer", dtc_runtime_buffer, "bytes"),
        (
            "context storage",
            context_storage_size if context_storage_size is not None else "unavailable",
            "bytes",
        ),
    ]
    estimated_caller_ram: object = "unavailable"
    if context_storage_size is not None and isinstance(dtc_runtime_buffer, int):
        estimated_caller_ram = context_storage_size + dtc_runtime_buffer + capsule_minimum
    estimates.append(("estimated caller RAM", estimated_caller_ram, "bytes"))
    return estimates


def align_up(value: int, alignment: int) -> int:
    if alignment <= 1:
        return value
    return ((value + alignment - 1) // alignment) * alignment


def add_size_estimate_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--dtc-capacity",
        type=int,
        default=DEFAULT_SIZE_DTC_CAPACITY,
        help=f"DTC records to use for RAM/capsule estimates. Default: {DEFAULT_SIZE_DTC_CAPACITY}",
    )
    parser.add_argument(
        "--write-alignment",
        type=int,
        default=DEFAULT_SIZE_WRITE_ALIGNMENT,
        help=(
            "Storage write alignment used for capsule estimates. "
            f"Default: {DEFAULT_SIZE_WRITE_ALIGNMENT}"
        ),
    )
    parser.add_argument(
        "--sections",
        default=DEFAULT_SIZE_SECTIONS,
        help=(
            "Comma-separated persisted sections to estimate: dtc,lifecycle. "
            f"Default: {DEFAULT_SIZE_SECTIONS}"
        ),
    )


def feature_profile_options(profile: dict[str, object]) -> list[str]:
    features = profile["features"]
    assert isinstance(features, dict)

    options: list[str] = []
    for feature in FEATURE_NAMES:
        value = "ON" if features[feature] else "OFF"
        options.append(f"DIAG_FEATURE_{feature}={value}")
    return options


def size_config_from_profile(profile: dict[str, object]) -> dict[str, object]:
    return {
        "dtc_capacity": int(profile["dtc_capacity"]),
        "write_alignment": DEFAULT_SIZE_WRITE_ALIGNMENT,
        "sections": parse_size_sections(str(profile["sections"])),
    }


def size_config_from_args(args: argparse.Namespace) -> dict[str, object]:
    if args.dtc_capacity < 0:
        raise SystemExit("--dtc-capacity must be >= 0")
    if args.write_alignment < 1:
        raise SystemExit("--write-alignment must be >= 1")

    return {
        "dtc_capacity": args.dtc_capacity,
        "write_alignment": args.write_alignment,
        "sections": parse_size_sections(args.sections),
    }


def parse_size_sections(value: str) -> list[str]:
    sections = [section.strip().lower() for section in value.split(",") if section.strip()]
    valid = {"dtc", "lifecycle"}
    invalid = sorted(set(sections) - valid)
    if invalid:
        raise SystemExit(f"unsupported size report section(s): {', '.join(invalid)}")
    return sections


def cmake_cache_value(build_dir: Path, key: str) -> str | None:
    cache = build_dir / "CMakeCache.txt"
    if not cache.exists():
        return None

    for line in cache.read_text(encoding="utf-8").splitlines():
        if line.startswith(f"{key}:"):
            return line.split("=", 1)[1]
    return None


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


def resolve_nm_tool() -> str:
    path = shutil.which(os.environ.get("NM") or "nm")
    if path:
        return path

    raise SystemExit(
        "GNU nm is required to validate feature resource contracts. Install binutils, "
        "or set NM to a compatible nm tool."
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
    add_size_estimate_args(all_parser)
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
    add_size_estimate_args(size_parser)
    add_common_build_args(size_parser, DEFAULT_LIBRARY_PRESET)
    size_parser.set_defaults(func=size)

    feature_matrix_parser = subcommands.add_parser(
        "feature-matrix",
        help="Build feature profiles and validate resource contracts",
    )
    add_common_build_args(feature_matrix_parser, DEFAULT_PRESET)
    feature_matrix_parser.set_defaults(func=feature_matrix)

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
