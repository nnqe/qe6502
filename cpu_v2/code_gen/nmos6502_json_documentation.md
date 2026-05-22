# NMOS 6502 JSON Files

This document describes two JSON files used for the NMOS 6502 core generator:

- `nmos6502_opcodes_meta.json`
- `nmos6502_class_codegen.json`

The first file describes the documented NMOS 6502 opcode set.  
The second file describes how each opcode class expands into microcode table entries and handler stubs.

---

## 1. `nmos6502_opcodes_meta.json`

This file is the opcode metadata file. It should stay mostly factual. It should not contain C function names or microcode table details.

Top-level structure:

```json
{
  "meta": {},
  "opcodes": {}
}
```

### 1.1 `meta`

`meta` contains shared information used to describe and validate the opcode table.

Important sections:

```json
{
  "instructions": {},
  "classes": {},
  "addressing_modes": {}
}
```

#### `meta.instructions`

Contains one entry per instruction mnemonic.

Example:

```json
"LDA": {
  "summary": "Load A from memory operand; updates N and Z."
}
```

This is descriptive only. It does not drive the microcode flow directly.

#### `meta.classes`

Contains the opcode execution classes used by the generator.

Example:

```json
"r_abx": {
  "summary": "Read-class instruction using absolute,X addressing; may include page-cross correction."
}
```

The `class` value is the key that selects a code-generation flow in `nmos6502_class_codegen.json`.

Examples:

| Class | Meaning |
|---|---|
| `r_imm` | Read instruction with immediate operand |
| `r_zp` | Read instruction with zero-page operand |
| `r_abx` | Read instruction with absolute,X operand |
| `w_abs` | Write instruction with absolute operand |
| `rw_abx` | Read-modify-write instruction with absolute,X operand |
| `acc` | Accumulator operation |
| `imp` | Implied/register operation |
| `branch_rel` | Relative branch |
| `jmp_abs` | Absolute JMP |
| `jmp_ind` | Indirect JMP |
| `jsr_abs` | JSR absolute |
| `brk` | BRK flow |
| `rts` | RTS flow |
| `rti` | RTI flow |
| `stack_push` | PHA/PHP stack push |
| `stack_pull` | PLA/PLP stack pull |

#### `meta.addressing_modes`

Contains human-readable addressing mode data.

Example:

```json
"Absolute,X": {
  "token": "abx",
  "bytes": 3,
  "summary": "A 16-bit base address is indexed by X; read instructions may need an extra cycle on page crossing."
}
```

This section is useful for documentation and validation. The generator should still use the opcode `class` as the main flow key.

---

### 1.2 `opcodes`

`opcodes` is an object with all 256 opcode keys:

```json
"opcodes": {
  "0x00": {},
  "0x01": {},
  "...": {},
  "0xFF": {}
}
```

Documented NMOS 6502 opcodes contain metadata. Undocumented opcode slots are empty objects.

Example:

```json
"0xBD": {
  "mnemonic": "LDA",
  "class": "r_abx",
  "addressing_mode": "Absolute,X",
  "bytes": 3,
  "cycles": [4, 5],
  "flags": ["page_cross"],
  "syntax": "LDA abs,X",
  "summary": "Load A from memory operand; updates N and Z."
}
```

Field meanings:

| Field | Meaning |
|---|---|
| `mnemonic` | Instruction name, such as `LDA`, `STA`, `ROR` |
| `class` | Execution class; selects a flow in the codegen file |
| `addressing_mode` | Human-readable addressing mode name |
| `bytes` | Instruction length in bytes |
| `cycles` | Documented CPU timing as `[min, max]` |
| `flags` | Optional notes, such as `page_cross` |
| `syntax` | Short assembly syntax |
| `summary` | Short instruction description |

Important rule:

```text
opcodes.json does not name C handlers.
```

It only says which `class` the opcode belongs to.

---

## 2. `nmos6502_class_codegen.json`

This file describes how each opcode class is generated into microcode table entries and handler stubs.

Top-level structure:

```json
{
  "meta": {},
  "classes": {}
}
```

This file is the main input for the code generator.

---

### 2.1 Main idea

Each documented opcode in `nmos6502_opcodes_meta.json` has a `class`.

Example:

```json
"0xBD": {
  "mnemonic": "LDA",
  "class": "r_abx"
}
```

The generator looks up `r_abx` in `nmos6502_class_codegen.json`:

