package com.egt.qe6502;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.Objects;

/** Cycle-accurate qe6502 CPU wrapper over the stable native ABI. */
public final class Cpu implements AutoCloseable {
    private final Arena arena;
    private final MemorySegment ctx;
    private boolean closed;
    private int tick;

    /** Creates an NMOS 6502 CPU context. */
    public Cpu() {
        this(Model.NMOS);
    }

    /** Creates a CPU context for the selected model. */
    public Cpu(Model model) {
        Objects.requireNonNull(model, "model");
        NativeLibrary nativeLibrary = SharedNative.LIBRARY;
        ensureAbiVersion(nativeLibrary);
        arena = Arena.ofConfined();
        ctx = nativeLibrary.allocateContext(arena);
        nativeLibrary.setup(ctx, model.abiValue());
    }

    /** The CPU model stored in the native context. */
    public Model model() {
        ensureOpen();
        return Model.fromAbiValue(SharedNative.LIBRARY.getModel(ctx));
    }

    /** Changes the CPU model stored in the native context. */
    public void model(Model model) {
        ensureOpen();
        Objects.requireNonNull(model, "model");
        SharedNative.LIBRARY.setModel(ctx, model.abiValue());
    }

    /** The raw packed ABI tick value from the last CPU cycle. */
    public int rawTick() {
        return tick;
    }

    /** The current bus address from the last CPU cycle. */
    public int address() {
        return tick & 0xffff;
    }

    /** The bus data byte from the last CPU cycle. */
    public int data() {
        return (tick >>> 24) & 0xff;
    }

    /** True when the current bus cycle writes data to address; otherwise the cycle reads from address. */
    public boolean isWrite() {
        return (tick & 0x00010000) != 0;
    }

    /** True when the current bus cycle is an opcode fetch. */
    public boolean isOpcodeFetch() {
        return (tick & 0x00020000) != 0;
    }

    /** True while the CPU is performing the internal reset sequence. */
    public boolean isInternalReset() {
        return (tick & 0x00400000) != 0;
    }

    /** True when the CPU is jammed by a KIL/JAM opcode. */
    public boolean isJammed() {
        return (tick & 0x00800000) != 0;
    }

    /** Program counter register. */
    public int pc() {
        ensureOpen();
        return SharedNative.LIBRARY.getPc(ctx) & 0xffff;
    }

    /** Sets the program counter register. */
    public void pc(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setPc(ctx, value & 0xffff);
    }

    /** Stack pointer register. */
    public int s() {
        ensureOpen();
        return SharedNative.LIBRARY.getS(ctx) & 0xff;
    }

    /** Sets the stack pointer register. */
    public void s(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setS(ctx, value & 0xff);
    }

    /** Accumulator register. */
    public int a() {
        ensureOpen();
        return SharedNative.LIBRARY.getA(ctx) & 0xff;
    }

    /** Sets the accumulator register. */
    public void a(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setA(ctx, value & 0xff);
    }

    /** X index register. */
    public int x() {
        ensureOpen();
        return SharedNative.LIBRARY.getX(ctx) & 0xff;
    }

    /** Sets the X index register. */
    public void x(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setX(ctx, value & 0xff);
    }

    /** Y index register. */
    public int y() {
        ensureOpen();
        return SharedNative.LIBRARY.getY(ctx) & 0xff;
    }

    /** Sets the Y index register. */
    public void y(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setY(ctx, value & 0xff);
    }

    /** Processor status register. */
    public int p() {
        ensureOpen();
        return SharedNative.LIBRARY.getP(ctx) & 0xff;
    }

    /** Sets the processor status register. */
    public void p(int value) {
        ensureOpen();
        SharedNative.LIBRARY.setP(ctx, value & 0xff);
    }

