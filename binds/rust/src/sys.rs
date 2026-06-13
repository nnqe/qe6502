pub(crate) const SNAPSHOT_SIZE: usize = 64;

pub(crate) const MODEL_NMOS: u8 = 0;
pub(crate) const MODEL_NES: u8 = 1;
pub(crate) const MODEL_WDC: u8 = 2;
pub(crate) const MODEL_RW: u8 = 3;
pub(crate) const MODEL_ST: u8 = 4;

pub(crate) const FLAG_C: u8 = 1 << 0;
pub(crate) const FLAG_Z: u8 = 1 << 1;
pub(crate) const FLAG_I: u8 = 1 << 2;
pub(crate) const FLAG_D: u8 = 1 << 3;
pub(crate) const FLAG_B: u8 = 1 << 4;
pub(crate) const FLAG_UN: u8 = 1 << 5;
pub(crate) const FLAG_V: u8 = 1 << 6;
pub(crate) const FLAG_N: u8 = 1 << 7;

pub(crate) const STATUS_WRITING: u8 = 1 << 0;
pub(crate) const STATUS_OPCODE_FETCH: u8 = 1 << 1;
pub(crate) const STATUS_INTERNAL_RESET: u8 = 1 << 6;
pub(crate) const STATUS_CPU_JAMMED: u8 = 1 << 7;

#[repr(C)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub(crate) struct CpuContext {
    pub(crate) model: u8,
    pub(crate) reserved_extension: u8,
    pub(crate) microcode: u16,
    pub(crate) latch_addr: u16,
    pub(crate) latch_data: u8,
    pub(crate) hijack_microcode: u8,
    pub(crate) pc: u16,
    pub(crate) s: u8,
    pub(crate) a: u8,
    pub(crate) x: u8,
    pub(crate) y: u8,
    pub(crate) p: u8,
    pub(crate) interrupts: u8,
}

const _: [(); 16] = [(); core::mem::size_of::<CpuContext>()];
const _: [(); 2] = [(); core::mem::align_of::<CpuContext>()];

impl Default for CpuContext {
    fn default() -> Self {
        Self {
            model: 0,
            reserved_extension: 0,
            microcode: 0,
            latch_addr: 0,
            latch_data: 0,
            hijack_microcode: 0,
            pc: 0,
            s: 0,
            a: 0,
            x: 0,
            y: 0,
            p: 0,
            interrupts: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub(crate) struct Tick {
    pub(crate) address: u16,
    pub(crate) data: u8,
    pub(crate) flags: u8,
}

const _: [(); 4] = [(); core::mem::size_of::<Tick>()];
const _: [(); 2] = [(); core::mem::align_of::<Tick>()];

unsafe extern "C" {
    pub(crate) fn qe6502_restart(cpu: *mut CpuContext) -> Tick;
    pub(crate) fn qe6502_goto(cpu: *mut CpuContext, address: u16) -> Tick;
    pub(crate) fn qe6502_tick_exported(cpu: *mut CpuContext, bus: u8) -> Tick;

    pub(crate) fn qe6502_nmi_assert(cpu: *mut CpuContext, assert_nmi: u8);
    pub(crate) fn qe6502_irq_assert(cpu: *mut CpuContext, assert_irq: u8);
    pub(crate) fn qe6502_is_nmi_asserted(cpu: *const CpuContext) -> u8;
    pub(crate) fn qe6502_is_irq_asserted(cpu: *const CpuContext) -> u8;

    pub(crate) fn qe6502_save(cpu: *const CpuContext, tick: Tick, snapshot: *mut u8);
    pub(crate) fn qe6502_load(cpu: *mut CpuContext, snapshot: *const u8) -> Tick;
}
