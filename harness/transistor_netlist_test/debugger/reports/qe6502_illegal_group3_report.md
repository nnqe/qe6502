# qe6502_v2 Illegal Opcode Report - Group 3

Scope: NMOS illegal read-modify-write (RMW) opcode families. The goal is implementation guidance for `qe6502_v2`.

Sources used:
- `perfect6502_debug` controlled traces, fullcycle mode.
- `6502_light` SingleStepTests snippets.
- Current investigator results, used only as a bus-shape hint.

Status summary:
- Fully decoded: 24 / 24
- Still needs work: 0 / 24

Important implementation note:
- For `RRA` and `ISC`, this report describes the illegal opcode as a composition with the existing `ADC` / `SBC` operation. Decimal-mode and overflow/carry behavior should be delegated to the already-correct NMOS `ADC` / `SBC` implementation in `qe6502_v2`, not reimplemented ad hoc.

## Shared notation

- `read(PC)` means opcode fetch/process cycle.
- `read(PC+n)` means operand fetch.
- `zp(x)` means zero-page wrap: `uint8(x)`.
- `base = hi:lo` means a 16-bit address built from fetched bytes.
- `intermediate = hi:uint8(lo + X)` means the low-byte addition is performed without carrying into the high byte.
- `effective = base + X` means the corrected 16-bit address.
- `old` means the original byte read from the effective address.
- `mod` means the modified byte written back by the RMW operation.
- All group-3 opcodes first write `old` back, then write `mod`.

The SingleStepTests cycle lists end at the final memory write. In `perfect6502_debug` fullcycle view, some combined RMW+ALU register effects are easiest to observe at the following fetch-process boundary. The architectural final state is the one shown in SingleStepTests and summarized below.

## Decoded opcode matrix

| Family | Operation | zp | zp,X | abs | abs,X |
|---|---|---:|---:|---:|---:|
| `SLO` | `ASL mem`, then `ORA A,mem` | `07` | `17` | `0F` | `1F` |
| `RLA` | `ROL mem`, then `AND A,mem` | `27` | `37` | `2F` | `3F` |
| `SRE` | `LSR mem`, then `EOR A,mem` | `47` | `57` | `4F` | `5F` |
| `RRA` | `ROR mem`, then `ADC A,mem` | `67` | `77` | `6F` | `7F` |
| `DCP` | `DEC mem`, then `CMP A,mem` | `C7` | `D7` | `CF` | `DF` |
| `ISC` | `INC mem`, then `SBC A,mem` | `E7` | `F7` | `EF` | `FF` |

All 24 opcodes are read-modify-write instructions. There are no stack accesses and no control-flow changes beyond `PC += instruction_size`.

## Addressing / bus templates

### Zero page RMW

Applies to:

```text
07 27 47 67 C7 E7
```

Instruction bytes:

```text
OP zp
```

Memory requests:

```text
0: read(PC)       -> opcode
1: read(PC+1)     -> zp
2: read(00zp)     -> old
3: write(00zp)    <- old
4: write(00zp)    <- mod
```

State / memory:

```text
memory[00zp] = mod
PC = PC + 2
```

SingleStepTests evidence: all six zero-page RMW illegal families have 5 cycles in `6502_light`.

### Zero page,X RMW

Applies to:

```text
17 37 57 77 D7 F7
```

Instruction bytes:

```text
OP zp
```

Memory requests:

```text
0: read(PC)                    -> opcode
1: read(PC+1)                  -> zp
2: read(00zp)                  -> dummy/base zero-page read
3: read(00 uint8(zp + X))      -> old
4: write(00 uint8(zp + X))     <- old
5: write(00 uint8(zp + X))     <- mod
```

State / memory:

```text
memory[00 uint8(zp + X)] = mod
PC = PC + 2
```

Implementation notes:
- The indexed address wraps in zero page.
- The cycle-2 zero-page read is a dummy/base read and is ignored.

SingleStepTests evidence: all six zero-page,X RMW illegal families have 6 cycles in `6502_light`.

### Absolute RMW

Applies to:

```text
0F 2F 4F 6F CF EF
```

Instruction bytes:

```text
OP ll hh
```

Memory requests:

```text
0: read(PC)       -> opcode
1: read(PC+1)     -> ll
2: read(PC+2)     -> hh
3: read(hhll)     -> old
4: write(hhll)    <- old
5: write(hhll)    <- mod
```

State / memory:

```text
memory[hhll] = mod
PC = PC + 3
```

SingleStepTests evidence: all six absolute RMW illegal families have 6 cycles in `6502_light`.

### Absolute,X RMW

Applies to:

```text
1F 3F 5F 7F DF FF
```

Instruction bytes:

```text
OP ll hh
```

Address construction:

```text
base = hh:ll
intermediate = hh:uint8(ll + X)
effective = base + X
```

Memory requests:

```text
0: read(PC)             -> opcode
1: read(PC+1)           -> ll
2: read(PC+2)           -> hh
3: read(intermediate)   -> ignored intermediate byte
4: read(effective)      -> old
5: write(effective)     <- old
6: write(effective)     <- mod
```

State / memory:

```text
memory[effective] = mod
PC = PC + 3
```

Implementation notes:
- This is always 7 cycles in the SingleStepTests light data.
- The intermediate read exists regardless of whether a page is crossed.
- If no page is crossed, `intermediate == effective`, so the same address is read twice.
- If a page is crossed, cycle 3 reads the no-carry address and cycle 4 reads the corrected effective address.