    public boolean carryFlag() { return (p() & 0x01) != 0; }
    public void carryFlag(boolean value) { setPFlag(0x01, value); }
    public boolean zeroFlag() { return (p() & 0x02) != 0; }
    public void zeroFlag(boolean value) { setPFlag(0x02, value); }
    public boolean interruptDisableFlag() { return (p() & 0x04) != 0; }
    public void interruptDisableFlag(boolean value) { setPFlag(0x04, value); }
    public boolean decimalFlag() { return (p() & 0x08) != 0; }
    public void decimalFlag(boolean value) { setPFlag(0x08, value); }
    public boolean breakFlag() { return (p() & 0x10) != 0; }
    public void breakFlag(boolean value) { setPFlag(0x10, value); }
    public boolean unusedFlag() { return (p() & 0x20) != 0; }
    public void unusedFlag(boolean value) { setPFlag(0x20, value); }
    public boolean overflowFlag() { return (p() & 0x40) != 0; }
    public void overflowFlag(boolean value) { setPFlag(0x40, value); }
    public boolean negativeFlag() { return (p() & 0x80) != 0; }
    public void negativeFlag(boolean value) { setPFlag(0x80, value); }

    /** True when the NMI input is logically asserted. */
    public boolean nmiAsserted() {
        ensureOpen();
        return SharedNative.LIBRARY.isNmiAsserted(ctx);
    }

    /** Asserts or deasserts the NMI input. */
    public void nmiAsserted(boolean value) {
        ensureOpen();
        SharedNative.LIBRARY.nmiAssert(ctx, value);
    }

    /** True when the IRQ input is logically asserted. */
    public boolean irqAsserted() {
        ensureOpen();
        return SharedNative.LIBRARY.isIrqAsserted(ctx);
    }

    /** Asserts or deasserts the IRQ input. */
    public void irqAsserted(boolean value) {
        ensureOpen();
        SharedNative.LIBRARY.irqAssert(ctx, value);
    }

    /** Starts the CPU reset sequence and stores the first reset tick. */
    public void restart() {
        ensureOpen();
        tick = SharedNative.LIBRARY.restart(ctx);
    }

    /** Places the CPU at an address and stores the first fetch tick. */
    public void jumpTo(int address) {
        ensureOpen();
        tick = SharedNative.LIBRARY.goTo(ctx, address & 0xffff);
    }

    /** Runs one bus cycle, feeding zero on the data bus. */
    public void tick() {
        tick(0);
    }

    /** Runs one bus cycle, feeding the byte observed on the data bus. */
    public void tick(int data) {
        ensureOpen();
        tick = SharedNative.LIBRARY.tick(ctx, data & 0xff);
    }

    /** Saves the portable 64-byte CPU snapshot, including the last tick. */
    public byte[] save() {
        ensureOpen();
        return SharedNative.LIBRARY.save(ctx, tick);
    }

    /** Loads a portable 64-byte CPU snapshot and restores its tick. */
    public void load(byte[] snapshot) {
        ensureOpen();
        Objects.requireNonNull(snapshot, "snapshot");
        if (snapshot.length != NativeLibrary.SNAPSHOT_SIZE) {
            throw new IllegalArgumentException("Snapshot must be exactly 64 bytes.");
        }
        tick = SharedNative.LIBRARY.load(ctx, snapshot);
    }

    @Override
    public void close() {
        if (!closed) {
            closed = true;
            arena.close();
        }
    }

    private void setPFlag(int mask, boolean value) {
        int current = p();
        p(value ? (current | mask) : (current & ~mask));
    }

    private void ensureOpen() {
        if (closed) {
            throw new IllegalStateException("CPU context is closed.");
        }
    }

    private static void ensureAbiVersion(NativeLibrary nativeLibrary) {
        int version = nativeLibrary.version();
        if (version != NativeLibrary.EXPECTED_ABI_VERSION) {
            throw new IllegalStateException(
                "Unsupported qe6502 ABI version 0x" + Integer.toHexString(version) +
                "; expected 0x" + Integer.toHexString(NativeLibrary.EXPECTED_ABI_VERSION) + ".");
        }
    }

    private static final class SharedNative {
        static final NativeLibrary LIBRARY = new NativeLibrary();
    }
}
