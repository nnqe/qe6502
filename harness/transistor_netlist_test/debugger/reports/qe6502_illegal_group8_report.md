# qe6502_v2 Illegal Opcode Report — Group 8

## Scope

This report covers the KIL/JAM illegal opcodes:

| Opcode |
|---:|
| `02` |
| `12` |
| `22` |
| `32` |
| `42` |
| `52` |
| `62` |
| `72` |
| `92` |
| `B2` |
| `D2` |
| `F2` |

These opcodes are a special class. They do not behave like normal instructions that eventually reach a next-opcode fetch. They enter a jammed/stopped bus pattern.

## Confidence summary

| Status | Count |
|---|---:|
| Fully decoded as JAM/KIL class | 12 |
| Needs more work for normal instruction behavior | 0 |

All opcodes in this group are considered fully identified as KIL/JAM behavior.

They should **not** be implemented as normal instructions with a normal completion/fetch-request point.

---

# Important interpretation rule

SingleStepTests show 11 cycles for these opcodes:

```text
02 12 22 32 42 52 62 72 92 B2 D2 F2 -> 11 cycles
```

This should not be interpreted as:

```text
instruction length = 11 cycles
```

The better interpretation is:

```text
the test records an 11-cycle observation window of a jammed CPU state
```

The opcode does not normally complete after those 11 cycles. The processor remains jammed.

---

# Observed common bus pattern

The examples from SingleStepTests and debugger traces show the same structural pattern.

For opcode `02`:

```text
cycle 0: R PC      -> opcode
cycle 1: R PC+1    -> next byte
cycle 2: R FFFF
cycle 3: R FFFE
cycle 4: R FFFE
cycle 5: R FFFF
cycle 6: R FFFF
cycle 7: R FFFF
cycle 8: R FFFF
cycle 9: R FFFF
cycle 10: R FFFF
...
```

The exact observed values at `FFFE` and `FFFF` come from memory, but the structural address pattern is stable.

The processor does not reach a normal next-instruction fetch process.

---

# State behavior

For the sampled KIL/JAM cases:

```text
PC = initial PC + 1
A unchanged
X unchanged
Y unchanged
S unchanged
P unchanged
RAM unchanged
```

The `PC + 1` result should be interpreted carefully. It is the observed state after the fixed test trace window, not a normal completed-instruction PC.

---

# Interrupt behavior

Debugger experiments showed:

```text
NMI does not escape KIL/JAM
IRQ does not escape KIL/JAM
```

Observed behavior:

```text
no interrupt stack sequence
no vector entry sequence
no normal instruction completion
jammed bus pattern continues
```

This applies to the tested KIL/JAM behavior and matches the conclusion that these opcodes are not normal instructions.

---

# Implementation guidance for qe6502_v2

These opcodes should be implemented as a special jammed state.

Recommended model:

```text
on KIL/JAM opcode:
    enter jammed CPU state
    do not continue normal opcode dispatch
    do not complete as a normal instruction
    do not accept IRQ/NMI as a normal escape path
```

The existing qe6502_v2 trap/jam concept should be used if available.

## Important

Do not implement these as:

```text
11-cycle instructions
```

The `11` from SingleStepTests is only the length of the recorded observation window.

---

# Bus behavior to emulate

If qe6502_v2 needs to provide cycle-level bus behavior while jammed, the observed KIL/JAM bus pattern should be matched as far as the test harness requires.

Minimum implementation requirement:

```text
the CPU enters jammed state and does not fetch/execute further opcodes normally
```

Stronger cycle-observable behavior:

```text
cycle 0: fetch KIL opcode
cycle 1: read PC+1
then enter repeated jammed bus reads around FFFE/FFFF / FFFF pattern
```

The exact long-term repeated address pattern should be verified against the existing qe6502_v2 trap behavior before implementation changes. If the current v2 trap implementation already passes SingleStepTests for legal/known trap behavior, reuse it.

---

# Relationship to investigator results

The illegal-opcode investigator reports these opcodes as:

```text
max_cycles = 11
best_min_bus_prefix = 1
large legal candidate group
```

This is expected.

Only cycle 0 looks like many legal instructions because cycle 0 is just opcode fetch. Starting from cycle 1, KIL/JAM diverges from all normal legal opcode flows.

This is a useful classifier:

```text
best_min_bus_prefix = 1 + max_cycles = 11 -> likely jam/special class
```

---

# Per-opcode table

| Opcode | Class | Normal completion | IRQ/NMI escape observed | Notes |
|---:|---|---|---|---|
| `02` | KIL/JAM | no | no | stable jam pattern |
| `12` | KIL/JAM | no | no | stable jam pattern expected |
| `22` | KIL/JAM | no | no | stable jam pattern expected |
| `32` | KIL/JAM | no | no | stable jam pattern expected |
| `42` | KIL/JAM | no | no | stable jam pattern expected |
| `52` | KIL/JAM | no | no | stable jam pattern expected |
| `62` | KIL/JAM | no | no | stable jam pattern expected |
| `72` | KIL/JAM | no | no | stable jam pattern expected |
| `92` | KIL/JAM | no | no | stable jam pattern expected |
| `B2` | KIL/JAM | no | no | stable jam pattern expected |
| `D2` | KIL/JAM | no | no | stable jam pattern expected |
| `F2` | KIL/JAM | no | no | stable jam pattern expected |

---

# Verification notes

Direct debugger testing was performed on representative KIL/JAM behavior, especially `02`, with NMI and IRQ. It showed no interrupt escape and no normal instruction completion.

SingleStepTests light/full cycle-count data show all KIL/JAM opcodes with the same fixed 11-cycle observation window.

---

# Summary

| Category | Count |
|---|---:|
| Opcodes investigated | 12 |
| Fully decoded as KIL/JAM | 12 |
| Needs more investigation | 0 |

Group 8 is fully classified as KIL/JAM. The main implementation warning is to avoid treating the 11-cycle SingleStepTests trace length as a normal instruction duration.
