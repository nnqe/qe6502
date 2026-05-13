#!/usr/bin/env python3
"""
Generate a self-contained C header with a 256-entry 6502 opcode lookup table.

Input JSON format:

{
    "opcodes": [
        {
            "opcode": "0x69",
            "name": "ADC",
            "bytes": "2",
            "description": "Add with Carry",
            "mode": "Immediate"
        },
        {
            "opcode": "0xEF",
            "name": "BBS6",
            "bytes": "3",
            "description": "Branch on Bit Set",
            "mode": "ZeroPage Relative",
            "model": "cmos"
        }
    ]
}

Rules:
- opcode is converted to uint8_t.
- bytes is converted to uint8_t.
- model == "cmos" means is_cmos_extension = 1.
- missing model means is_cmos_extension = 0.
- addr modes are collected from the full JSON and converted to enum values.
- missing opcode slots are generated as illegal opcodes.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


HEADER_NAME = "QE6502_OPCODES_H"
ADDR_MODE_TYPE = "qe6502_addr_mode_t"
OPCODE_INFO_TYPE = "qe6502_opcode_info_t"
OPCODE_TABLE_NAME = "qe6502_opcode_table"

ENUM_PREFIX = "QE6502_ADDR_MODE_"

ILLEGAL_NAME = "ILL"
ILLEGAL_DESCRIPTION = "Illegal opcode"
ILLEGAL_ADDR_MODE_STR = "Illegal"
ILLEGAL_ENUM_NAME = ENUM_PREFIX + "ILLEGAL"

# If an opcode entry has no "mode", this is used.
# If you prefer missing mode to be an error, change ALLOW_MISSING_MODE to False.
ALLOW_MISSING_MODE = True
DEFAULT_MISSING_MODE_STR = "Implied"


class OpcodeJsonError(Exception):
    pass


def parse_uint8(value: Any, field_name: str, opcode_context: str | None = None) -> int:
    """
    Parse a JSON value into a uint8_t-compatible integer.

    Accepts:
    - integer: 105
    - decimal string: "105"
    - hex string: "0x69"
    """
    ctx = f" for opcode {opcode_context}" if opcode_context else ""

    if isinstance(value, bool):
        raise OpcodeJsonError(f'Field "{field_name}"{ctx} must be an integer, not boolean')

    if isinstance(value, int):
        number = value
    elif isinstance(value, str):
        text = value.strip()
        if not text:
            raise OpcodeJsonError(f'Field "{field_name}"{ctx} is empty')
        try:
            number = int(text, 0)
        except ValueError as exc:
            raise OpcodeJsonError(
                f'Field "{field_name}"{ctx} has invalid integer value: {value!r}'
            ) from exc
    else:
        raise OpcodeJsonError(
            f'Field "{field_name}"{ctx} must be an integer or string, got {type(value).__name__}'
        )

    if not 0 <= number <= 0xFF:
        raise OpcodeJsonError(
            f'Field "{field_name}"{ctx} must be in range 0..255, got {number}'
        )

    return number


def require_string(entry: dict[str, Any], field_name: str, opcode_context: str) -> str:
    value = entry.get(field_name)

    if not isinstance(value, str):
        raise OpcodeJsonError(
            f'Field "{field_name}" for opcode {opcode_context} must be a string'
        )

    if value == "":
        raise OpcodeJsonError(
            f'Field "{field_name}" for opcode {opcode_context} must not be empty'
        )

    return value


def c_string_literal(value: str) -> str:
    """
    Convert Python string to a safe C string literal.
    """
    out = ['"']

    for ch in value:
        code = ord(ch)

        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        elif code < 32 or code == 127:
            out.append(f"\\x{code:02X}")
        else:
            out.append(ch)

    out.append('"')
    return "".join(out)


def split_camel_case(text: str) -> str:
    """
    Convert CamelCase-ish text into separated words.

    Examples:
    - ZeroPage -> Zero Page
    - ZeroPageRelative -> Zero Page Relative
    """
    text = re.sub(r"([a-z0-9])([A-Z])", r"\1 \2", text)
    text = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1 \2", text)
    return text


def mode_to_enum_name(mode: str) -> str:
    """
    Convert address mode string to a C enum name.

    Examples:
    - Immediate          -> QE6502_ADDR_MODE_IMMEDIATE
    - ZeroPage           -> QE6502_ADDR_MODE_ZERO_PAGE
    - ZeroPage Relative  -> QE6502_ADDR_MODE_ZERO_PAGE_RELATIVE
    - Absolute,X         -> QE6502_ADDR_MODE_ABSOLUTE_X
    - (Indirect),Y       -> QE6502_ADDR_MODE_INDIRECT_Y
    """
    text = mode.strip()

    if not text:
        raise OpcodeJsonError("Addressing mode string must not be empty")

    text = split_camel_case(text)
    text = text.upper()

    # Replace all non-alphanumeric runs with underscores.
    text = re.sub(r"[^A-Z0-9]+", "_", text)

    # Collapse duplicate underscores and trim.
    text = re.sub(r"_+", "_", text).strip("_")

    if not text:
        raise OpcodeJsonError(f"Could not derive enum name from addressing mode {mode!r}")

    return ENUM_PREFIX + text


def get_entry_mode(entry: dict[str, Any], opcode_context: str) -> str:
    if "mode" not in entry or entry["mode"] is None:
        if ALLOW_MISSING_MODE:
            return DEFAULT_MISSING_MODE_STR

        raise OpcodeJsonError(f'Field "mode" is missing for opcode {opcode_context}')

    if not isinstance(entry["mode"], str):
        raise OpcodeJsonError(f'Field "mode" for opcode {opcode_context} must be a string')

    mode = entry["mode"].strip()

    if not mode:
        raise OpcodeJsonError(f'Field "mode" for opcode {opcode_context} must not be empty')

    return mode


def parse_model_is_cmos(entry: dict[str, Any], opcode_context: str) -> int:
    if "model" not in entry or entry["model"] is None:
        return 0

    model = entry["model"]

    if not isinstance(model, str):
        raise OpcodeJsonError(f'Field "model" for opcode {opcode_context} must be a string')

    model_normalized = model.strip().lower()

    if model_normalized == "":
        return 0

    if model_normalized == "cmos":
        return 1

    raise OpcodeJsonError(
        f'Unsupported model {model!r} for opcode {opcode_context}; expected missing or "cmos"'
    )


def load_and_validate_json(input_path: Path) -> list[dict[str, Any]]:
    try:
        with input_path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except OSError as exc:
        raise OpcodeJsonError(f"Could not read input file {input_path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise OpcodeJsonError(f"Invalid JSON in {input_path}: {exc}") from exc

    if not isinstance(data, dict):
        raise OpcodeJsonError("Top-level JSON value must be an object")

    opcodes = data.get("opcodes")

    if not isinstance(opcodes, list):
        raise OpcodeJsonError('Top-level field "opcodes" must be an array')

    entries: list[dict[str, Any]] = []

    for index, item in enumerate(opcodes):
        if not isinstance(item, dict):
            raise OpcodeJsonError(f"Opcode entry at index {index} must be an object")

        if "opcode" not in item:
            raise OpcodeJsonError(f'Opcode entry at index {index} is missing field "opcode"')

        opcode = parse_uint8(item["opcode"], "opcode")
        opcode_context = f"0x{opcode:02X}"

        require_string(item, "name", opcode_context)

        if "bytes" not in item:
            raise OpcodeJsonError(f'Field "bytes" is missing for opcode {opcode_context}')

        parse_uint8(item["bytes"], "bytes", opcode_context)
        require_string(item, "description", opcode_context)
        get_entry_mode(item, opcode_context)
        parse_model_is_cmos(item, opcode_context)

        entries.append(item)

    return entries


def build_addr_mode_map(entries: list[dict[str, Any]]) -> dict[str, str]:
    """
    Returns:
        {
            "Immediate": "QE6502_ADDR_MODE_IMMEDIATE",
            ...
        }

    Raises an error if two different mode strings produce the same enum name.
    """
    mode_to_enum: dict[str, str] = {
        ILLEGAL_ADDR_MODE_STR: ILLEGAL_ENUM_NAME,
    }

    enum_to_mode: dict[str, str] = {
        ILLEGAL_ENUM_NAME: ILLEGAL_ADDR_MODE_STR,
    }

    for entry in entries:
        opcode = parse_uint8(entry["opcode"], "opcode")
        opcode_context = f"0x{opcode:02X}"

        mode = get_entry_mode(entry, opcode_context)
        enum_name = mode_to_enum_name(mode)

        existing_mode = enum_to_mode.get(enum_name)
        if existing_mode is not None and existing_mode != mode:
            raise OpcodeJsonError(
                "Addressing mode enum name collision: "
                f"{existing_mode!r} and {mode!r} both map to {enum_name}"
            )

        mode_to_enum[mode] = enum_name
        enum_to_mode[enum_name] = mode

    return mode_to_enum


def build_opcode_table(
    entries: list[dict[str, Any]],
    mode_to_enum: dict[str, str],
) -> list[dict[str, Any]]:
    """
    Builds a 256-entry table. Missing opcode slots become illegal opcodes.
    """
    table: list[dict[str, Any] | None] = [None] * 256

    for entry in entries:
        opcode = parse_uint8(entry["opcode"], "opcode")
        opcode_context = f"0x{opcode:02X}"

        if table[opcode] is not None:
            previous = table[opcode]
            assert previous is not None
            previous_name = previous["name"]
            current_name = entry["name"]
            raise OpcodeJsonError(
                f"Duplicate opcode {opcode_context}: {previous_name!r} and {current_name!r}"
            )

        name = require_string(entry, "name", opcode_context)
        byte_count = parse_uint8(entry["bytes"], "bytes", opcode_context)
        description = require_string(entry, "description", opcode_context)
        mode = get_entry_mode(entry, opcode_context)
        is_cmos_extension = parse_model_is_cmos(entry, opcode_context)

        table[opcode] = {
            "opcode": opcode,
            "name": name,
            "bytes": byte_count,
            "description": description,
            "addr_mode_str": mode,
            "addr_mode": mode_to_enum[mode],
            "is_cmos_extension": is_cmos_extension,
        }

    for opcode in range(256):
        if table[opcode] is None:
            table[opcode] = {
                "opcode": opcode,
                "name": ILLEGAL_NAME,
                "bytes": 1,
                "description": ILLEGAL_DESCRIPTION,
                "addr_mode_str": ILLEGAL_ADDR_MODE_STR,
                "addr_mode": ILLEGAL_ENUM_NAME,
                "is_cmos_extension": 0,
            }

    return table  # type: ignore[return-value]


def generate_header(
    table: list[dict[str, Any]],
    mode_to_enum: dict[str, str],
) -> str:
    lines: list[str] = []

    lines.append("/*")
    lines.append(" * Auto-generated file. Do not edit manually.")
    lines.append(" *")
    lines.append(" * Generated by generate_qe6502_opcodes.py.")
    lines.append(" */")
    lines.append("")
    lines.append(f"#ifndef {HEADER_NAME}")
    lines.append(f"#define {HEADER_NAME}")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("typedef enum {")
    lines.append(f"    {ILLEGAL_ENUM_NAME} = 0,")

    # Keep ILLEGAL first, then all real modes sorted by enum name.
    real_modes = [
        (mode, enum_name)
        for mode, enum_name in mode_to_enum.items()
        if enum_name != ILLEGAL_ENUM_NAME
    ]

    for _mode, enum_name in sorted(real_modes, key=lambda item: item[1]):
        lines.append(f"    {enum_name},")

    lines.append(f"}} {ADDR_MODE_TYPE};")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    const char *name;")
    lines.append("    const char *description;")
    lines.append("    const char *addr_mode_str;")
    lines.append("    const void* reserved_ptr;")
    lines.append("    uint8_t opcode;")
    lines.append("    uint8_t bytes;")
    lines.append("    uint8_t addr_mode;")
    lines.append("    uint8_t is_cmos_extension;")
    lines.append("    uint8_t reserved_data[4];")
    lines.append(f"}} {OPCODE_INFO_TYPE};")
    lines.append("")
    lines.append(f"static const {OPCODE_INFO_TYPE} {OPCODE_TABLE_NAME}[256] = {{")

    for item in table:
        opcode = item["opcode"]

        lines.append(f"    [0x{opcode:02X}] = {{")
        lines.append(f"        .name = {c_string_literal(item['name'])},")
        lines.append(f"        .description = {c_string_literal(item['description'])},")
        lines.append(f"        .addr_mode_str = {c_string_literal(item['addr_mode_str'])},")
        lines.append( "        .reserved_ptr = 0,")
        lines.append(f"        .opcode = 0x{opcode:02X},")
        lines.append(f"        .bytes = {item['bytes']},")
        lines.append(f"        .addr_mode = {item['addr_mode']},")
        lines.append(f"        .is_cmos_extension = {item['is_cmos_extension']},")
        lines.append( "        .reserved_data = {0, 0, 0, 0}")
        lines.append("    },")

    lines.append("};")
    lines.append("")
    lines.append(f"#endif /* {HEADER_NAME} */")
    lines.append("")

    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate a self-contained C header from 6502 opcode JSON."
    )

    parser.add_argument(
        "input_json",
        type=Path,
        help="Input JSON file containing the top-level field 'opcodes'.",
    )

    parser.add_argument(
        "output_header",
        type=Path,
        help="Output C header file.",
    )

    args = parser.parse_args()

    try:
        entries = load_and_validate_json(args.input_json)
        mode_to_enum = build_addr_mode_map(entries)
        table = build_opcode_table(entries, mode_to_enum)
        header = generate_header(table, mode_to_enum)

        args.output_header.write_text(header, encoding="utf-8")

    except OpcodeJsonError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"error: could not write output file {args.output_header}: {exc}", file=sys.stderr)
        return 1

    print(f"generated {args.output_header}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())