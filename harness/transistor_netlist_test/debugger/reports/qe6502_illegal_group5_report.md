# qe6502_v2 Illegal Opcode Report — Group 5

## Scope

This report covers the absolute,Y illegal RMW opcodes:

| Opcode | Mnemonic | Family |
|---:|---|---|
| `1B` | `SLO abs,Y` | `ASL` + `ORA` |
| `3B` | `RLA abs,Y` | `ROL` + `AND` |
| `5B` | `SRE abs,Y` | `LSR` + `EOR` |
| `7B` | `RRA abs,Y` | `ROR` + `ADC` |
| `DB` | `DCP abs,Y` | `DEC` + `CMP` |
| `FB` | `ISC abs,Y` | `INC` + `SBC` |

These are the absolute,Y variants of the RMW+ALU illegal families.

## Confidence summary

| Status | Count |
|---|---:|
| Fully decoded | 6 |
| Needs more work | 0 |

All opcodes in this group are considered fully decoded for qe6502_v2 implementation purposes.

The main implementation risk is arithmetic reuse: `RRA` and `ISC` should reuse the existing NMOS `ADC`/`SBC` behavior, including decimal/overflow/carry handling.

---

# Cycle count

All opcodes in this group are:

```text
7 cycles, constant
```

This is true for both page-cross and no-page-cross cases.

Unlike legal absolute,Y read instructions, these RMW illegal opcodes do not become shorter when there is no page crossing. They always perform the full RMW sequence.

---

# Addressing behavior

Let:

```text
lo = read(PC + 1)
hi = read(PC + 2)

intermediate_addr = (hi << 8) | uint8(lo + Y)
addr              = ((hi + carry(lo + Y)) << 8) | uint8(lo + Y)
old               = read(addr)
```

The intermediate address uses the low-byte addition without carrying into the high byte.

If there is no page crossing:

```text
intermediate_addr == addr
```

If there is page crossing:

```text
intermediate_addr != addr
```

---

# Bus template

The bus sequence is always:

```text
cycle 0: R PC                -> opcode
cycle 1: R PC+1              -> low operand
cycle 2: R PC+2              -> high operand
cycle 3: R intermediate_addr -> intermediate/dummy read
cycle 4: R addr              -> old memory value
cycle 5: W addr              <= old memory value
cycle 6: W addr              <= modified value, then fetch-request for PC+3
```

For no-page-cross cases, cycles 3 and 4 read the same address.

For page-cross cases, cycle 3 reads the no-carry intermediate address and cycle 4 reads the corrected address.

---

# Example: no page crossing

Example shape from `1B`:

```text
PC      = BDEA
opcode  = 1B
lo      = 01
hi      = AB
Y       = 8B

base    = AB01
addr    = AB8C
```

Observed cycles:

```text
R BDEA -> 1B
R BDEB -> 01
R BDEC -> AB
R AB8C -> old/dummy
R AB8C -> old
W AB8C <= old
W AB8C <= modified
```

Because there is no page crossing, the intermediate address and corrected address are the same.

---

# Example: page crossing

Example shape from `1B`:

```text
PC                = 77B5
opcode            = 1B
lo                = 43
hi                = 66
Y                 = D7

base              = 6643
intermediate_addr = 661A
addr              = 671A
```

Observed cycles:

```text
R 77B5 -> 1B
R 77B6 -> 43
R 77B7 -> 66
R 661A -> intermediate/dummy
R 671A -> old
W 671A <= old
W 671A <= modified
```

The corrected high byte is used from the RMW read onward.

---

# Shared RMW behavior

Every opcode in this group performs:

```text
old = read(addr)
write(addr, old)
modified = modify(old)
write(addr, modified)
then apply family ALU operation using modified
```

The final memory value is `modified`.

---

# Per-family operation semantics

## `1B` — SLO abs,Y

```text
modified = old << 1
C = old bit 7
A = A | modified
N/Z from A
```

Memory receives `modified`.

## `3B` — RLA abs,Y

```text
old_carry = C
modified = (old << 1) | old_carry
C = old bit 7
A = A & modified
N/Z from A
```

Memory receives `modified`.

## `5B` — SRE abs,Y

```text
modified = old >> 1
C = old bit 0
A = A ^ modified
N/Z from A
```

Memory receives `modified`.

## `7B` — RRA abs,Y

```text
old_carry = C
modified = (old >> 1) | (old_carry << 7)
C = old bit 0
A = A ADC modified
N/Z/C/V from ADC result
```

Memory receives `modified`.

Implementation note: use the existing qe6502_v2 NMOS ADC path/helper. Do not duplicate decimal/overflow/carry behavior.

## `DB` — DCP abs,Y

```text
modified = old - 1
compare = A - modified
C = A >= modified
N/Z from compare result
A unchanged
```

Memory receives `modified`.

## `FB` — ISC abs,Y

```text
modified = old + 1
A = A SBC modified
N/Z/C/V from SBC result
```

Memory receives `modified`.

Implementation note: use the existing qe6502_v2 NMOS SBC path/helper. Do not duplicate decimal/overflow/carry behavior.

---

# Per-opcode implementation table

| Opcode | Modify | ALU step | A changes | Memory changes | Flags |
|---:|---|---|---|---|---|
| `1B` | `ASL` | `ORA` | yes | yes | `C` from ASL, `N/Z` from final `A` |
| `3B` | `ROL` | `AND` | yes | yes | `C` from ROL, `N/Z` from final `A` |
| `5B` | `LSR` | `EOR` | yes | yes | `C` from LSR, `N/Z` from final `A` |
| `7B` | `ROR` | `ADC` | yes | yes | ADC flags after ROR memory modification |
| `DB` | `DEC` | `CMP` | no | yes | compare flags from `A - modified` |
| `FB` | `INC` | `SBC` | yes | yes | SBC flags |

---

# Implementation guidance for qe6502_v2

This group should be implemented as an absolute,Y RMW addressing form with a fixed 7-cycle sequence.

Do not model it as a legal absolute,Y read form plus a conditional extra cycle. The illegal RMW form always has:

```text
intermediate read
corrected/effective read
old-value write
modified-value write
```

Even in no-page-cross cases, the intermediate and effective address are equal, so the same address is read twice before the writes.

The useful implementation decomposition is:

```text
1. Fetch low operand.
2. Fetch high operand.
3. Read intermediate no-carry address.
4. Read corrected effective address.
5. Write old value.
6. Write modified value and apply family ALU operation.
7. Fetch next opcode at PC+3.
```

---

# Verification notes

The debugger traces and `6502_light` samples agree on these structural facts:

- All six opcodes are 7-cycle constant.
- No-page-cross cases read the same target address twice before writing.
- Page-cross cases read an intermediate no-carry address, then the corrected target address.
- All forms perform old-value write followed by modified-value write.
- `DCP` does not change `A`.
- `SLO/RLA/SRE/RRA/ISC` change `A`.
- The final memory byte is the modified value.

---

# Summary

| Category | Count |
|---|---:|
| Opcodes investigated | 6 |
| Fully decoded | 6 |
| Needs more investigation | 0 |

Group 5 is fully decoded and ready to guide qe6502_v2 implementation.
