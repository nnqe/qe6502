# perfect6502 Debugger Cycle Terminology

This document defines the official cycle terminology for the `perfect6502_debug` tool and for future qe6502 continuation summaries.

## Full cycle

A full cycle is defined as:

```text
full cycle = memory halfcycle + CPU halfcycle
```

The memory halfcycle represents external memory/client-side bus processing.

The CPU halfcycle represents the CPU/netlist transition that consumes the bus result, updates internal/public CPU state, and leaves the next bus request visible.

## Cursor positions around a full cycle

The debugger cursor is always positioned before the next halfcycle.

There are three useful positions relative to a full cycle:

```text
before the full cycle:
    before the memory halfcycle

inside/during the full cycle:
    after the memory halfcycle and before the CPU halfcycle

after the full cycle:
    after the CPU halfcycle
```

The phrase "during the full cycle" is only valid for the point between the two halfcycles. At that point, the memory halfcycle has already completed and the CPU halfcycle is still pending.

Avoid vague "current cycle" terminology. It is only meaningful when the debugger cursor is inside a full cycle, between the memory halfcycle and the CPU halfcycle.

## Mapping to qe6502

The intended cross-model mapping is:

```text
perfect6502 memory halfcycle  <->  qe6502 client-side bus processing
perfect6502 CPU halfcycle     <->  qe6502_tick()
```

This mapping is the basis for comparing `perfect6502` netlist behavior with `qe6502_v2` behavior.

## Fetch-request cycle

A fetch-request cycle is the full cycle whose ending CPU halfcycle leaves an opcode read request on the memory bus.

The opcode byte is not physically read during that CPU halfcycle. The CPU halfcycle only leaves the opcode read request visible for the next full cycle.

## Fetch-process cycle

A fetch-process cycle is the full cycle that physically processes the opcode read request left by the previous fetch-request cycle.

The fetch-process cycle is also cycle 0 of the next instruction.

At the input/beginning of instruction cycle 0, the bus already represents the opcode read request created by the previous fetch-request cycle.

During instruction cycle 0:

```text
memory halfcycle:
    reads the opcode byte from memory

CPU halfcycle:
    consumes the opcode byte and leaves the next bus request for the instruction
```

## Instruction cycle numbering

Instruction cycle numbering always starts at cycle 0.

Instruction cycle 0 is the fetch-process cycle.

Cycle 0 is the first full cycle of the instruction: it processes the opcode fetch and, by the end of its CPU halfcycle, leaves the instruction's next memory-bus request visible.

