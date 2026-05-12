# qe6502

A compact, cycle-driven 6502 CPU core written in C.

---

## Overview

`qe6502` is a minimal and portable implementation of the 6502 family of CPUs.  
It is designed for emulator development, experimentation, and low-level systems work.

The core follows a **cycle-accurate, bus-accurate, bus-driven model**, allowing fine-grained control over memory and I/O interactions.

---

## Features

- Cycle-by-cycle execution model
- Cycle-accurate execution with accurate per-cycle bus interactions
- High-performance design with low overhead per cycle
- Clean and minimal C API
- Support for multiple CPU variants:
  - NMOS 6502 (MOS, NES)
  - CMOS 65C02 (WDC, Rockwell, Synertek)
- No dynamic memory allocation
- Small and predictable CPU state
- FFI-friendly interface

---

## Getting Started

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Build Options

| Option | Description | Default |
|------|------------|--------|
| `QE6502_BUILD_SHARED` | Build shared library | OFF |
| `QE6502_ENABLE_DEBUG_LOG` | Enable logging | OFF |
| `QE6502_ENABLE_NMOS_6502` | Enable NMOS core | ON |
| `QE6502_ENABLE_CMOS_65C02` | Enable CMOS cores | ON |
| `QE6502_ENABLE_CYCLE_MERGE` | Faster, non cycle-accurate mode | OFF |
| `QE6502_BUILD_TESTS` | Build test programs | ON |

---

## Basic Usage

The CPU is driven cycle-by-cycle through a simple bus interface:

```c
    void* cpu_ptr = malloc( qe6502_cpu_size() );

    if (!qe6502_cpu_create(cpu_ptr, qe6502_cpu_size()))
    {
        return "Memory error";
    }

    qe6502_cpu_power_on(cpu_ptr, QE6502_MODEL_NES);
    if (!qe6502_ok(cpu_ptr))
    {
        return "CPU power-on error!";
    }

    uint64_t cycles = 0;
    uint64_t instructions = 0;
    while(qe6502_ok(cpu_ptr))
    {
        uint16_t address = qe6502_address(cpu_ptr);
        if (qe6502_needs_data(cpu_ptr))
        {
            // read memory
            qe6502_feed_data(cpu_ptr, memory[address]);
        }
        else
        {
            // write memory
            memory[address] = qe6502_data(cpu_ptr);
        }

        if (nmi)
        {
            qe6502_nmi_lo(cpu_ptr);
            qe6502_nmi_hi(cpu_ptr);
        }
        
        qe6502_cpu_tick(cpu_ptr);
        cycles++;
        if (qe6502_instr_done(cpu_ptr))
        {
            instructions++;
        }
        // process system
    }
    return "CPU Error";
```

---

## Execution Model

The core does **not** manage memory internally.

Instead:
- The CPU exposes the current bus state (address + read/write)
- The host system provides or consumes data

This allows:
- Full control over memory mapping
- Easy integration with emulators and hardware simulations
- Cycle-accurate behavior

---

## Tests

The repository includes a bundled **Klaus2m5 functional test suite**.

To build tests:

```bash
cmake -S . -B build -DQE6502_BUILD_TESTS=ON
cmake --build build
```

---

## Design Goals

- Simplicity over abstraction
- Predictable behavior
- Minimal dependencies
- Clear separation of concerns
- No hidden allocations
- Low-overhead per-cycle execution

---

## Project Structure

```
cpu/        - core implementation
tests/      - test programs and validation
```

---

## Notes

- Default configuration is cycle-accurate and bus-interactions-accurate.
- Enabling `QE6502_ENABLE_CYCLE_MERGE` improves performance but breaks strict cycle accuracy.
- Internal implementation details are intentionally hidden from the public API.

---

## License

See the `LICENSE` file.