```json
"r_abx": {
  "cycles": [ ... ]
}
```

Then it expands the class cycles into table entries.

Mnemonic placeholders are expanded only in mnemonic-specific handlers:

```text
op_{mnemonic}_r_fetch
```

For `LDA`, this becomes:

```c
op_lda_r_fetch
```

Addressing handlers must not contain the mnemonic name.

Good:

```c
mc_r_abx_c0_lo
mc_r_abx_c1_hi
mc_r_abx_c2_probe
mc_r_abx_c3_pgfix
op_lda_r_fetch
mc_dispatch
```

Bad:

```c
mc_r_abx_c0_lo_lda
mc_r_abx_c1_hi_lda
```

---

### 2.2 Cycle semantics

In the codegen file, `cycles` means **microcode table entries**, not only documented 6502 instruction cycles.

The final entry is normally:

```c
mc_dispatch
```

So the rule is usually:

```text
len(class.cycles) == documented max instruction cycles + 1
```

Example: `ROR abs,X` is a 7-cycle instruction.

Its class `rw_abx` has 8 microcode entries:

```text
c0  low operand byte
c1  high operand byte
c2  indexed probe/dummy read
c3  final data read
c4  write back old value
c5  execute ROR and emit final write
c6  fetch next opcode
c7  dispatch fetched opcode
```

---

### 2.3 Class structure

Each class has this form:

```json
"r_abx": {
  "description": "Reader flow for absolute,X operands with optional page-cross correction.",
  "cycles": [],
  "v1_reference": [],
  "ai_certainty": "high",
  "ai_comment": "..."
}
```

Class fields:

| Field | Meaning |
|---|---|
| `description` | Short description of this execution class |
| `cycles` | Ordered microcode entries for this class |
| `v1_reference` | Related qe6502 v1 functions used as reference |
| `ai_certainty` | Confidence level: `high`, `medium`, or `low` |
| `ai_comment` | Short note about confidence, risk, or design detail |

---

### 2.4 Cycle entry structure

Each item in `cycles` describes one microcode table entry.

Example:

```json
{
  "role": "probe",
  "symbol": "mc_r_abx_c2_probe",
  "handler_kind": "addressing_handler",
  "action": "read_indexed_probe_address_to_data_and_skip_pgfix_if_no_page_cross",
  "control": "if no page cross, advance microcode to skip mc_r_abx_c3_pgfix",
  "v1_reference": ["nmos_pre_r_absolute_x_3"],
  "ai_certainty": "high",
  "ai_comment": "Cycle role and order were derived from qe6502 v1 NMOS handler flow."
}
```

Cycle fields:

| Field | Meaning |
|---|---|
| `role` | Short human name for the cycle role |
| `symbol` | C handler symbol or symbol template |
| `handler_kind` | Type of handler to generate or reference |
| `action` | Short description of what the handler does |
| `terminal_action` | Optional final action style, such as `fetch_now` |
| `control` | Optional control-flow note for conditional cycles |
| `v1_reference` | qe6502 v1 function names used as reference |
| `ai_certainty` | Confidence level for this entry |
| `ai_comment` | Short note about risk or uncertainty |

---

## 3. Handler kinds

`handler_kind` tells the generator how to treat the symbol.

| Kind | Meaning |
|---|---|
| `addressing_handler` | Reusable addressing/class handler. No mnemonic in the name. |
| `mnemonic_handler` | Semantic handler. May use `{mnemonic}`. |
| `special_handler` | Control, jump, stack, interrupt, or return handler. |
| `control_handler` | Conditional flow handler, used for branch/page-cross logic. |
| `shared_handler` | Shared handler such as `mc_fetch` or `mc_dispatch`. |

---

## 4. Terminal actions

`terminal_action` describes what the handler returns or how the flow continues.

| Action | Meaning |
|---|---|
| `fetch_now` | Execute semantics and immediately emit next opcode fetch. |
| `write_then_next_cycle` | Emit a write cycle; the following `mc_fetch` consumes it. |
| `dummy_then_next_cycle` | Emit a dummy read; the following `mc_fetch` consumes it. |
| `fetch_cycle` | Shared fetch handler emits the next opcode read. |
| `fetch_cycle_no_interrupts` | Fetch next opcode without interrupt check. |
| `dispatch` | Final dispatch entry. |
| `conditional_branch_start` | Branch handler decides not-taken/taken path. |

