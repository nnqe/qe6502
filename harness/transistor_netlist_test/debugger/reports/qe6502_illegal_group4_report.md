# qe6502_v2 Illegal Opcode Report — Group 4

## Scope

This report covers the indexed-indirect and indirect-indexed illegal RMW opcodes:

| Mode | Opcodes |
|---|---|
| `(zp,X)` | `03 23 43 63 C3 E3` |
| `(zp),Y` | `13 33 53 73 D3 F3` |

These opcodes are the indirect-addressing variants of the same RMW+ALU families described in Group 3:

| Opcode family | Operation |
|---|---|
| `SLO` | `ASL memory`, then `A = A OR modified_value` |
| `RLA` | `ROL memory`, then `A = A AND modified_value` |
| `SRE` | `LSR memory`, then `A = A XOR modified_value` |
| `RRA` | `ROR memory`, then `A = A ADC modified_value` |
| `DCP` | `DEC memory`, then compare `A - modified_value` |
| `ISC` | `INC memory`, then `A = A SBC modified_value` |

## Confidence summary

| Status | Count |
|---|---:|
| Fully decoded | 12 |
| Needs more work | 0 |

All opcodes in this group are considered fully decoded for qe6502_v2 implementation purposes.

The main implementation risk is not the bus sequence. It is preserving the exact existing NMOS helper behavior for `ADC`/`SBC`, especially decimal mode and overflow/carry behavior, when implementing `RRA` and `ISC`.

---

# Shared operation semantics

Each opcode performs a read-modify-write memory operation:

1. Read the target memory byte.
2. Write the original byte back to the same address.
3. Compute the modified byte.
4. Write the modified byte to the same address.
5. Apply the opcode-family ALU operation using the modified byte.

The final memory value is the modified byte.

## Family details

### SLO

```text
modified = old << 1
C = old bit 7
A = A | modified
N/Z from A
```

### RLA

```text
modified = (old << 1) | old_C
C = old bit 7
A = A & modified
N/Z from A
```

### SRE

```text
modified = old >> 1
C = old bit 0
A = A ^ modified
N/Z from A
```

### RRA

```text
modified = (old >> 1) | (old_C << 7)
C = old bit 0
A = A + modified + C   // use existing ADC/NMOS decimal behavior
N/Z/C/V from ADC result
```

### DCP

```text
modified = old - 1
compare = A - modified
C = A >= modified
N/Z from compare result
A unchanged
```

### ISC

```text
modified = old + 1
A = A - modified - (1 - C)   // use existing SBC/NMOS decimal behavior
N/Z/C/V from SBC result
```

---

# `(zp,X)` group

Opcodes:

| Opcode | Mnemonic |
|---:|---|
| `03` | `SLO (zp,X)` |
| `23` | `RLA (zp,X)` |
| `43` | `SRE (zp,X)` |
| `63` | `RRA (zp,X)` |
| `C3` | `DCP (zp,X)` |
| `E3` | `ISC (zp,X)` |

## Cycle count

```text
8 cycles, constant
```

## Addressing behavior

Let:

```text
operand = read(PC + 1)
ptr     = uint8(operand + X)
low     = read(ptr)
high    = read(uint8(ptr + 1))
addr    = (high << 8) | low
old     = read(addr)
```

Zero-page pointer indexing wraps at `0xFF`.

## Bus template

```text
cycle 0: R PC       -> opcode
cycle 1: R PC+1     -> operand
cycle 2: R operand  -> dummy/base zero-page read
cycle 3: R ptr      -> effective low byte
cycle 4: R ptr+1    -> effective high byte
cycle 5: R addr     -> old memory value
cycle 6: W addr     <= old memory value
cycle 7: W addr     <= modified value, then fetch-request for PC+2
```

The final CPU-visible operation is family-specific and uses the modified value.

## Implementation note

The first five cycles match the legal `(zp,X)` read-addressing machinery, for example `ORA (zp,X)` / `AND (zp,X)` / `EOR (zp,X)` / `ADC (zp,X)` / `LDA (zp,X)` / `CMP (zp,X)` / `SBC (zp,X)`, followed by the RMW write-back/write-modified sequence.

---

# `(zp),Y` group

Opcodes:

| Opcode | Mnemonic |
|---:|---|
| `13` | `SLO (zp),Y` |
| `33` | `RLA (zp),Y` |
| `53` | `SRE (zp),Y` |
| `73` | `RRA (zp),Y` |
| `D3` | `DCP (zp),Y` |
| `F3` | `ISC (zp),Y` |

