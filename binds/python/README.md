# qe6502 for Python

`qe6502` is a lightweight bus-cycle 6502 CPU emulator exposed as a CPython extension module.

The Python package wraps the stable qe6502 C ABI and provides a small stateful `CPU` object for stepping the emulator one bus cycle at a time.

## Install

```sh
pip install qe6502
```

The published package is intended to provide prebuilt wheels for supported platforms. Building from source may require a C compiler, CMake, and Python development headers.

## Quick start

```python
import qe6502

cpu = qe6502.CPU(qe6502.MODEL_NMOS)
memory = bytearray(65536)

# INC $0200; JMP $8000
memory[0x8000:0x8006] = bytes([0xEE, 0x00, 0x02, 0x4C, 0x00, 0x80])

bus = cpu.jump_to(0x8000)

for _ in range(64):
    address = bus & qe6502.TICK_ADDRESS_MASK
    if bus & qe6502.TICK_WRITING:
        memory[address] = bus >> qe6502.TICK_BUS_SHIFT
        bus = cpu.tick()
    else:
        bus = cpu.tick(memory[address])

assert memory[0x0200] != 0
```

## CPU models

The binding exposes the same model constants as the C ABI, including:

- `MODEL_NMOS`
- `MODEL_NES`
- `MODEL_WDC65C02`
- `MODEL_ROCKWELL65C02`
- `MODEL_ST65C02`

## Bus state

`CPU.tick()` returns the packed qe6502 bus state. Use the exported masks and shifts to inspect it:

- `TICK_ADDRESS_MASK`
- `TICK_BUS_SHIFT`
- `TICK_WRITING`
- `TICK_OPCODE_FETCH`
- `TICK_INTERNAL_RESET`
- `TICK_CPU_JAMMED`

## Registers and flags

The `CPU` object exposes register properties such as `pc`, `a`, `x`, `y`, `s`, and `p`, plus individual flag properties such as `carry_flag`, `decimal_flag`, and `negative_flag`.

## Interrupt pins

Use `cpu.nmi_asserted` and `cpu.irq_asserted` to inspect or update the interrupt input pins.

## Snapshots

```python
snapshot = cpu.save()
restored_tick = cpu.load(snapshot)
```

Snapshots are byte strings with length `SNAPSHOT_SIZE`.

## Versioning

The Python package version is derived from the qe6502 core major/minor version as `major.minor.0`.

The extension targets CPython's stable ABI for Python 3.10 and newer.

## License

MIT
