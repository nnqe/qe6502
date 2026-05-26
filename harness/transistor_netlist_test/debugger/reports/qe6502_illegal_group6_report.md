# qe6502_v2 Illegal Opcode Report — Group 6

## Scope

This report covers the store / unstable write-data illegal opcode family:

| Opcode | Common mnemonic | Mode |
|---:|---|---|
| `83` | `SAX (zp,X)` | indexed indirect |
| `87` | `SAX zp` | zero page |
| `8F` | `SAX abs` | absolute |
| `93` | `AHX (zp),Y` / `SHA (zp),Y` | indirect indexed |
| `97` | `SAX zp,Y` | zero page,Y |
| `9B` | `TAS abs,Y` / `SHS abs,Y` | absolute,Y |
| `9C` | `SHY abs,X` | absolute,X |
| `9E` | `SHX abs,Y` | absolute,Y |
| `9F` | `AHX abs,Y` / `SHA abs,Y` | absolute,Y |

This group is bus-addressing-wise mostly clear, but it is harder than the previous groups because the important behavior is the write-data formula.

## Confidence summary

| Status | Count |
|---|---:|
| Fully decoded | 4 |
| Needs more work / verify carefully | 5 |

Fully decoded:

```text
83 87 8F 97
```

Need careful implementation verification:

```text
93 9B 9C 9E 9F
```

The `9x` high-byte-dependent opcodes are known to be unstable/quirky on NMOS 6502-family documentation. The SingleStepTests samples and debugger traces are enough to identify the broad formula family, but implementation should be validated against the full SingleStepTests corpus after coding.

---

# Stable SAX family

The stable `SAX` family writes:

```text
write_value = A & X
```

It does not modify registers or flags.

## `87` — SAX zp

### Cycle count

```text
3 cycles, constant
```

### Bus template

```text
cycle 0: R PC      -> opcode
cycle 1: R PC+1    -> zero-page operand
cycle 2: W operand <= A & X, then fetch-request for PC+2
```

### Semantics

```text
addr = operand
write(addr, A & X)
PC += 2
A, X, Y, S, P unchanged
```

Status: fully decoded.

---

## `97` — SAX zp,Y

### Cycle count

```text
4 cycles, constant
```

### Bus template

```text
cycle 0: R PC              -> opcode
cycle 1: R PC+1            -> zero-page operand
cycle 2: R operand         -> dummy/base zero-page read
cycle 3: W uint8(operand + Y) <= A & X, then fetch-request for PC+2
```

### Semantics

```text
addr = uint8(operand + Y)
write(addr, A & X)
PC += 2
A, X, Y, S, P unchanged
```

Zero-page address calculation wraps at `0xFF`.

Status: fully decoded.

---

## `8F` — SAX abs

### Cycle count

```text
4 cycles, constant
```

### Bus template

```text
cycle 0: R PC       -> opcode
cycle 1: R PC+1     -> low operand
cycle 2: R PC+2     -> high operand
cycle 3: W abs_addr <= A & X, then fetch-request for PC+3
```

### Semantics

```text
addr = (hi << 8) | lo
write(addr, A & X)
PC += 3
A, X, Y, S, P unchanged
```

Status: fully decoded.

---

## `83` — SAX (zp,X)

### Cycle count

```text
6 cycles, constant
```

### Bus template

```text
cycle 0: R PC       -> opcode
cycle 1: R PC+1     -> operand
cycle 2: R operand  -> dummy/base zero-page read
cycle 3: R ptr      -> low byte, where ptr = uint8(operand + X)
cycle 4: R ptr+1    -> high byte
cycle 5: W addr     <= A & X, then fetch-request for PC+2
```

### Semantics

```text
ptr  = uint8(operand + X)
lo   = read(ptr)
hi   = read(uint8(ptr + 1))
addr = (hi << 8) | lo

write(addr, A & X)
PC += 2
A, X, Y, S, P unchanged
```

Status: fully decoded.

---

# High-byte-dependent / unstable store family

These opcodes write values involving `A`, `X`, `Y`, `S`, and the target high byte. The common published forms are:

```text
93 AHX (zp),Y  : write A & X & high(addr)
9F AHX abs,Y   : write A & X & high(addr)
9C SHY abs,X   : write Y & high(addr)
9E SHX abs,Y   : write X & high(addr)
9B TAS abs,Y   : S = A & X, write S & high(addr)
```

However, for NMOS 6502 behavior the exact high-byte value can be subtle around page crossings. Some documentation describes the mask as `(high_byte + 1)` rather than simply `high(addr)`, and behavior can depend on the intermediate address calculation.