## Cycle count

```text
8 cycles, constant
```

Unlike legal `(zp),Y` read instructions, this RMW illegal family does not become shorter when there is no page crossing. The RMW form always performs the full sequence.

## Addressing behavior

Let:

```text
operand = read(PC + 1)
base_lo = read(operand)
base_hi = read(uint8(operand + 1))

intermediate_addr = (base_hi << 8) | uint8(base_lo + Y)
addr              = ((base_hi + carry(base_lo + Y)) << 8) | uint8(base_lo + Y)
old               = read(addr)
```

The intermediate address uses the low-byte addition without carrying into the high byte.

## Bus template

```text
cycle 0: R PC                -> opcode
cycle 1: R PC+1              -> operand
cycle 2: R operand           -> base low byte
cycle 3: R operand+1         -> base high byte
cycle 4: R intermediate_addr -> intermediate/dummy read
cycle 5: R addr              -> old memory value
cycle 6: W addr              <= old memory value
cycle 7: W addr              <= modified value, then fetch-request for PC+2
```

For no-page-cross cases, `intermediate_addr == addr`, so cycles 4 and 5 read the same address. For page-cross cases, cycle 4 reads the no-carry address and cycle 5 reads the corrected address.

## Implementation note

This group should be implemented as an indirect-indexed RMW form, not as the shorter legal read form. The target address correction must happen before the RMW read/old-write/modified-write sequence.

---

# Per-opcode implementation table

| Opcode | Mode | Modify | ALU step | A changes | Memory changes | Flags |
|---:|---|---|---|---|---|---|
| `03` | `(zp,X)` | `ASL` | `ORA` | yes | yes | `C` from ASL, `N/Z` from final `A` |
| `23` | `(zp,X)` | `ROL` | `AND` | yes | yes | `C` from ROL, `N/Z` from final `A` |
| `43` | `(zp,X)` | `LSR` | `EOR` | yes | yes | `C` from LSR, `N/Z` from final `A` |
| `63` | `(zp,X)` | `ROR` | `ADC` | yes | yes | ADC flags after ROR carry update/input handling |
| `C3` | `(zp,X)` | `DEC` | `CMP` | no | yes | compare flags from `A - modified` |
| `E3` | `(zp,X)` | `INC` | `SBC` | yes | yes | SBC flags |
| `13` | `(zp),Y` | `ASL` | `ORA` | yes | yes | `C` from ASL, `N/Z` from final `A` |
| `33` | `(zp),Y` | `ROL` | `AND` | yes | yes | `C` from ROL, `N/Z` from final `A` |
| `53` | `(zp),Y` | `LSR` | `EOR` | yes | yes | `C` from LSR, `N/Z` from final `A` |
| `73` | `(zp),Y` | `ROR` | `ADC` | yes | yes | ADC flags after ROR carry update/input handling |
| `D3` | `(zp),Y` | `DEC` | `CMP` | no | yes | compare flags from `A - modified` |
| `F3` | `(zp),Y` | `INC` | `SBC` | yes | yes | SBC flags |

## Important carry interaction note

For `RLA`, `RRA`, and the other rotate-based families, the rotate uses the carry flag that is present before the memory modification. After the rotate, the carry flag is updated by the shift/rotate operation. Then the ALU operation runs.

For `RRA`, this means the implementation must be careful about the order:

```text
old_carry = C before ROR
modified = ROR(old, old_carry)
C = old bit 0
ADC(modified)
```

The existing qe6502_v2 ADC helper should receive the correct carry state according to the NMOS behavior already used by legal ADC paths.

For `ISC`, use the existing SBC path/helper rather than duplicating the arithmetic.

---

# Verification notes

The debugger traces and SingleStepTests light samples agree on the following structural facts:

- `(zp,X)` forms are 8 cycles.
- `(zp),Y` forms are 8 cycles.
- Both groups perform RMW: read old value, write old value, write modified value.
- The final operation uses the modified value.
- `DCP` does not modify `A`.
- `SLO/RLA/SRE/RRA/ISC` modify `A`.
- The memory target contains the modified value after execution.

---

# Summary

| Category | Count |
|---|---:|
| Opcodes investigated | 12 |
| Fully decoded | 12 |
| Needs more investigation | 0 |

Group 4 is fully decoded and ready to guide qe6502_v2 implementation.
