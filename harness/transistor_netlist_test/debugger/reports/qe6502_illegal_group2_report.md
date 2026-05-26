# qe6502_v2 Illegal Opcode Report - Group 2

Scope: LAX opcodes with conditional page-cross timing. The goal is implementation guidance for `qe6502_v2`.

Sources used:
- `perfect6502_debug` controlled traces, fullcycle mode.
- `6502_light` SingleStepTests snippets.
- Current investigator results, used only as a bus-shape hint.

Status summary:
- Fully decoded: 2 / 2
- Still needs work: 0 / 2

## Shared notation

- `read(PC)` means opcode fetch/process cycle.
- `read(PC+n)` means operand fetch.
- `zp(x)` means zero-page wrap: `uint8(x)`.
- `base = hi:lo` means a 16-bit address built from fetched bytes.
- `intermediate = hi:uint8(lo + index)` means the low-byte addition is performed without carrying into the high byte.
- `effective = base + index` means the final corrected 16-bit address.
- `set_nz(value)` means update `N` and `Z` from `value`; all other flags are preserved.

Both opcodes below are read-only. No memory writes occur. `Y` and `S` are unchanged.

## Decoded opcodes

| Opcode | Name used here | Addressing | Cycles | Legal bus analog | Status |
|---:|---|---|---:|---|---|
| `B3` | `LAX (zp),Y` | indirect indexed | 5 no-cross / 6 cross | `LDA (zp),Y` read shape | complete |
| `BF` | `LAX abs,Y` | absolute,Y | 4 no-cross / 5 cross | `LDA abs,Y` read shape | complete |

Common operation:

```text
value = final effective read byte
A = value
X = value
set_nz(value)
PC = PC + instruction_size
```

Debugger flag spot checks for both `B3` and `BF`:

```text
value=00 -> A=00 X=00, Z=1, N=0
value=80 -> A=80 X=80, Z=0, N=1
value=7F -> A=7F X=7F, Z=0, N=0
value=C3 -> A=C3 X=C3, Z=0, N=1
```

## 1. `B3` - LAX indirect indexed `(zp),Y`

Instruction bytes:

```text
B3 zp
```

Base pointer construction:

```text
ptr_lo = read(00zp)
ptr_hi = read(00 uint8(zp + 1))
base   = ptr_hi:ptr_lo
effective = base + Y
```

### No page-cross case

Condition:

```text
uint8(ptr_lo + Y) >= ptr_lo
```

Memory requests:

```text
0: read(PC)                       -> B3
1: read(PC+1)                     -> zp
2: read(00zp)                     -> ptr_lo
3: read(00 uint8(zp + 1))         -> ptr_hi
4: read(ptr_hi:uint8(ptr_lo + Y)) -> value
```

State update at cycle 4:

```text
A = value
X = value
set_nz(value)
PC = PC + 2
```

Controlled debugger example:

```text
zp=80, Y=05
mem[0080]=00, mem[0081]=12
read(1205)=C3

cycles:
0: read(0200) -> B3
1: read(0201) -> 80
2: read(0080) -> 00
3: read(0081) -> 12
4: read(1205) -> C3

final: A=C3, X=C3, PC=0202, P has N=1/Z=0
```

SingleStepTests evidence: light cases include 5-cycle no-cross examples. Example shape:

```text
read(PC), read(PC+1), read(zp), read(zp+1), read(base+Y)
```

### Page-cross case

Condition:

```text
ptr_lo + Y > 0xFF
```

Memory requests:

```text
0: read(PC)                              -> B3
1: read(PC+1)                            -> zp
2: read(00zp)                            -> ptr_lo
3: read(00 uint8(zp + 1))                -> ptr_hi
4: read(ptr_hi:uint8(ptr_lo + Y))        -> intermediate byte, ignored
5: read(uint8(ptr_hi + 1):uint8(ptr_lo + Y)) -> value
```

State update at cycle 5:

```text
A = value
X = value
set_nz(value)
PC = PC + 2
```

Controlled debugger example:

```text
zp=80, Y=01
mem[0080]=FF, mem[0081]=12
intermediate = 1200
corrected effective = 1300
read(1200)=99, read(1300)=C3

cycles:
0: read(0200) -> B3
1: read(0201) -> 80
2: read(0080) -> FF
3: read(0081) -> 12
4: read(1200) -> 99   ; intermediate no-carry address, ignored
5: read(1300) -> C3   ; corrected effective read

final: A=C3, X=C3, PC=0202, P has N=1/Z=0
```

Implementation notes:

- `B3` follows the legal `LDA (zp),Y` page-cross bus shape.
- The final operation is LAX: load both `A` and `X` from the final effective read.
- The intermediate page-cross read is ignored and must not affect `A`, `X`, or flags.
- Zero-page pointer high byte wraps: `read(00 uint8(zp + 1))`.

## 2. `BF` - LAX absolute,Y

Instruction bytes:

```text
BF ll hh
```

Base address construction:

```text
base = hh:ll
effective = base + Y
```

### No page-cross case

Condition:

```text
uint8(ll + Y) >= ll
```

Memory requests:

```text
0: read(PC)                    -> BF
1: read(PC+1)                  -> ll
2: read(PC+2)                  -> hh
3: read(hh:uint8(ll + Y))      -> value
```

State update at cycle 3:

```text
A = value
X = value
set_nz(value)
PC = PC + 3
```

Controlled debugger example:

```text
ll=00, hh=12, Y=05
read(1205)=C3

cycles:
0: read(0200) -> BF
1: read(0201) -> 00
2: read(0202) -> 12
3: read(1205) -> C3

final: A=C3, X=C3, PC=0203, P has N=1/Z=0
```

SingleStepTests evidence: light cases include 4-cycle no-cross examples. Example shape:

```text
read(PC), read(PC+1), read(PC+2), read(base+Y)
```

### Page-cross case

Condition:

```text
ll + Y > 0xFF
```

Memory requests:

```text
0: read(PC)                         -> BF
1: read(PC+1)                       -> ll
2: read(PC+2)                       -> hh
3: read(hh:uint8(ll + Y))           -> intermediate byte, ignored
4: read(uint8(hh + 1):uint8(ll + Y)) -> value
```

State update at cycle 4:

```text
A = value
X = value
set_nz(value)
PC = PC + 3
```

Controlled debugger example:

```text
ll=FF, hh=12, Y=01
intermediate = 1200
corrected effective = 1300
read(1200)=99, read(1300)=C3

cycles:
0: read(0200) -> BF
1: read(0201) -> FF
2: read(0202) -> 12
3: read(1200) -> 99   ; intermediate no-carry address, ignored
4: read(1300) -> C3   ; corrected effective read

final: A=C3, X=C3, PC=0203, P has N=1/Z=0
```

Implementation notes:

- `BF` follows the legal `LDA abs,Y` page-cross bus shape.
- The final operation is LAX: load both `A` and `X` from the final effective read.
- The intermediate page-cross read is ignored and must not affect `A`, `X`, or flags.

## Implementation summary for qe6502_v2

### `B3` behavior

```text
operand = read(PC+1)
ptr_lo  = read(00 operand)
ptr_hi  = read(00 uint8(operand + 1))
base    = ptr_hi:ptr_lo
addr0   = ptr_hi:uint8(ptr_lo + Y)

if ptr_lo + Y crosses page:
    ignored = read(addr0)
    value   = read(base + Y)
else:
    value   = read(addr0)

A = value
X = value
set_nz(value)
PC = PC + 2
```

### `BF` behavior

```text
ll    = read(PC+1)
hh    = read(PC+2)
base  = hh:ll
addr0 = hh:uint8(ll + Y)

if ll + Y crosses page:
    ignored = read(addr0)
    value   = read(base + Y)
else:
    value   = read(addr0)

A = value
X = value
set_nz(value)
PC = PC + 3
```

## Final status

Fully decoded:

```text
B3 BF
```

Still needs work:

```text
none
```

Summary:

```text
Fully decoded: 2 / 2
Still needs work: 0 / 2
```
