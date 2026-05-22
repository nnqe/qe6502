#!/usr/bin/env python3
"""
Generate a paste-ready C microcode stub block for cpu_v2/src/qe6502.c.

The generated C is intentionally only a compilable skeleton. Most handler bodies
are placeholders. The important part is that symbols and table layout come from:
  1) nmos6502_opcodes_meta.json
  2) nmos6502_class_codegen.json

Usage:
  python3 generate_qe6502_v2_stub.py \
      --opcodes nmos6502_opcodes_meta.json \
      --classes nmos6502_class_codegen.json \
      --out qe6502_v2_generated_stub.c
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any, Dict, List, Tuple

TABLE_ROWS = 256 + 16
CYCLES_PER_ROW = 8
TABLE_SIZE = TABLE_ROWS * CYCLES_PER_ROW


def c_ident(text: str) -> str:
    """Return a safe C identifier fragment."""
    text = text.lower()
    text = re.sub(r"[^a-z0-9_]+", "_", text)
    text = re.sub(r"_+", "_", text).strip("_")
    return text


def load_json(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def documented_opcodes(opcodes: Dict[str, Any]) -> Dict[str, Dict[str, Any]]:
    return {code: obj for code, obj in opcodes.items() if obj}


def expand_symbol(template: str, mnemonic: str) -> str:
    return template.replace("{mnemonic}", c_ident(mnemonic))


def resolve_opcode_cycles(opcode: Dict[str, Any], classes: Dict[str, Any]) -> List[Dict[str, Any]]:
    cls = opcode["class"]
    if cls not in classes:
        raise KeyError(f"missing class '{cls}'")

    result: List[Dict[str, Any]] = []
    for cycle in classes[cls]["cycles"]:
        item = dict(cycle)
        item["symbol"] = expand_symbol(item["symbol"], opcode["mnemonic"])
        result.append(item)
    return result


def collect_symbols(opcodes: Dict[str, Any], classes: Dict[str, Any]) -> Dict[str, Dict[str, Any]]:
    symbols: Dict[str, Dict[str, Any]] = {
        "op_ILL": {
            "handler_kind": "illegal_handler",
            "role": "illegal",
            "action": "placeholder illegal opcode handler",
        }
    }

    for opcode in documented_opcodes(opcodes).values():
        for cycle in resolve_opcode_cycles(opcode, classes):
            symbol = cycle["symbol"]
            symbols.setdefault(symbol, cycle)

    return symbols


def validate(opcodes: Dict[str, Any], classes: Dict[str, Any]) -> None:
    keys = [f"0x{i:02X}" for i in range(256)]
    if list(opcodes.keys()) != keys:
        raise ValueError("opcodes must contain ordered keys 0x00..0xFF")

    for code, opcode in documented_opcodes(opcodes).items():
        cls = opcode.get("class")
        if cls not in classes:
            raise ValueError(f"{code}: class '{cls}' is not defined")

        cycles = resolve_opcode_cycles(opcode, classes)
        if len(cycles) > CYCLES_PER_ROW:
            raise ValueError(f"{code}: class '{cls}' uses {len(cycles)} cycles, max is {CYCLES_PER_ROW}")

        if cycles[-1]["symbol"] != "mc_dispatch":
            raise ValueError(f"{code}: class '{cls}' does not end with mc_dispatch")

        documented_max = int(opcode["cycles"][1])
        if len(cycles) != documented_max + 1:
            raise ValueError(
                f"{code}: class '{cls}' has {len(cycles)} microcode cycles; "
                f"expected documented max {documented_max} + dispatch"
            )


def mnemonic_from_op_symbol(symbol: str) -> str:
    # op_lda_r_fetch -> lda
    parts = symbol.split("_")
    if len(parts) >= 2 and parts[0] == "op":
        return parts[1]
    return ""


def write_value_for_mnemonic(mnemonic: str) -> str:
    if mnemonic == "sta":
        return "cpu->A"
    if mnemonic == "stx":
        return "cpu->X"
    if mnemonic == "sty":
        return "cpu->Y"
    return "cpu->latch_data"


def emit_function(symbol: str, meta: Dict[str, Any]) -> str:
    action = meta.get("action", "TODO")
    kind = meta.get("handler_kind", meta.get("kind", "handler"))
    role = meta.get("role", "")
    lines: List[str] = []

    lines.append(f"/* {kind}; role={role}; action={action} */")
    lines.append(f"static qe6502_tick_t {symbol}(qe6502_t* cpu, uint8_t bus)")
    lines.append("{")

    if symbol == "mc_dispatch":
        lines.append("    cpu->microcode = (uint16_t)((uint16_t)bus << 3);")
        lines.append("    cpu->PC = (uint16_t)(cpu->PC + 1u);")
        lines.append("    return qe6502_microcode_table[cpu->microcode](cpu, bus);")

    elif symbol in ("mc_fetch", "mc_fetch_no_interrupts"):
        lines.append("    (void)bus;")
        lines.append("    qe6502_tick_t tick = read(cpu, cpu->PC);")
        lines.append("    tick.status = (uint8_t)(tick.status | qe6502_status_instr_done);")
        lines.append("    return tick;")

    elif symbol == "op_ILL":
        lines.append("    (void)bus;")
        lines.append("    (void)(flag_C | flag_Z | flag_I | flag_D | flag_B | flag_UN | flag_V | flag_N);")
        lines.append("    cpu->status = (uint8_t)(qe6502_error_illegal_op | qe6502_status_halted);")
        lines.append("    cpu->microcode = (uint16_t)(cpu->microcode - 1u);")
        lines.append("    return read(cpu, cpu->PC);")

    elif symbol.startswith("op_"):
        mnemonic = mnemonic_from_op_symbol(symbol)

        if symbol.endswith("_r_fetch") or symbol.endswith("_stack_pull_fetch"):
            lines.append("    cpu->latch_data = bus;")
            lines.append("    /* TODO: implement mnemonic semantics before fetching next opcode. */")
            lines.append("    return mc_fetch(cpu, bus);")

        elif symbol.endswith("_w_wr"):
            value = write_value_for_mnemonic(mnemonic)
            lines.append("    (void)bus;")
            lines.append("    /* TODO: implement store/write mnemonic semantics. */")
            lines.append(f"    return write(cpu, cpu->latch_addr, {value});")

        elif symbol.endswith("_rw_wr"):
            lines.append("    (void)bus;")
            lines.append("    /* TODO: modify cpu->latch_data and update flags before final write. */")
            lines.append("    return write(cpu, cpu->latch_addr, cpu->latch_data);")

        elif symbol.endswith("_stack_push_wr"):
            if mnemonic == "php":
                value = "(uint8_t)(cpu->P | flag_B)"
            else:
                value = "cpu->A"
            lines.append("    (void)bus;")
            lines.append("    /* TODO: verify exact pushed value. */")
            lines.append(f"    return stack_write(cpu, {value});")

        elif symbol.endswith("_imp_dummy") or symbol.endswith("_acc_dummy"):
            lines.append("    (void)bus;")
            lines.append("    /* TODO: implement implied/accumulator mnemonic semantics. */")
            lines.append("    return read(cpu, cpu->PC);")

        elif "_branch_c0_offset" in symbol:
            lines.append("    (void)bus;")
            lines.append("    /* TODO: read signed branch offset, test condition, and adjust microcode. */")
            lines.append("    qe6502_tick_t tick = read(cpu, cpu->PC);")
            lines.append("    cpu->PC = (uint16_t)(cpu->PC + 1u);")
            lines.append("    return tick;")

        else:
            lines.append("    cpu->latch_data = bus;")
            lines.append("    /* TODO: semantic placeholder. */")
            lines.append("    return read(cpu, cpu->PC);")

    else:
        # Generic mc_* placeholders. They are intentionally simple but compile.
        if "push" in symbol:
            lines.append("    (void)bus;")
            lines.append("    /* TODO: replace with exact stack push value. */")
            lines.append("    return stack_write(cpu, cpu->latch_data);")

        elif "pull" in symbol or symbol.endswith("_read"):
            lines.append("    cpu->latch_data = bus;")
            lines.append("    /* TODO: replace with exact stack/read behavior. */")
            lines.append("    return stack_read(cpu);")

        elif "wb" in symbol or symbol.endswith("_wr"):
            lines.append("    cpu->latch_data = bus;")
            lines.append("    /* TODO: write-back placeholder. */")
            lines.append("    return write(cpu, cpu->latch_addr, cpu->latch_data);")

        elif "hi" in symbol or "ptrhi" in symbol or "pch" in symbol:
            lines.append("    set_latch_addr1(cpu, bus);")
            lines.append("    /* TODO: replace with exact high-byte/address behavior. */")
            lines.append("    return read(cpu, cpu->PC);")

        elif "lo" in symbol or "addr" in symbol or "ptr" in symbol or "pcl" in symbol:
            lines.append("    set_latch_addr0(cpu, bus);")
            lines.append("    /* TODO: replace with exact low-byte/address behavior. */")
            lines.append("    qe6502_tick_t tick = read(cpu, cpu->PC);")
            lines.append("    cpu->PC = (uint16_t)(cpu->PC + 1u);")
            lines.append("    return tick;")

        elif "fetch" in symbol:
            lines.append("    return mc_fetch(cpu, bus);")

        else:
            lines.append("    cpu->latch_data = bus;")
            lines.append("    /* TODO: generic microcode placeholder. */")
            lines.append("    return read(cpu, cpu->PC);")

    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def sort_key(symbol: str) -> Tuple[int, str]:
    if symbol == "op_ILL":
        return (0, symbol)
    if symbol in ("mc_fetch", "mc_fetch_no_interrupts", "mc_dispatch"):
        return (1, symbol)
    if symbol.startswith("mc_"):
        return (2, symbol)
    return (3, symbol)


def table_symbols_for_opcode(opcode: Dict[str, Any], classes: Dict[str, Any]) -> List[str]:
    symbols = [cycle["symbol"] for cycle in resolve_opcode_cycles(opcode, classes)]
    while len(symbols) < CYCLES_PER_ROW:
        symbols.append("op_ILL")
    return symbols


def emit_table(opcodes: Dict[str, Any], classes: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("#define IDX(opcode, cycle) ((((opcode) & 0x1FFu) << 3u) + (cycle))")
    lines.append("")
    lines.append(f"const qe6502_microcode_fn qe6502_microcode_table[{TABLE_SIZE}] =")
    lines.append("{")

    for opcode_value in range(TABLE_ROWS):
        if opcode_value < 256:
            code = f"0x{opcode_value:02X}"
            opcode = opcodes[code]
            if opcode:
                title = f"{code} {opcode['syntax']} ; class={opcode['class']}"
                symbols = table_symbols_for_opcode(opcode, classes)
            else:
                title = f"{code} undocumented/illegal"
                symbols = ["op_ILL"] * CYCLES_PER_ROW
        else:
            code = f"0x{opcode_value:03X}"
            title = f"{code} reserved extension row"
            symbols = ["op_ILL"] * CYCLES_PER_ROW

        lines.append(f"    /* {title} */")
        for cycle, symbol in enumerate(symbols):
            lines.append(f"    [IDX({code}, {cycle})] = &{symbol},")
        lines.append("")

    lines.append("};")
    lines.append("")
    lines.append("#undef IDX")
    lines.append("")
    return "\n".join(lines)


def generate(opcodes_path: Path, classes_path: Path) -> str:
    opmeta = load_json(opcodes_path)
    classmeta = load_json(classes_path)

    opcodes = opmeta["opcodes"]
    classes = classmeta["classes"]

    validate(opcodes, classes)
    symbols = collect_symbols(opcodes, classes)

    lines: List[str] = []
    lines.append("/*")
    lines.append(" * Generated QE6502 v2 NMOS skeleton.")
    lines.append(" * Paste into cpu_v2/src/qe6502.c after the low-level helpers.")
    lines.append(" * Replace the old experimental dispatcher/fetch/op/table block.")
    lines.append(" * Handler bodies are placeholders; table symbols come from JSON.")
    lines.append(" */")
    lines.append("")

    for symbol in sorted(symbols, key=sort_key):
        lines.append(emit_function(symbol, symbols[symbol]))

    lines.append(emit_table(opcodes, classes))
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate QE6502 v2 C skeleton from JSON metadata.")
    parser.add_argument("--opcodes", type=Path, default=Path("nmos6502_opcodes_meta.json"))
    parser.add_argument("--classes", type=Path, default=Path("nmos6502_class_codegen.json"))
    parser.add_argument("--out", type=Path, default=Path("qe6502_v2_generated_stub.c"))
    args = parser.parse_args()

    c_text = generate(args.opcodes, args.classes)
    args.out.write_text(c_text, encoding="utf-8")
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
