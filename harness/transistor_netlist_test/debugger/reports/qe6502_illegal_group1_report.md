# qe6502_v2 Illegal Opcode Report - Group 1

Scope: first easy group of NMOS illegal opcodes. The goal is implementation guidance for `qe6502_v2`, not a full historical naming discussion.

Sources used:
- `perfect6502_debug` controlled traces, fullcycle mode.
- `6502_light` SingleStepTests snippets.
- Current investigator results, used only as a bus-shape hint.

Status summary:
- Fully decoded: 20 / 20
- Still needs work: 0 / 20

## Shared notation

Cycle lists below use SingleStepTests-style memory requests for the instruction body.

- `read(PC)` means opcode fetch/process cycle.
- `read(PC+n)` means operand or next-opcode fetch request, depending on the instruction.
- `zp(x)` means zero-page wrap: `uint8(x)`.
- `set_nz(value)` means update `N` and `Z` from `value`; all other flags are preserved.

## 1. LAX read-family opcodes

Decoded opcodes:

| Opcode | Name used here | Addressing | Cycles | Legal bus analog | Status |
|---:|---|---|---:|---|---|
| `A7` | `LAX zp` | zero page | 3 | `LDA/LDX zp` read shape | complete |
| `AF` | `LAX abs` | absolute | 4 | `LDA/LDX abs` read shape | complete |
| `A3` | `LAX (zp,X)` | indexed indirect | 6 | `LDA (zp,X)` read shape | complete |
| `B7` | `LAX zp,Y` | zero page,Y | 4 | `LDX zp,Y` read shape | complete |

Common operation:

```text
value = final effective read byte
A = value
X = value
set_nz(value)
PC = PC + instruction_size
```

No memory writes. `Y` and `S` are unchanged. `P` changes only in `N/Z`.

Debugger flag spot checks for `A7`, `AF`, `A3`, `B7`:

```text
value=00 -> A=00 X=00 P has Z=1, N=0
value=80 -> A=80 X=80 P has Z=0, N=1
value=7F -> A=7F X=7F P has Z=0, N=0
```

### `A7` - LAX zero page

Instruction bytes:

```text
A7 ll
```

Memory requests:

```text
0: read(PC)       -> A7
1: read(PC+1)     -> ll
2: read(00ll)     -> value
```

State update at the final effective read:

```text
A = value
X = value
set_nz(value)
PC = PC + 2
```

SingleStepTests evidence: `A7` has 3 cycles in all light cases and final `A == X == read(00ll)`.

### `AF` - LAX absolute

Instruction bytes:

```text
AF ll hh
```

Memory requests:

```text
0: read(PC)       -> AF
1: read(PC+1)     -> ll
2: read(PC+2)     -> hh
3: read(hhll)     -> value
```

State update:

```text
A = value
X = value
set_nz(value)
PC = PC + 3
```

SingleStepTests evidence: `AF` has 4 cycles in all light cases and final `A == X == read(hhll)`.

### `A3` - LAX indexed indirect `(zp,X)`

Instruction bytes:

```text
A3 zp
```

Memory requests:

```text
0: read(PC)                    -> A3
1: read(PC+1)                  -> zp
2: read(00zp)                  -> dummy/base zero-page read
3: read(00 uint8(zp + X))      -> ptr_lo
4: read(00 uint8(zp + X + 1))  -> ptr_hi
5: read(ptr_hi:ptr_lo)         -> value
```

State update:

```text
A = value
X = value
set_nz(value)
PC = PC + 2
```

Implementation notes:
- The pointer address wraps in zero page for both low and high bytes.
- The cycle-2 zero-page read is a dummy/base read and does not affect state.

SingleStepTests evidence: `A3` has 6 cycles in all light cases. Example pattern from light tests: `read(zp)`, `read(uint8(zp+X))`, `read(uint8(zp+X+1))`, then read the final 16-bit effective address.

### `B7` - LAX zero page,Y

Instruction bytes:

```text
B7 zp
```

Memory requests:

```text
0: read(PC)                -> B7
1: read(PC+1)              -> zp
2: read(00zp)              -> dummy/base zero-page read
3: read(00 uint8(zp + Y))  -> value
```

