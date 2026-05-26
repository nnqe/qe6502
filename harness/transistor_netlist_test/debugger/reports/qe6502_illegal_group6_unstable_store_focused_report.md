# qe6502_v2 Illegal Opcode Report — Group 6 Focused Follow-up

## Scope

This follow-up focuses only on the previously unresolved / high-byte-dependent store opcodes:

```text
93 9B 9C 9E 9F
```

These are now considered decoded well enough for implementation guidance, with one important caveat: they are NMOS unstable-store style opcodes, so final implementation must still be validated against the full SingleStepTests corpus.

## Result summary

| Opcode | Common name | Status |
|---:|---|---|
| `93` | `AHX (zp),Y` / `SHA (zp),Y` | decoded |
| `9F` | `AHX abs,Y` / `SHA abs,Y` | decoded |
| `9C` | `SHY abs,X` | decoded |
| `9E` | `SHX abs,Y` | decoded |
| `9B` | `TAS abs,Y` / `SHS abs,Y` | decoded |

The key discovery is that the same high-byte mask mechanism appears in all five opcodes:

```text
mask = base_high + 1
```

The target address high byte also becomes unstable on page crossing:

```text
if page_cross:
    write_addr_high = write_value
else:
    write_addr_high = base_high
```

So the write address itself can be corrupted by the generated write byte.

---

# Shared high-byte rule

For absolute-indexed variants:

```text
base_low  = operand_low
base_high = operand_high
low_sum   = base_low + index
low       = uint8(low_sum)
cross     = low_sum > 0xFF
mask      = uint8(base_high + 1)
```

For `93 (zp),Y`:

```text
base_low  = read(zp_operand)
base_high = read(uint8(zp_operand + 1))
low_sum   = base_low + Y
low       = uint8(low_sum)
cross     = low_sum > 0xFF
mask      = uint8(base_high + 1)
```

The write address is:

```text
if cross:
    write_addr = (write_value << 8) | low
else:
    write_addr = (base_high << 8) | low
```

This is the important unstable behavior. The write address high byte is not simply the corrected effective high byte in page-cross cases.

---

# Per-opcode formulas

## `93` — AHX/SHA `(zp),Y`

```text
write_value = A & X & uint8(base_high + 1)
```

Address rule:

```text
if base_low + Y crosses page:
    write_addr_high = write_value
else:
    write_addr_high = base_high
```

Registers and flags unchanged.

## `9F` — AHX/SHA `abs,Y`

```text
write_value = A & X & uint8(base_high + 1)
```

Address rule:

```text
if operand_low + Y crosses page:
    write_addr_high = write_value
else:
    write_addr_high = base_high
```

Registers and flags unchanged.

## `9C` — SHY `abs,X`

```text
write_value = Y & uint8(base_high + 1)
```

Address rule:

```text
if operand_low + X crosses page:
    write_addr_high = write_value
else:
    write_addr_high = base_high
```

Registers and flags unchanged.

## `9E` — SHX `abs,Y`

```text
write_value = X & uint8(base_high + 1)
```

Address rule:

```text
if operand_low + Y crosses page:
    write_addr_high = write_value
else:
    write_addr_high = base_high
```

Registers and flags unchanged.

## `9B` — TAS/SHS `abs,Y`

```text
S = A & X
write_value = S & uint8(base_high + 1)
```

Address rule:

```text
if operand_low + Y crosses page:
    write_addr_high = write_value
else:
    write_addr_high = base_high
```

Flags unchanged.

Trace note: in the current debugger trace, `S` becomes visibly updated at the next CPU-input boundary after the write service. Final state is `S = A & X`.

---

# Hypothesis and attempted disproof

## Hypothesis

The original uncertain hypothesis was:

```text
write_value uses a high-byte-derived mask, probably base_high + 1.
```

The stronger tested hypothesis is now:

```text
mask = uint8(base_high + 1)
write_value = formula-specific register expression & mask
if page_cross:
    write_addr_high = write_value
else:
    write_addr_high = base_high
```

## Wrong alternatives tested against

The targeted debugger tests were chosen to disprove these wrong alternatives:

1. `mask = base_high`
2. `mask = corrected_effective_high`
3. page-cross write address high remains the corrected effective high byte
4. page-cross write address high remains the base high byte

The tests used `base_high = 0x44`, so:

```text
base_high              = 44
base_high + 1 / mask   = 45
corrected high on cross = 45
```

For page-cross tests, registers were chosen so that:

```text
write_value = 05
```

This makes the corrupted write address obvious:

```text
expected normal corrected address = 4510
observed write address            = 0510
observed write data               = 05
```

That disproves the idea that page-cross writes simply target the corrected effective address.

---

# Targeted perfect6502_debug tests

## `93` no page-cross

Setup idea:

```text
base_high = 44
mask      = 45
A = FF
X = FF
Y = 05
base_low = 10
```

Expected by hypothesis:

```text
write_value = FF & FF & 45 = 45
write_addr  = 4415
```

Observed:

```text
out W 4415 <= 45
```

This disproves `mask = base_high`, which would have produced `44`.

## `93` page-cross

Setup idea:

```text
base_high = 44
base_low  = F0
Y         = 20
A         = 3F
X         = FF
mask      = 45
```

Expected by hypothesis:

```text
write_value = 3F & FF & 45 = 05
normal corrected address would be 4510
unstable write address should be 0510
```

Observed:

```text
out W 0510 <= 05
```

This confirms both the mask and the page-cross address corruption.

## `9F` page-cross

```text
base = 44F0
Y    = 20
A    = 3F
X    = FF
mask = 45
```

Observed:

```text
out W 0510 <= 05
```

So `9F` follows the same AHX rule as `93`.

## `9C` page-cross

```text
base = 44F0
X    = 20
Y    = 3F
mask = 45
```

Expected:

```text
write_value = Y & mask = 3F & 45 = 05
write_addr  = 0510
```

Observed:

```text
out W 0510 <= 05
```

## `9E` page-cross

```text
base = 44F0
Y    = 20
X    = 3F
mask = 45
```

Expected:

```text
write_value = X & mask = 3F & 45 = 05
write_addr  = 0510
```

Observed:

```text
out W 0510 <= 05
```

## `9B` page-cross

```text
base = 44F0
Y    = 20
A    = 3F
X    = FF
mask = 45
```

Expected:

```text
S = A & X = 3F
write_value = S & mask = 3F & 45 = 05
write_addr  = 0510
```

Observed:

```text
out W 0510 <= 05
final S = 3F
```

---

# Cross-check with 6502_light / SingleStepTests samples

The light samples agree with the same rule.

Examples:

## `9C` page-cross sample

Observed sample behavior includes:

```text
operand high = EE
Y = 97
write data = 87
write addr high = 87
```

Formula:

```text
mask = EE + 1 = EF
write_value = Y & mask = 97 & EF = 87
```

Since the indexed address crosses page, the write address high byte is also `87`.

## `9E` page-cross sample

Observed:

```text
operand high = 64
X = 22
write data = 20
write addr high = 20
```

Formula:

```text
mask = 65
write_value = X & mask = 22 & 65 = 20
```

## `9F` page-cross sample

Observed:

```text
operand high = BE
A = 7A
X = CB
write data = 0A
write addr high = 0A
```

Formula:

```text
mask = BF
write_value = A & X & mask = 7A & CB & BF = 0A
```

## `9B` page-cross sample

Observed:

```text
operand high = 5B
A = A8
X = 68
final S = 28
write data = 08
write addr high = 08
```

Formula:

```text
S = A & X = A8 & 68 = 28
mask = 5C
write_value = S & mask = 28 & 5C = 08
```

---

# Implementation guidance for qe6502_v2

## Shared helper idea

A shared helper for unstable stores could take:

```text
base_high
low_sum_crossed
low_result
write_value
```

and produce:

```text
write_addr_high = low_sum_crossed ? write_value : base_high
write_addr      = (write_addr_high << 8) | low_result
```

## Formula helpers

```text
AHX/SHA: write_value = A & X & (base_high + 1)
SHY:     write_value = Y & (base_high + 1)
SHX:     write_value = X & (base_high + 1)
TAS/SHS: S = A & X; write_value = S & (base_high + 1)
```

All masks are 8-bit:

```text
mask = uint8(base_high + 1)
```

## Register effects

```text
93: no register/flag changes
9F: no register/flag changes
9C: no register/flag changes
9E: no register/flag changes
9B: S = A & X, flags unchanged
```

---

# Remaining risk

These opcodes are now decoded from both targeted perfect6502 tests and SingleStepTests-light examples.

Remaining risk is implementation accuracy, especially around:

1. exact timing of when `S` changes for `9B`;
2. making sure the address-high corruption uses the computed write byte, not the corrected effective high byte;
3. validating against the full SingleStepTests corpus after implementation.

No unresolved behavior remains at the level needed for initial qe6502_v2 implementation guidance.

---

# Updated focused summary

| Category | Count |
|---|---:|
| Previously unresolved opcodes investigated | 5 |
| Decoded after focused tests | 5 |
| Still unresolved | 0 |

Updated group 6 status:

| Subgroup | Count | Status |
|---|---:|---|
| Stable SAX (`83 87 8F 97`) | 4 | decoded |
| Unstable stores (`93 9B 9C 9E 9F`) | 5 | decoded after follow-up |

Group 6 is now fully decoded for implementation planning, with the expected final step of full SingleStepTests validation after coding.
