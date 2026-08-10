#!/usr/bin/env python3
import argparse
import subprocess
import sys


def parse_errors(input_file: str) -> list[tuple[str, str]]:
    entries = []
    with open(input_file) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            name, _, value = line.partition("=")
            entries.append((name.strip(), value.strip()))
    return entries


def generate_cpp(entries: list[tuple[str, str]]) -> str:
    lines = [
        "#pragma once",
        "#include \"core/Core.hh\"",
        "namespace jstine {",
        "",
        "enum class ErrorCode : u32 {",
    ]
    for name, value in entries:
        lines.append(f"    {name} = {value},")
    lines += [
        "};",
        "",
        "}  // namespace jstine",
        "",
    ]
    return "\n".join(lines)


def generate_python(entries: list[tuple[str, str]]) -> str:
    lines = [
        "# This file is generated. Do not edit manually.",
        "# Source: conf/errors.in",
        "",
        "from enum import IntEnum",
        "",
        "",
        "class ErrorCode(IntEnum):",
    ]
    for name, value in entries:
        lines.append(f"    {name} = {value}")
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate ErrorCode enum from errors.in")
    parser.add_argument("input", help="Path to errors.in file")
    parser.add_argument("output", help="Path to output file")
    parser.add_argument(
        "--lang", choices=["cpp", "python"], default="cpp",
        help="Output language (default: cpp)")
    args = parser.parse_args()

    entries = parse_errors(args.input)

    if args.lang == "python":
        content = generate_python(entries)
        with open(args.output, "w") as f:
            f.write(content)
    else:
        content = generate_cpp(entries)
        with open(args.output, "w") as f:
            f.write(content)
        result = subprocess.run(["clang-format", "-i", args.output])
        if result.returncode != 0:
            print("clang-format failed", file=sys.stderr)
            sys.exit(result.returncode)


if __name__ == "__main__":
    main()