State update:

```text
A = value
X = value
set_nz(value)
PC = PC + 2
```

Implementation notes:
- The indexed address wraps in zero page.
- Bus shape matched `B6` uniquely in the investigator output.

SingleStepTests evidence: `B7` has 4 cycles in all light cases. The supplied cases confirm final `A == X == read(uint8(zp+Y))` and `N/Z` from that value.

## 2. Zero-page illegal NOPs

Decoded opcodes:

```text
04 44 64
```

Instruction bytes:

```text
OP zp
```

Memory requests:

```text
0: read(PC)       -> opcode
1: read(PC+1)     -> zp
2: read(00zp)     -> ignored
```

State update:

```text
PC = PC + 2
A, X, Y, S, P unchanged
memory unchanged
```

Implementation notes:
- These are zero-page addressing-only NOPs.
- The zero-page read is performed and ignored.
- No flags are modified.

SingleStepTests evidence: `04`, `44`, and `64` all have 3 cycles in the light set; final architectural state equals initial state except `PC += 2`.

## 3. Zero-page,X illegal NOPs

Decoded opcodes:

```text
14 34 54 74 D4 F4
```

Instruction bytes:

```text
OP zp
```

Memory requests:

```text
0: read(PC)                -> opcode
1: read(PC+1)              -> zp
2: read(00zp)              -> dummy/base zero-page read
3: read(00 uint8(zp + X))  -> ignored
```

State update:

```text
PC = PC + 2
A, X, Y, S, P unchanged
memory unchanged
```

Implementation notes:
- These are zero-page,X addressing-only NOPs.
- The indexed address wraps in zero page.
- Both the base zero-page read and indexed zero-page read are observable bus cycles but are ignored architecturally.

SingleStepTests evidence: all six opcodes have 4 cycles in the light set; final architectural state equals initial state except `PC += 2`.

## 4. Absolute illegal NOP

Decoded opcode:

```text
0C
```

Instruction bytes:

```text
0C ll hh
```

Memory requests:

```text
0: read(PC)       -> 0C
1: read(PC+1)     -> ll
2: read(PC+2)     -> hh
3: read(hhll)     -> ignored
```

State update:

```text
PC = PC + 3
A, X, Y, S, P unchanged
memory unchanged
```

Implementation notes:
- Absolute addressing-only NOP.
- The absolute read is performed and ignored.
- No flags are modified.

SingleStepTests evidence: `0C` has 4 cycles in the light set; final architectural state equals initial state except `PC += 3`.

## 5. Implied illegal NOPs

Decoded opcodes:

```text
1A 3A 5A 7A DA FA
```

Instruction bytes:

```text
OP
```

Memory requests:

```text
0: read(PC)      -> opcode
1: read(PC+1)    -> next byte / fetch-request-style read
```

State update:

```text
PC = PC + 1
A, X, Y, S, P unchanged
memory unchanged
```

Implementation notes:
- These behave as one-byte implied NOPs.
- The second read is observable in SingleStepTests but is not consumed as an operand.
- No flags are modified.

SingleStepTests evidence: all six opcodes have 2 cycles in the light set; final architectural state equals initial state except `PC += 1`.

## Implementation-oriented summary

### Complete opcode list

```text
A7 AF A3 B7
04 44 64
14 34 54 74 D4 F4
0C
1A 3A 5A 7A DA FA
```

### Fully decoded

```text
20 / 20
```

### Still unresolved

```text
0 / 20
```

### Suggested implementation grouping for `qe6502_v2`

- `A7/AF/A3/B7`: use existing legal read addressing machinery, then load both `A` and `X`, set `N/Z` from the loaded value.
- `04/44/64`: use zero-page read addressing machinery, ignore read data, no state changes except `PC`.
- `14/34/54/74/D4/F4`: use zero-page,X read addressing machinery, ignore read data, no state changes except `PC`.
- `0C`: use absolute read addressing machinery, ignore read data, no state changes except `PC`.
- `1A/3A/5A/7A/DA/FA`: one-byte implied NOP, no state changes except `PC`.

### Confidence

All opcodes in this group are complete enough for implementation. No opcode in this group currently needs additional investigation.
