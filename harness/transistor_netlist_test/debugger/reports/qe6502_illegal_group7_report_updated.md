# qe6502_v2 Illegal Opcode Report — Group 7, Updated

## Scope

This report covers the immediate / weird ALU illegal opcodes:

| Opcode | Common mnemonic | Shape |
|---:|---|---|
| `0B` | `ANC #imm` | immediate |
| `2B` | `ANC #imm` | immediate |
| `4B` | `ALR #imm` / `ASR #imm` | immediate |
| `6B` | `ARR #imm` | immediate |
| `8B` | `XAA #imm` / `ANE #imm` | immediate |
| `AB` | `LXA #imm` / `OAL #imm` | immediate |
| `CB` | `AXS #imm` / `SBX #imm` | immediate |
| `EB` | unofficial `SBC #imm` | immediate |

All opcodes in this group share the same bus shape:

```text
cycle 0: R PC    -> opcode
cycle 1: R PC+1  -> immediate operand, then fetch-request for PC+2
```

Therefore the bus behavior is not the hard part. The only meaningful differences are in register and flag side effects.

---

# Confidence summary

| Status | Count |
|---|---:|
| Fully decoded / ready for implementation planning | 5 |
| Risky / needs careful full-test validation | 3 |

Fully decoded / low risk:

```text
0B 2B 4B CB EB
```

Risky / special validation required:

```text
6B 8B AB
```

The risky opcodes are not risky because of bus behavior. They are risky because they are known NMOS unstable/quirky immediate illegal opcodes, and their exact state behavior depends on internal CPU state that is not fully controlled by the current `perfect6502_debug setup` command.

---

# Shared bus template

All opcodes:

```text
cycle 0: R PC    -> opcode
cycle 1: R PC+1  -> immediate operand
```

Common effects:

```text
PC = PC + 2
memory unchanged
S unchanged
Y unchanged
```

`X` changes only for `AB` and `CB`.

---

# Fully decoded / low-risk opcodes

## `0B` and `2B` — ANC #imm

Behavior:

```text
A = A & imm
N/Z from A
C = bit7(A)
```

`X`, `Y`, `S` unchanged.

Implementation status: fully decoded.

---

## `4B` — ALR #imm / ASR #imm

Behavior:

```text
tmp = A & imm
C = bit0(tmp)
A = tmp >> 1
N/Z from A
```

`X`, `Y`, `S` unchanged.

Implementation status: fully decoded.

---

## `CB` — AXS #imm / SBX #imm

Behavior:

```text
base = A & X
result = base - imm

X = result & 0xFF
C = base >= imm
N/Z from X
```

`A`, `Y`, `S` unchanged.

Implementation status: fully decoded.

---

## `EB` — unofficial SBC #imm

Behavior:

```text
A = A SBC imm
N/Z/C/V from SBC
```

`X`, `Y`, `S` unchanged.

Implementation note: reuse the existing qe6502_v2 NMOS `SBC #imm` logic/path. Do not duplicate decimal, overflow, or carry behavior.

Implementation status: fully decoded.

---

# Risky opcodes

## `6B` — ARR #imm

Broad behavior:

```text
tmp = A & imm
A = ROR(tmp) through old carry
```

Common binary-mode flag model:

```text
N/Z from A
C = bit6(A)
V = bit6(A) XOR bit5(A)
```

Risk:

`ARR` is known to have unusual NMOS behavior, especially around flags and decimal mode. The exact behavior should be validated against the full SingleStepTests corpus after implementation.

Important note:

The current `perfect6502_debug setup` command is not a fully reliable final-state oracle for this opcode. It correctly reproduces the simple two-cycle bus flow, but direct setup-based final-state results can differ from SingleStepTests-generated expectations. This strongly suggests that hidden/internal CPU state matters for this opcode.

Implementation recommendation:

```text
Implement the SingleStepTests-compatible ARR model.
Validate full SingleStepTests, especially D-flag cases and C/V behavior.
```

Status: risky, needs careful validation.

---

## `8B` — XAA #imm / ANE #imm

SingleStepTests-compatible deterministic model:

```text
A = (A | 0xEE) & X & imm
N/Z from A
```

`X`, `Y`, `S` unchanged.

Important finding:

`6502_light` and the SingleStepTests-derived samples match the deterministic `0xEE` model.

However, direct `perfect6502_debug setup` tests do not always reproduce this final state. In controlled setup tests, `8B` can behave closer to a state-dependent/latch-dependent variant instead of the `0xEE` model.

Additional experiment:

A set of small programs was tested with one legal instruction before the illegal opcode. The programs were constructed so that the visible architectural state at entry to the illegal opcode was identical. This did **not** force the SingleStepTests `0xEE` behavior. Therefore the difference is not explained simply by one preceding legal instruction leaving an obvious hidden state.

Interpretation:

This is consistent with the known unstable nature of `XAA/ANE`. For qe6502_v2, the practical target should be the deterministic SingleStepTests model, not a direct `perfect6502_debug setup` replay result.

Implementation recommendation:

```text
A = (A | 0xEE) & X & imm
set N/Z from A
```

Then validate against the full SingleStepTests corpus.

Status: risky but implementation target is clear for SingleStepTests compatibility.

---

## `AB` — LXA #imm / OAL #imm

SingleStepTests-compatible deterministic model:

```text
value = (A | 0xEE) & imm
A = value
X = value
N/Z from value
```

Important finding:

`6502_light` and the SingleStepTests-derived samples match the deterministic `0xEE` model.

Direct `perfect6502_debug setup` tests often produce a different final state, commonly closer to:

```text
A = X = A & imm
```

rather than:

```text
A = X = (A | 0xEE) & imm
```

Additional experiment:

Small programs were built in the form:

```text
legal opcode + operands
AB imm
```

The illegal opcode was always placed at the same address, with the same operand, and the visible architectural state at entry to `AB` was forced to be the same across all programs.

Result:

The preceding legal instruction did not make `AB` reproduce the SingleStepTests `0xEE` model. The direct debugger result remained stable in the alternate behavior.

Interpretation:

This strongly suggests that `AB` depends on hidden/internal CPU state that is not initialized by the current debugger `setup` path in the same way as the SingleStepTests generator.

Implementation recommendation for qe6502_v2:

```text
value = (A | 0xEE) & imm
A = value
X = value
set N/Z from value
```

Then validate against the full SingleStepTests corpus.

Status: risky but implementation target is clear for SingleStepTests compatibility.

---

# Implementation table

| Opcode | Operation | A changes | X changes | Flags | Status |
|---:|---|---:|---:|---|---|
| `0B` | `A = A & imm; C = bit7(A)` | yes | no | `N/Z` from `A`, `C=N` | clear |
| `2B` | `A = A & imm; C = bit7(A)` | yes | no | `N/Z` from `A`, `C=N` | clear |
| `4B` | `A = (A & imm) >> 1` | yes | no | `C` from bit0 of `A&imm`, `N/Z` from result | clear |
| `6B` | `A = ROR(A & imm)` plus special flags | yes | no | `N/Z`, special `C/V`; validate decimal cases | risky |
| `8B` | `A = (A | EE) & X & imm` | yes | no | `N/Z` from `A` | risky / deterministic target |
| `AB` | `A = X = (A | EE) & imm` | yes | yes | `N/Z` from value | risky / deterministic target |
| `CB` | `X = (A & X) - imm` | no | yes | `C` from compare, `N/Z` from `X` | clear |
| `EB` | `A = A SBC imm` | yes | no | normal `SBC` flags | clear |

---

# Debugger/setup limitation note

The current `perfect6502_debug setup` command is excellent for bus-flow experiments and for many normal/illegal opcodes. However, for `6B`, `8B`, and `AB`, it should not be treated as a final-state oracle.

Observed:

```text
cycles/bus flow match SingleStepTests
PC/Y/S generally match
A/X/P may diverge for 6B/8B/AB
```

This is probably because `setup` restores architectural state and memory, but does not reproduce all hidden/internal transistor-level state used by these unstable opcodes.

For implementation work, use:

```text
SingleStepTests expected final state as oracle
perfect6502_debug setup as bus-flow helper only
```

for these three opcodes.

---

# Suggested qe6502_v2 implementation order

1. `0B`, `2B`
2. `4B`
3. `CB`
4. `EB` as alias/reuse of `E9`
5. `8B`, `AB` using the deterministic `0xEE` model
6. `6B` last, with special attention to flags and decimal mode

---

# Summary

| Category | Count |
|---|---:|
| Opcodes investigated | 8 |
| Fully decoded / low risk | 5 |
| Risky / needs careful full-test validation | 3 |

Fully decoded / low risk:

```text
0B 2B 4B CB EB
```

Risky but with clear SingleStepTests-compatible target behavior:

```text
8B AB
```

Risky and requiring special flag/decimal validation:

```text
6B
```

Group 7 is mostly decoded. The remaining risk is not bus behavior; it is unstable/internal-state-dependent final state behavior for `6B/8B/AB`.