This is useful for generating C stubs.

Example stub for a reader semantic handler:

```c
static qe6502_tick_t op_lda_r_fetch(qe6502_t* cpu, uint8_t bus)
{
    /* TODO: implement LDA semantics */
    return fetch_now(cpu);
}
```

Example stub for a writer semantic handler:

```c
static qe6502_tick_t op_sta_w_wr(qe6502_t* cpu, uint8_t bus)
{
    /* TODO: prepare value to write */
    return write(cpu, cpu->latch_addr, cpu->latch_data);
}
```

---

## 5. Code generation process

A simple generator can follow these steps:

1. Load `nmos6502_opcodes_meta.json`.
2. Load `nmos6502_class_codegen.json`.
3. For each opcode from `0x00` to `0xFF`:
   - If the opcode object is empty, emit the illegal/empty opcode behavior.
   - Read `mnemonic` and `class`.
   - Find the class in `class_codegen.classes`.
   - Expand each cycle symbol.
   - Replace `{mnemonic}` with the lowercase mnemonic.
   - Add the resolved symbols to the opcode microcode row.
4. Collect required handler symbols.
5. Generate missing handler stubs by `handler_kind` and `terminal_action`.
6. Validate cycle counts.

Example expansion:

```json
"0xBD": {
  "mnemonic": "LDA",
  "class": "r_abx",
  "cycles": [4, 5]
}
```

Class:

```text
r_abx
```

Generated row:

```c
mc_r_abx_c0_lo
mc_r_abx_c1_hi
mc_r_abx_c2_probe
mc_r_abx_c3_pgfix
op_lda_r_fetch
mc_dispatch
```

For a no-page-cross case, `mc_r_abx_c2_probe` skips `mc_r_abx_c3_pgfix`.

---

## 6. Naming rules

### 6.1 Addressing/class handlers

Format:

```text
mc_<class>_c<N>_<role>
```

Examples:

```c
mc_r_zp_c0_addr
mc_r_zp_c1_data
mc_w_abx_c2_probe
mc_rw_abx_c4_wb
```

These handlers are shared by all mnemonics in the same class.

### 6.2 Mnemonic handlers

Format:

```text
op_<mnemonic>_<class-part>_<action>
```

Examples:

```c
op_lda_r_fetch
op_sta_w_wr
op_ror_rw_wr
op_asl_acc_dummy
```

Mnemonic handlers contain the instruction semantics.

### 6.3 Shared handlers

Examples:

```c
mc_fetch
mc_fetch_no_interrupts
mc_dispatch
```

`mc_dispatch` is the final entry that consumes the fetched opcode byte and transfers control to the next instruction flow.

---

## 7. Validation rules

The generator should check:

- `opcodes` has exactly 256 keys.
- Documented opcodes reference valid instruction names.
- Documented opcodes reference valid classes.
- Documented opcodes reference valid addressing modes.
- Every class used by an opcode exists in `nmos6502_class_codegen.json`.
- Every class ends with `mc_dispatch`.
- Addressing handlers do not contain `{mnemonic}`.
- Mnemonic handlers only use `{mnemonic}` where instruction-specific semantics are needed.
- Usually: `len(class.cycles) == opcode.cycles.max + 1`.

---

## 8. Confidence fields

Both class entries and cycle entries may contain:

```json
"ai_certainty": "high",
"ai_comment": "..."
```

Allowed certainty levels:

```text
high
medium
low
```

Use them as review hints, not as source-of-truth data.

Suggested meaning:

| Value | Meaning |
|---|---|
| `high` | The entry was checked against v1 and looks stable. |
| `medium` | The flow is probably correct but should be reviewed carefully. |
| `low` | The entry is uncertain and should not be trusted without manual review. |

---

## 9. Practical design notes

The main design goal is simple:

```text
Opcode metadata stays factual.
Class codegen describes microcode flow.
C handler names are generated from class cycles.
Mnemonic names appear only in semantic handlers.
```

This keeps the opcode file small and stable, while the codegen file controls the emulator-core layout.

The important split is:

```text
addressing/class handler = timing, bus flow, address calculation
mnemonic handler         = instruction semantics
shared handler           = common fetch/dispatch behavior
```

This should allow the v2 core to keep the accuracy of v1, while making the generated microcode table easier to read and maintain.
