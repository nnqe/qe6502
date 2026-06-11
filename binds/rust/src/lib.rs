use std::fmt;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct Tick {
    pub address: u16,
    pub data: u8,
    pub flags: u8,
}

mod sys;

pub const SNAPSHOT_SIZE: usize = sys::SNAPSHOT_SIZE;
pub type Snapshot = [u8; SNAPSHOT_SIZE];

pub const TICK_WRITE: u8 = sys::STATUS_WRITING;
pub const TICK_OPCODE_FETCH: u8 = sys::STATUS_OPCODE_FETCH;
pub const TICK_INTERNAL_RESET: u8 = sys::STATUS_INTERNAL_RESET;
pub const TICK_CPU_JAMMED: u8 = sys::STATUS_CPU_JAMMED;

#[repr(u8)]
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Model {
    Nmos = sys::MODEL_NMOS,
    Nes = sys::MODEL_NES,
    Wdc = sys::MODEL_WDC,
    Rw = sys::MODEL_RW,
    St = sys::MODEL_ST,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Error {
    InvalidModel(u8),
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Error::InvalidModel(model) => write!(f, "unsupported qe6502 CPU model {model}"),
        }
    }
}

impl std::error::Error for Error {}

pub struct Cpu {
    ctx: sys::CpuContext,
    tick: Tick,
}

impl Cpu {
    pub fn new(model: Model) -> Result<Self, Error> {
        let model = model as u8;
        if model >= sys::MODEL_COUNT {
            return Err(Error::InvalidModel(model));
        }

        let mut ctx = sys::CpuContext::default();
        ctx.model = model;

        Ok(Self {
            ctx,
            tick: Tick::default(),
        })
    }

    pub fn restart(&mut self) {
        self.tick = unsafe { sys::qe6502_restart(&mut self.ctx) };
    }

    pub fn jump_to(&mut self, address: u16) {
        self.tick = unsafe { sys::qe6502_goto(&mut self.ctx, address) };
    }

    #[inline(always)]
    pub fn tick(&mut self, bus: u8) -> Tick {
        self.tick = unsafe { sys::qe6502_tick_exported(&mut self.ctx, bus) };
        self.tick
    }

    #[inline(always)]
    pub fn last_tick(&self) -> Tick {
        self.tick
    }

    #[inline(always)]
    pub fn pc(&self) -> u16 {
        self.ctx.pc
    }

    #[inline(always)]
    pub fn set_pc(&mut self, value: u16) {
        self.ctx.pc = value;
    }

    #[inline(always)]
    pub fn s(&self) -> u8 {
        self.ctx.s
    }

    #[inline(always)]
    pub fn set_s(&mut self, value: u8) {
        self.ctx.s = value;
    }

    #[inline(always)]
    pub fn a(&self) -> u8 {
        self.ctx.a
    }

    #[inline(always)]
    pub fn set_a(&mut self, value: u8) {
        self.ctx.a = value;
    }

    #[inline(always)]
    pub fn x(&self) -> u8 {
        self.ctx.x
    }

    #[inline(always)]
    pub fn set_x(&mut self, value: u8) {
        self.ctx.x = value;
    }

    #[inline(always)]
    pub fn y(&self) -> u8 {
        self.ctx.y
    }

    #[inline(always)]
    pub fn set_y(&mut self, value: u8) {
        self.ctx.y = value;
    }

    #[inline(always)]
    pub fn p(&self) -> u8 {
        self.ctx.p
    }

    #[inline(always)]
    pub fn set_p(&mut self, value: u8) {
        self.ctx.p = value;
    }

    #[inline(always)]
    pub fn carry_flag(&self) -> bool {
        self.p_flag(sys::FLAG_C)
    }

    #[inline(always)]
    pub fn set_carry_flag(&mut self, value: bool) {
        self.set_p_flag(sys::FLAG_C, value);
    }

    #[inline(always)]
    pub fn zero_flag(&self) -> bool {
        self.p_flag(sys::FLAG_Z)
    }

    #[inline(always)]
    pub fn set_zero_flag(&mut self, value: bool) {
        self.set_p_flag(sys::FLAG_Z, value);
    }

    #[inline(always)]
    pub fn interrupt_disable_flag(&self) -> bool {
        self.p_flag(sys::FLAG_I)
    }

    #[inline(always)]
    pub fn set_interrupt_disable_flag(&mut self, value: bool) {
        self.set_p_flag(sys::FLAG_I, value);
    }

    #[inline(always)]
    pub fn decimal_flag(&self) -> bool {
        self.p_flag(sys::FLAG_D)
    }

    #[inline(always)]
    pub fn set_decimal_flag(&mut self, value: bool) {
        self.set_p_flag(sys::FLAG_D, value);
    }

    #[inline(always)]
    pub fn break_flag(&self) -> bool {
        self.p_flag(sys::FLAG_B)
    }

    #[inline(always)]
    pub fn set_break_flag(&mut self, value: bool) {
        self.set_p_flag(sys::FLAG_B, value);
    }

    #[inline(always)]
    pub fn unused_flag(&self) -> bool {
        self.p_flag(sys::FLAG_UN)
    }

    #[inline(always)]
    pub fn set_unused_flag(&mut self, value: bool) {
        self.set_p_flag(sys::FLAG_UN, value);
    }

    #[inline(always)]
    pub fn overflow_flag(&self) -> bool {
        self.p_flag(sys::FLAG_V)
    }

    #[inline(always)]
    pub fn set_overflow_flag(&mut self, value: bool) {
        self.set_p_flag(sys::FLAG_V, value);
    }

    #[inline(always)]
    pub fn negative_flag(&self) -> bool {
        self.p_flag(sys::FLAG_N)
    }

    #[inline(always)]
    pub fn set_negative_flag(&mut self, value: bool) {
        self.set_p_flag(sys::FLAG_N, value);
    }

    pub fn nmi_asserted(&self) -> bool {
        unsafe { sys::qe6502_is_nmi_asserted(&self.ctx) != 0 }
    }

    pub fn set_nmi_asserted(&mut self, value: bool) {
        unsafe { sys::qe6502_nmi_assert(&mut self.ctx, u8::from(value)) };
    }

    pub fn irq_asserted(&self) -> bool {
        unsafe { sys::qe6502_is_irq_asserted(&self.ctx) != 0 }
    }

    pub fn set_irq_asserted(&mut self, value: bool) {
        unsafe { sys::qe6502_irq_assert(&mut self.ctx, u8::from(value)) };
    }

    pub fn save(&self) -> Snapshot {
        let mut snapshot = [0; SNAPSHOT_SIZE];
        unsafe {
            sys::qe6502_save(&self.ctx, self.tick, snapshot.as_mut_ptr());
        }
        snapshot
    }

    pub fn load(&mut self, snapshot: &Snapshot) -> Tick {
        self.tick = unsafe { sys::qe6502_load(&mut self.ctx, snapshot.as_ptr()) };
        self.tick
    }

    #[inline(always)]
    fn p_flag(&self, mask: u8) -> bool {
        (self.ctx.p & mask) != 0
    }

    #[inline(always)]
    fn set_p_flag(&mut self, mask: u8, value: bool) {
        if value {
            self.ctx.p |= mask;
        } else {
            self.ctx.p &= !mask;
        }
    }
}