For qe6502_v2 implementation, these opcodes should be treated carefully and verified against the full SingleStepTests after implementation.

---

## `93` — AHX (zp),Y / SHA (zp),Y

### Cycle count

```text
6 cycles, constant
```

### Bus template

```text
cycle 0: R PC                -> opcode
cycle 1: R PC+1              -> operand
cycle 2: R operand           -> base low byte
cycle 3: R operand+1         -> base high byte
cycle 4: R intermediate_addr -> intermediate/dummy read
cycle 5: W addr              <= write_value, then fetch-request for PC+2
```

### Broad semantics

```text
base_lo = read(operand)
base_hi = read(uint8(operand + 1))
addr    = base + Y

write_value ~= A & X & high-byte-derived-mask
```

Likely conventional formula:

```text
write_value = A & X & high(addr)
```

or, depending on NMOS interpretation:

```text
write_value = A & X & (high(addr) + 1)
```

Status: needs careful verification.

Implementation should be validated against SingleStepTests before marking complete.

---

## `9F` — AHX abs,Y / SHA abs,Y

### Cycle count

```text
5 cycles, constant
```

### Bus template

```text
cycle 0: R PC                -> opcode
cycle 1: R PC+1              -> low operand
cycle 2: R PC+2              -> high operand
cycle 3: R intermediate_addr -> intermediate/dummy read
cycle 4: W addr              <= write_value, then fetch-request for PC+3
```

### Broad semantics

```text
addr = absolute + Y
write_value ~= A & X & high-byte-derived-mask
```

Status: needs careful verification.

---

## `9C` — SHY abs,X

### Cycle count

```text
5 cycles, constant
```

### Bus template

```text
cycle 0: R PC                -> opcode
cycle 1: R PC+1              -> low operand
cycle 2: R PC+2              -> high operand
cycle 3: R intermediate_addr -> intermediate/dummy read
cycle 4: W addr              <= write_value, then fetch-request for PC+3
```

### Broad semantics

```text
addr = absolute + X
write_value ~= Y & high-byte-derived-mask
```

Common form:

```text
write_value = Y & (high(addr) + 1)
```

or equivalent NMOS high-byte mask behavior.

Status: needs careful verification.

---

## `9E` — SHX abs,Y

### Cycle count

```text
5 cycles, constant
```

### Bus template

```text
cycle 0: R PC                -> opcode
cycle 1: R PC+1              -> low operand
cycle 2: R PC+2              -> high operand
cycle 3: R intermediate_addr -> intermediate/dummy read
cycle 4: W addr              <= write_value, then fetch-request for PC+3
```

### Broad semantics

```text
addr = absolute + Y
write_value ~= X & high-byte-derived-mask
```

Common form:

```text
write_value = X & (high(addr) + 1)
```

or equivalent NMOS high-byte mask behavior.

Status: needs careful verification.

---

## `9B` — TAS abs,Y / SHS abs,Y

### Cycle count

```text
5 cycles, constant
```

### Bus template

```text
cycle 0: R PC                -> opcode
cycle 1: R PC+1              -> low operand
cycle 2: R PC+2              -> high operand
cycle 3: R intermediate_addr -> intermediate/dummy read
cycle 4: W addr              <= write_value, then fetch-request for PC+3
```

### Broad semantics

Common form:

```text
S = A & X
write_value ~= S & high-byte-derived-mask
```

Status: needs careful verification.

Special note: unlike `93/9F/9C/9E`, this opcode also modifies `S`.

---

# Implementation guidance for qe6502_v2

## Stable opcodes

Implement these first:

```text
83 87 8F 97
```

They are straightforward `SAX` variants:

```text
write_value = A & X
flags unchanged
registers unchanged
```

Their bus templates are stable and fully decoded.

## Unstable/high-byte opcodes

Implement later, with direct SingleStepTests validation:

```text
93 9B 9C 9E 9F
```

The address sequencing is clear, but write-data formula must be confirmed against test cases.

Suggested development process:

1. Implement one opcode at a time.
2. Use the known bus template above.
3. Start with the conventional formula.
4. Run full SingleStepTests.
5. If failures cluster around page crossing or high-byte masks, adjust the mask formula.

---

# Summary

| Category | Count |
|---|---:|
| Opcodes investigated | 9 |
| Fully decoded | 4 |
| Needs more investigation | 5 |

Fully decoded:

```text
83 87 8F 97
```

Need more verification:

```text
93 9B 9C 9E 9F
```

Group 6 is partially decoded. The stable `SAX` subset is ready for qe6502_v2 implementation. The high-byte-dependent store subset needs targeted test-case analysis before implementation.