SingleStepTests evidence: all six absolute,X RMW illegal families have 7 cycles in `6502_light`.

## Operation families

### 1. `SLO` - ASL then ORA

Opcodes:

```text
07 17 0F 1F
```

Operation:

```text
old = read(effective)
mod = uint8(old << 1)
C = bit7(old)
memory[effective] = mod
A = A | mod
set_nz(A)
```

Flags:

```text
C from old bit 7
N/Z from final A
V, D, I unchanged
```

Representative evidence:

```text
07 zp example from perfect6502_debug:
old=81, A=10
writes: old 81, then mod 02
final: A=12, C=1, N=0, Z=0
```

SingleStepTests evidence: `07`, `17`, `0F`, and `1F` all write `old` then `old << 1`, and final `A` equals `initial A | mod`.

### 2. `RLA` - ROL then AND

Opcodes:

```text
27 37 2F 3F
```

Operation:

```text
old_c = C
old = read(effective)
mod = uint8((old << 1) | old_c)
C = bit7(old)
memory[effective] = mod
A = A & mod
set_nz(A)
```

Flags:

```text
C from old bit 7
N/Z from final A
V, D, I unchanged
```

SingleStepTests evidence: examples show write-back of `ROL(old, old C)`, and final `A` equals `initial A & mod`.

### 3. `SRE` - LSR then EOR

Opcodes:

```text
47 57 4F 5F
```

Operation:

```text
old = read(effective)
mod = old >> 1
C = bit0(old)
memory[effective] = mod
A = A ^ mod
set_nz(A)
```

Flags:

```text
C from old bit 0
N/Z from final A
V, D, I unchanged
```

SingleStepTests evidence: examples show write-back of `old >> 1`, and final `A` equals `initial A ^ mod`.

### 4. `RRA` - ROR then ADC

Opcodes:

```text
67 77 6F 7F
```

Operation:

```text
old_c = C
old = read(effective)
mod = uint8((old >> 1) | (old_c << 7))
C = bit0(old)              ; carry produced by ROR
memory[effective] = mod
A = ADC(A, mod, C)         ; use normal NMOS ADC semantics
```

Flags:

```text
C initially becomes old bit 0 from ROR, then ADC produces final C/V/N/Z
D flag controls ADC exactly as in legal ADC
D, I unchanged
```

Implementation note:
- Do not duplicate ADC flag logic. Use the existing NMOS ADC operation after the ROR-modified memory value is produced.
- The ADC carry input is the carry produced by the ROR step.

SingleStepTests evidence: examples show write-back of `ROR(old, initial C)`, and final `A/P` follow ADC with the modified byte.

### 5. `DCP` - DEC then CMP

Opcodes:

```text
C7 D7 CF DF
```

Operation:

```text
old = read(effective)
mod = uint8(old - 1)
memory[effective] = mod
compare = A - mod
C = (A >= mod)
Z = (A == mod)
N = bit7(compare)
```

State:

```text
A unchanged
X/Y/S unchanged
```

Flags:

```text
N/Z/C from CMP(A, mod)
V, D, I unchanged
```

SingleStepTests evidence: examples show write-back of `old - 1`, with `A` unchanged and final flags matching compare against `mod`.

### 6. `ISC` / `ISB` - INC then SBC

Opcodes:

```text
E7 F7 EF FF
```

Operation:

```text
old = read(effective)
mod = uint8(old + 1)
memory[effective] = mod
A = SBC(A, mod, C)         ; use normal NMOS SBC semantics
```

Flags:

```text
Final C/V/N/Z from SBC
D flag controls SBC exactly as in legal SBC
D, I unchanged
```

Implementation note:
- Do not duplicate SBC flag logic. Use the existing NMOS SBC operation after the INC-modified memory value is produced.
- `ISC` and `ISB` are aliases; this report uses `ISC`.

SingleStepTests evidence: examples show write-back of `old + 1`, and final `A/P` follow SBC with the modified byte.

## Implementation grouping suggestion

The most compact implementation approach in `qe6502_v2` is to reuse the existing legal RMW addressing templates and swap only the post-modify operation:

```text
RMW address template:
  zp      -> 5 cycles
  zp,X    -> 6 cycles
  abs     -> 6 cycles
  abs,X   -> 7 cycles

RMW memory sequence:
  read old
  write old
  write mod

Illegal post-modify operation:
  SLO: ASL result then ORA A
  RLA: ROL result then AND A
  SRE: LSR result then EOR A
  RRA: ROR result then ADC A
  DCP: DEC result then CMP A
  ISC: INC result then SBC A
```

Where possible, the operation part should call the same helpers used by legal `ASL/ROL/LSR/ROR`, `ORA/AND/EOR`, `ADC/SBC`, and `CMP`, so decimal-mode and flag behavior remain consistent with the existing implementation.

## Summary

- Fully decoded: 24 / 24
- Still needs work: 0 / 24

Decoded opcodes:

```text
07 17 0F 1F
27 37 2F 3F
47 57 4F 5F
67 77 6F 7F
C7 D7 CF DF
E7 F7 EF FF
```

No unresolved behavior remains for this group at the level needed for implementation. The only caution is to delegate `RRA` and `ISC` arithmetic to the existing NMOS `ADC` / `SBC` helpers rather than reimplementing decimal and overflow logic locally.
