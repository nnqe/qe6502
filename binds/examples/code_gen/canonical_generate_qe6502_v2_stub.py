#!/usr/bin/env python3
"""
Generate a paste-ready QE6502 v2 NMOS C skeleton from opcode metadata and
class codegen metadata.

This generator requires terminal_contract support. It intentionally fails when
fed an old class_codegen JSON that still produces legacy mnemonic symbols like:
  op_lda_r_fetch
  op_sta_w_wr
  op_inc_rw_wr
  op_clc_imp_dummy

Expected inputs:
  --opcodes canonical_nmos6502_opcodes_meta.json
  --classes canonical_nmos6502_class_codegen.json

Output:
  a C block suitable for review against cpu_v2/src/qe6502_v2.c.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

OPCODE_SLOTS = 256
MODEL_COUNT = 5
SLOTS_PER_MODEL = 512
CYCLES_PER_SLOT = 8
CONTROL_STORE_SIZE = MODEL_COUNT * SLOTS_PER_MODEL * CYCLES_PER_SLOT

ALLOWED_READY = {"none", "addr", "addrlo", "addr_data", "custom"}
ALLOWED_PENDING = {"none", "data", "addrlo", "addrhi", "custom"}

NORMAL_CLASSES_REQUIRING_TERMINAL_SYMBOLS = (
    "r_", "w_", "rw_",
)
EXPLICIT_TERMINAL_CLASSES = {"acc", "imp", "stack_push", "stack_pull"}

# These are the legacy names that caused the current mismatch. The generated
# output must not contain definitions or control-store references to them.
LEGACY_OP_SYMBOL_RE = re.compile(
    r"^op_[a-z0-9]+_(?:"
    r"r_fetch|"
    r"w_wr|"
    r"rw_wr|"
    r"acc_dummy|"
    r"imp_dummy|"
    r"stack_push_wr|"
    r"stack_pull_fetch"
    r")$"
)


def c_ident(text: str) -> str:
    text = text.lower()
    text = re.sub(r"[^a-z0-9_]+", "_", text)
    text = re.sub(r"_+", "_", text).strip("_")
    return text


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def documented_opcodes(opcodes: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {code: obj for code, obj in opcodes.items() if obj}


def terminal_contract_suffix(contract: dict[str, str]) -> str:
    return f"ready_{contract['ready']}_pending_{contract['pending']}"


def class_contract(classes: dict[str, Any], cls: str) -> dict[str, str]:
    item = classes.get(cls)
    if item is None:
        raise ValueError(f"missing class '{cls}'")

    contract = item.get("terminal_contract")
    if not isinstance(contract, dict):
        raise ValueError(
            f"class '{cls}' is missing terminal_contract. "
            "You are probably using an old class_codegen JSON."
        )

    ready = contract.get("ready")
    pending = contract.get("pending")
    if ready not in ALLOWED_READY:
        raise ValueError(f"class '{cls}' has invalid terminal_contract.ready '{ready}'")
    if pending not in ALLOWED_PENDING:
        raise ValueError(f"class '{cls}' has invalid terminal_contract.pending '{pending}'")

    return {"ready": ready, "pending": pending}


def expand_symbol(template: str, mnemonic: str, cls: str, classes: dict[str, Any]) -> str:
    contract = class_contract(classes, cls)
    return (
        template
        .replace("{mnemonic}", c_ident(mnemonic))
        .replace("{class}", c_ident(cls))
        .replace("{terminal_contract}", terminal_contract_suffix(contract))
        .replace("{terminal_ready}", c_ident(contract["ready"]))
        .replace("{terminal_pending}", c_ident(contract["pending"]))
    )


def resolve_opcode_cycles(opcode: dict[str, Any], classes: dict[str, Any]) -> list[dict[str, Any]]:
    cls = opcode["class"]
    if cls not in classes:
        raise ValueError(f"opcode class '{cls}' is not defined")

    resolved = []
    for cycle in classes[cls]["cycles"]:
        item = dict(cycle)
        item["class"] = cls
        item["terminal_contract"] = class_contract(classes, cls)
        item["symbol_template"] = item["symbol"]
        item["symbol"] = expand_symbol(item["symbol"], opcode["mnemonic"], cls, classes)
        resolved.append(item)
    return resolved


def validate_symbol_templates(classes: dict[str, Any]) -> None:
    for cls, class_info in classes.items():
        class_contract(classes, cls)

        for cycle in class_info.get("cycles", []):
            if cycle.get("handler_kind") != "mnemonic_handler":
                continue

            template = cycle.get("symbol", "")
            requires_contract = (
                cls.startswith(NORMAL_CLASSES_REQUIRING_TERMINAL_SYMBOLS)
                or cls in EXPLICIT_TERMINAL_CLASSES
            )
            if requires_contract and "{terminal_contract}" not in template:
                raise ValueError(
                    f"class '{cls}' mnemonic handler '{template}' does not include "
                    "{terminal_contract}; this would generate legacy op_* names."
                )


def validate(opcodes: dict[str, Any], classes: dict[str, Any]) -> None:
    keys = [f"0x{i:02X}" for i in range(256)]
    if list(opcodes.keys()) != keys:
        raise ValueError("opcodes must contain ordered keys 0x00..0xFF")

    validate_symbol_templates(classes)

    for code, opcode in documented_opcodes(opcodes).items():
        cls = opcode.get("class")
        if cls not in classes:
            raise ValueError(f"{code}: class '{cls}' is not defined")

        cycles = resolve_opcode_cycles(opcode, classes)
        if len(cycles) > CYCLES_PER_SLOT:
            raise ValueError(f"{code}: class '{cls}' uses {len(cycles)} cycles, max is {CYCLES_PER_SLOT}")
        if cycles[-1]["symbol"] != "mc_dispatch":
            raise ValueError(f"{code}: class '{cls}' does not end with mc_dispatch")

        documented_max = int(opcode["cycles"][1])
        expected = documented_max + 1
        if len(cycles) != expected:
            raise ValueError(
                f"{code}: class '{cls}' has {len(cycles)} microcode cycles; "
                f"expected documented max {documented_max} + dispatch = {expected}"
            )

    all_symbols = collect_symbols(opcodes, classes)
    legacy = sorted(s for s in all_symbols if LEGACY_OP_SYMBOL_RE.match(s))
    if legacy:
        raise ValueError("legacy mnemonic symbols generated: " + ", ".join(legacy))


def collect_symbols(opcodes: dict[str, Any], classes: dict[str, Any]) -> dict[str, dict[str, Any]]:
    symbols: dict[str, dict[str, Any]] = {
        "op_error_trap": {
            "handler_kind": "illegal_handler",
            "role": "illegal",
            "action": "trap_on_cpu_error",
            "terminal_contract": {"ready": "none", "pending": "none"},
        }
    }

    for opcode in documented_opcodes(opcodes).values():
        for cycle in resolve_opcode_cycles(opcode, classes):
            symbols.setdefault(cycle["symbol"], cycle)

    return symbols


def mnemonic_from_op_symbol(symbol: str) -> str:
    parts = symbol.split("_")
    return parts[1] if len(parts) > 1 and parts[0] == "op" else ""


def write_value_for_mnemonic(mnemonic: str) -> str:
    if mnemonic == "sta":
        return "cpu->A"
    if mnemonic == "stx":
        return "cpu->X"
    if mnemonic == "sty":
        return "cpu->Y"
    return "cpu->latch_data"


def emit_store(lines: list[str], mnemonic: str, contract: dict[str, str]) -> None:
    value = write_value_for_mnemonic(mnemonic)
    ready = contract["ready"]
    pending = contract["pending"]

    if ready == "addr" and pending == "none":
        lines.append("    (void)bus;")
        lines.append("    /* TODO: implement store/write mnemonic semantics. Address is already ready. */")
        lines.append(f"    return write(cpu, cpu->latch_addr, {value});")
        return

    if ready == "none" and pending == "addrlo":
        lines.append("    cpu->latch_addr = bus;")
        lines.append("    /* TODO: implement store/write mnemonic semantics. bus is the low/zero-page address byte. */")
        lines.append(f"    return write(cpu, cpu->latch_addr, {value});")
        return

    if ready == "addrlo" and pending == "addrhi":
        lines.append("    set_latch_addr1(cpu, bus);")
        lines.append("    /* TODO: implement store/write mnemonic semantics. bus completes the high address byte. */")
        lines.append(f"    return write(cpu, cpu->latch_addr, {value});")
        return

    lines.append("    cpu->latch_data = bus;")
    lines.append("    /* TODO: unsupported write terminal contract placeholder. */")
    lines.append(f"    return write(cpu, cpu->latch_addr, {value});")


def emit_function(symbol: str, meta: dict[str, Any]) -> str:
    kind = meta.get("handler_kind", "handler")
    role = meta.get("role", "")
    action = meta.get("action", "TODO")
    contract = meta.get("terminal_contract", {"ready": "none", "pending": "none"})

    lines: list[str] = []
    lines.append(f"/* {kind}; role={role}; action={action} */")
    lines.append(f"static qe6502_tick_t {symbol}(qe6502_t* cpu, uint8_t bus)")
    lines.append("{")

    if symbol == "op_error_trap":
        lines.append("    (void)bus;")
        lines.append("    cpu->status = (uint8_t)(qe6502_status_tick_not_ok);")
        lines.append("    cpu->microcode = (uint16_t)(cpu->microcode - 1u);")
        lines.append("    return read(cpu, cpu->PC);")

    elif symbol == "mc_dispatch":
        lines.append("    cpu->microcode = (uint16_t)(((uint16_t)cpu->model << 12u) | ((uint16_t)bus << 3u));")
        lines.append("    cpu->PC = (uint16_t)(cpu->PC + 1u);")
        lines.append("    return qe6502_control_store[cpu->microcode](cpu, bus);")

    elif symbol in {"mc_fetch", "mc_fetch_no_interrupts"}:
        lines.append("    (void)bus;")
        lines.append("    qe6502_tick_t tick = read(cpu, cpu->PC);")
        lines.append("    tick.status = (uint8_t)(tick.status | qe6502_status_opcode_fetch);")
        lines.append("    return tick;")

    elif symbol.startswith("op_"):
        mnemonic = mnemonic_from_op_symbol(symbol)

        if "_r_ready_none_pending_data_fetch" in symbol:
            lines.append("    cpu->latch_data = bus;")
            lines.append("    /* TODO: implement reader mnemonic semantics before fetching next opcode. */")
            lines.append("    return mc_fetch(cpu, bus);")

        elif "_w_ready_" in symbol and symbol.endswith("_wr"):
            emit_store(lines, mnemonic, contract)

        elif "_rw_ready_addr_data_pending_none_wr" in symbol:
            lines.append("    (void)bus;")
            lines.append("    /* TODO: modify cpu->latch_data and update flags before final write. */")
            lines.append("    return write(cpu, cpu->latch_addr, cpu->latch_data);")

        elif "_acc_ready_none_pending_none_dummy" in symbol:
            lines.append("    (void)bus;")
            lines.append("    /* TODO: implement accumulator mnemonic semantics. */")
            lines.append("    return read(cpu, cpu->PC);")

        elif "_imp_ready_none_pending_none_dummy" in symbol:
            lines.append("    (void)bus;")
            lines.append("    /* TODO: implement implied/register mnemonic semantics. */")
            lines.append("    return read(cpu, cpu->PC);")

        elif "_stack_push_ready_none_pending_none_wr" in symbol:
            value = "(uint8_t)(cpu->P | flag_B)" if mnemonic == "php" else "cpu->A"
            lines.append("    (void)bus;")
            lines.append("    /* TODO: verify exact pushed value. */")
            lines.append(f"    return stack_write(cpu, {value});")

        elif "_stack_pull_ready_none_pending_data_fetch" in symbol:
            lines.append("    cpu->latch_data = bus;")
            lines.append("    /* TODO: implement stack pull mnemonic semantics before fetching next opcode. */")
            lines.append("    return mc_fetch(cpu, bus);")

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
        # Compile-safe placeholder for mc_* helpers. Manual implementations can
        # replace these incrementally.
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


def sort_key(symbol: str) -> tuple[int, str]:
    if symbol == "op_error_trap":
        return (0, symbol)
    if symbol in {"mc_fetch", "mc_fetch_no_interrupts", "mc_dispatch"}:
        return (1, symbol)
    if symbol.startswith("mc_"):
        return (2, symbol)
    return (3, symbol)


def control_store_symbols_for_opcode(opcode: dict[str, Any], classes: dict[str, Any]) -> list[str]:
    symbols = [cycle["symbol"] for cycle in resolve_opcode_cycles(opcode, classes)]
    while len(symbols) < CYCLES_PER_SLOT:
        symbols.append("op_error_trap")
    return symbols


def emit_control_store(opcodes: dict[str, Any], classes: dict[str, Any]) -> str:
    lines: list[str] = []
    lines.append("#define IDX(model, opcode, cycle) ((((model) & 0x0Fu) << 12u) + (((opcode) & 0x1FFu) << 3u) + (cycle))")
    lines.append("")
    lines.append("const qe6502_microcode_fn qe6502_control_store[qe6502_control_store_size] =")
    lines.append("    {")

    for opcode_value in range(OPCODE_SLOTS):
        code = f"0x{opcode_value:02X}"
        opcode = opcodes[code]
        if opcode:
            title = f"{code} {opcode['syntax']} ; class={opcode['class']}"
            symbols = control_store_symbols_for_opcode(opcode, classes)
        else:
            title = f"{code} undocumented/illegal"
            symbols = ["op_error_trap"] * CYCLES_PER_SLOT

        lines.append(f"        /* {title} */")
        for cycle, symbol in enumerate(symbols):
            lines.append(f"        [IDX(qe6502_model_nmos, {code}, {cycle})] = &{symbol},")
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

    lines: list[str] = []
    lines.append("/*")
    lines.append(" * Generated QE6502 NMOS control-store stub.")
    lines.append(" * Terminal-contract codegen version.")
    lines.append(" * Review against cpu_v2/src/qe6502_v2.c and control-store blocks.")
    lines.append(" */")
    lines.append("")

    for symbol in sorted(symbols, key=sort_key):
        lines.append(emit_function(symbol, symbols[symbol]))

    lines.append(emit_control_store(opcodes, classes))
    c_text = "\n".join(lines)

    legacy_in_output = sorted(set(LEGACY_OP_SYMBOL_RE.findall(c_text)))
    if legacy_in_output:
        raise ValueError("legacy symbols leaked into C output")
    return c_text


def write_report(c_text: str, report_path: Path) -> None:
    function_defs = re.findall(r"^static\s+qe6502_tick_t\s+(\w+)\(", c_text, re.M)
    control_store_refs = re.findall(r"= &(\w+),", c_text)
    legacy_refs = sorted({s for s in function_defs + control_store_refs if LEGACY_OP_SYMBOL_RE.match(s)})
    missing_defs = sorted({s for s in control_store_refs if s not in function_defs and s != "op_error_trap"})

    report = {
        "function_count": len(function_defs),
        "control_store_ref_count": len(control_store_refs),
        "legacy_op_symbols": legacy_refs,
        "missing_control_store_definitions": missing_defs,
        "ok": not legacy_refs and not missing_defs,
    }
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if not report["ok"]:
        raise ValueError(f"generated report failed validation: {report_path}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate QE6502 v2 terminal-contract C skeleton.")
    parser.add_argument("--opcodes", type=Path, default=Path("canonical_nmos6502_opcodes_meta.json"))
    parser.add_argument("--classes", type=Path, default=Path("canonical_nmos6502_class_codegen.json"))
    parser.add_argument("--out", type=Path, default=Path("canonical_qe6502_v2_generated_stub.c"))
    parser.add_argument("--report", type=Path, default=Path("canonical_qe6502_v2_generated_stub_report.json"))
    args = parser.parse_args()

    c_text = generate(args.opcodes, args.classes)
    args.out.write_text(c_text, encoding="utf-8")
    write_report(c_text, args.report)
    print(f"wrote {args.out}")
    print(f"wrote {args.report}")


if __name__ == "__main__":
    main()
