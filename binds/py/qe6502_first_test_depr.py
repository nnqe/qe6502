# qe6502.py
from __future__ import annotations

import ctypes
import logging
import os
import platform
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional


MODEL_MOS = 0
MODEL_NES = 1
MODEL_WDC = 2
MODEL_RW = 3
MODEL_ST = 4

_VALID_MODELS = {MODEL_MOS, MODEL_NES, MODEL_WDC, MODEL_RW, MODEL_ST}


class QE6502Error(RuntimeError):
    def __init__(self, code: int, message: str = "") -> None:
        self.code = int(code)
        super().__init__(message or f"QE6502 error {self.code}")


@dataclass(frozen=True)
class Snapshot:
    pc: int
    a: int
    x: int
    y: int
    s: int
    p: int
    model: int
    nmi: bool
    irq: bool
    waiting: bool
    serializable: bool
    istate: int


@dataclass(frozen=True)
class OpcodeMeta:
    name: str
    description: str
    addr_mode_str: str
    opcode: int
    bytes: int
    addr_mode: int
    is_cmos_extension: bool
    is_illegal: bool


class _OpcodeMetaC(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char_p),
        ("description", ctypes.c_char_p),
        ("addr_mode_str", ctypes.c_char_p),
        ("reserved_ptr", ctypes.c_void_p),
        ("opcode", ctypes.c_uint8),
        ("bytes", ctypes.c_uint8),
        ("addr_mode", ctypes.c_uint8),
        ("is_cmos_extension", ctypes.c_uint8),
        ("reserved_data", ctypes.c_uint8 * 4),
    ]


_TICK_ADDRESS_MASK = 0x0000FFFF
_TICK_BUS_WRITE = 0x00010000
_TICK_STARTING = 0x00020000
_TICK_INSTR_DONE = 0x00040000
_TICK_NOT_OK = 0x00800000
_TICK_DATA_SHIFT = 24


_lib: Optional["_Lib"] = None


def _decode_c_string(value) -> str:
    if not value:
        return ""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return ctypes.cast(value, ctypes.c_char_p).value.decode("utf-8", errors="replace")


def _assert_u8(name: str, value: int) -> int:
    if not isinstance(value, int) or value < 0 or value > 0xFF:
        raise ValueError(f"{name} must be an integer in range 0..255")
    return value


def _assert_u16(name: str, value: int) -> int:
    if not isinstance(value, int) or value < 0 or value > 0xFFFF:
        raise ValueError(f"{name} must be an integer in range 0..65535")
    return value


def _assert_model(model: int) -> int:
    if model not in _VALID_MODELS:
        raise ValueError(f"invalid QE6502 model: {model}")
    return model


def _default_library_names() -> list[str]:
    system = platform.system()

    if system == "Windows":
        return ["qe6502.dll", "libqe6502.dll"]

    if system == "Darwin":
        return ["libqe6502.dylib", "qe6502.dylib"]

    return ["libqe6502.so", "qe6502.so"]


def _find_library_path() -> str:
    env_path = os.environ.get("QE6502_LIBRARY")

    if env_path:
        return env_path

    here = Path(__file__).resolve().parent

    candidates: list[Path] = []

    for name in _default_library_names():
        candidates.append(here / name)
        candidates.append(here / "lib" / name)
        candidates.append(Path.cwd() / name)

    for path in candidates:
        if path.exists():
            return str(path)

    names = ", ".join(_default_library_names())
    raise FileNotFoundError(
        "Could not find QE6502 native library. "
        "Set QE6502_LIBRARY=/path/to/libqe6502.so "
        f"or place one of these next to qe6502.py: {names}"
    )


class _Lib:
    def __init__(self, path: Optional[str] = None) -> None:
        self.path = path or _find_library_path()
        self.dll = ctypes.CDLL(self.path)
        self._log_callback_ref = None

        self._bind_required()
        self._bind_optional()

    def _bind_required(self) -> None:
        d = self.dll

        d.qe6502_cpu_alloc.argtypes = []
        d.qe6502_cpu_alloc.restype = ctypes.c_void_p

        d.qe6502_cpu_free.argtypes = [ctypes.c_void_p]
        d.qe6502_cpu_free.restype = None

        d.qe6502_cpu_power_on.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
        d.qe6502_cpu_power_on.restype = None

        d.qe6502_cpu_tick_ex.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
        d.qe6502_cpu_tick_ex.restype = ctypes.c_uint32

        d.qe6502_ok.argtypes = [ctypes.c_void_p]
        d.qe6502_ok.restype = ctypes.c_uint8

        d.qe6502_error_code.argtypes = [ctypes.c_void_p]
        d.qe6502_error_code.restype = ctypes.c_uint16

        d.qe6502_needs_data.argtypes = [ctypes.c_void_p]
        d.qe6502_needs_data.restype = ctypes.c_uint8

        d.qe6502_has_data.argtypes = [ctypes.c_void_p]
        d.qe6502_has_data.restype = ctypes.c_uint8

        d.qe6502_read_data.argtypes = [ctypes.c_void_p]
        d.qe6502_read_data.restype = ctypes.c_uint8

        d.qe6502_address.argtypes = [ctypes.c_void_p]
        d.qe6502_address.restype = ctypes.c_uint16

        d.qe6502_is_instr_done.argtypes = [ctypes.c_void_p]
        d.qe6502_is_instr_done.restype = ctypes.c_uint8

        d.qe6502_is_started.argtypes = [ctypes.c_void_p]
        d.qe6502_is_started.restype = ctypes.c_uint8

        d.qe6502_model.argtypes = [ctypes.c_void_p]
        d.qe6502_model.restype = ctypes.c_uint8

        d.qe6502_read_nmi_pin.argtypes = [ctypes.c_void_p]
        d.qe6502_read_nmi_pin.restype = ctypes.c_uint8

        d.qe6502_nmi_hi.argtypes = [ctypes.c_void_p]
        d.qe6502_nmi_hi.restype = None

        d.qe6502_nmi_lo.argtypes = [ctypes.c_void_p]
        d.qe6502_nmi_lo.restype = None

        d.qe6502_read_irq_pin.argtypes = [ctypes.c_void_p]
        d.qe6502_read_irq_pin.restype = ctypes.c_uint8

        d.qe6502_irq_hi.argtypes = [ctypes.c_void_p]
        d.qe6502_irq_hi.restype = None

        d.qe6502_irq_lo.argtypes = [ctypes.c_void_p]
        d.qe6502_irq_lo.restype = None

        d.qe6502_dump.argtypes = [ctypes.c_void_p]
        d.qe6502_dump.restype = ctypes.c_uint64

        d.qe6502_recover.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        d.qe6502_recover.restype = None

        d.qe6502_opcode_meta.argtypes = [ctypes.c_uint8]
        d.qe6502_opcode_meta.restype = ctypes.POINTER(_OpcodeMetaC)

    def _bind_optional(self) -> None:
        d = self.dll

        if hasattr(d, "qe6502_version"):
            d.qe6502_version.argtypes = []
            d.qe6502_version.restype = ctypes.c_uint32

        if hasattr(d, "qe6502_error_string"):
            d.qe6502_error_string.argtypes = [ctypes.c_void_p]
            d.qe6502_error_string.restype = ctypes.c_char_p

        if hasattr(d, "qe6502_decode_error"):
            d.qe6502_decode_error.argtypes = [ctypes.c_uint16]
            d.qe6502_decode_error.restype = ctypes.c_char_p

        if hasattr(d, "qe6502_pause_logger"):
            d.qe6502_pause_logger.argtypes = []
            d.qe6502_pause_logger.restype = None

        if hasattr(d, "qe6502_resume_logger"):
            d.qe6502_resume_logger.argtypes = []
            d.qe6502_resume_logger.restype = None

        # Optional future native hook.
        #
        # Expected C shape:
        #   typedef void (*qe6502_log_fn)(const char* topic,
        #                                 const char* message,
        #                                 void* user);
        #   void qe6502_set_logger(qe6502_log_fn fn, void* user);
        #
        if hasattr(d, "qe6502_set_logger"):
            self._LOG_CALLBACK = ctypes.CFUNCTYPE(
                None,
                ctypes.c_char_p,
                ctypes.c_char_p,
                ctypes.c_void_p,
            )
            d.qe6502_set_logger.argtypes = [
                self._LOG_CALLBACK,
                ctypes.c_void_p,
            ]
            d.qe6502_set_logger.restype = None
        else:
            self._LOG_CALLBACK = None

    def set_log_callback(
        self,
        callback: Optional[Callable[[str, str], None]],
    ) -> None:
        if not hasattr(self.dll, "qe6502_set_logger"):
            raise NotImplementedError(
                "This qe6502 library does not export qe6502_set_logger(). "
                "Only pause_logger()/resume_logger() can be used."
            )

        if callback is None:
            self._log_callback_ref = self._LOG_CALLBACK(lambda _t, _m, _u: None)
            self.dll.qe6502_set_logger(self._log_callback_ref, None)
            self._log_callback_ref = None
            return

        def bridge(topic_ptr, message_ptr, _user):
            topic = _decode_c_string(topic_ptr)
            message = _decode_c_string(message_ptr)
            callback(topic, message)

        self._log_callback_ref = self._LOG_CALLBACK(bridge)
        self.dll.qe6502_set_logger(self._log_callback_ref, None)


def load_library(path: Optional[str] = None) -> None:
    global _lib
    _lib = _Lib(path)


def _get_lib() -> _Lib:
    global _lib

    if _lib is None:
        _lib = _Lib()

    return _lib


def version() -> int:
    lib = _get_lib()

    if not hasattr(lib.dll, "qe6502_version"):
        raise NotImplementedError("qe6502_version() is not exported by this library")

    return int(lib.dll.qe6502_version())


def decode_error(code: int) -> str:
    code = _assert_u16("code", code)
    lib = _get_lib()

    if hasattr(lib.dll, "qe6502_decode_error"):
        return _decode_c_string(lib.dll.qe6502_decode_error(code))

    return f"QE6502 error {code}"


def opcode_meta(opcode: int) -> OpcodeMeta:
    opcode = _assert_u8("opcode", opcode)
    lib = _get_lib()

    ptr = lib.dll.qe6502_opcode_meta(opcode)

    if not ptr:
        raise QE6502Error(0, f"qe6502_opcode_meta returned null for opcode {opcode:#04x}")

    m = ptr.contents
    name = _decode_c_string(m.name)

    return OpcodeMeta(
        name=name,
        description=_decode_c_string(m.description),
        addr_mode_str=_decode_c_string(m.addr_mode_str),
        opcode=int(m.opcode),
        bytes=int(m.bytes),
        addr_mode=int(m.addr_mode),
        is_cmos_extension=bool(m.is_cmos_extension),
        is_illegal=(name == "ILL"),
    )


def set_log_callback(callback: Optional[Callable[[str, str], None]]) -> None:
    _get_lib().set_log_callback(callback)


def set_logger(logger: logging.Logger) -> None:
    def callback(topic: str, message: str) -> None:
        logger.debug("[%s] %s", topic, message)

    set_log_callback(callback)


def clear_log_callback() -> None:
    set_log_callback(None)


def pause_logger() -> None:
    lib = _get_lib()

    if not hasattr(lib.dll, "qe6502_pause_logger"):
        raise NotImplementedError("qe6502_pause_logger() is not exported by this library")

    lib.dll.qe6502_pause_logger()


def resume_logger() -> None:
    lib = _get_lib()

    if not hasattr(lib.dll, "qe6502_resume_logger"):
        raise NotImplementedError("qe6502_resume_logger() is not exported by this library")

    lib.dll.qe6502_resume_logger()


class CPU:
    def __init__(self, model: Optional[int] = None) -> None:
        self._lib = _get_lib()
        self._ptr = self._lib.dll.qe6502_cpu_alloc()

        if not self._ptr:
            raise MemoryError("qe6502_cpu_alloc returned null")

        self._closed = False

        self._address = 0
        self._is_write = False
        self._is_started = False
        self._is_instr_done = True
        self._ok = False
        self._data_in = 0
        self._data_out = 0

        if model is not None:
            self.power_on(model)

    def close(self) -> None:
        if self._closed:
            return

        self._lib.dll.qe6502_cpu_free(self._ptr)
        self._ptr = None
        self._closed = True

    def __enter__(self) -> "CPU":
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass

    def _assert_alive(self) -> None:
        if self._closed or not self._ptr:
            raise RuntimeError("QE6502 CPU is closed")

    def power_on(self, model: int = MODEL_MOS) -> None:
        self._assert_alive()
        model = _assert_model(model)

        self._lib.dll.qe6502_reset(self._ptr, model)
        self._data_in = 0
        self._refresh_state()

    def tick(self, data_in: Optional[int] = None) -> None:
        self._assert_alive()

        if data_in is not None:
            self._data_in = _assert_u8("data_in", data_in)

        packed = int(self._lib.dll.qe6502_tick(self._ptr, self._data_in))
        self._data_in = 0
        self._cache_tick_state(packed)

    def tick_raw(self, data_in: int = 0) -> int:
        self._assert_alive()
        data_in = _assert_u8("data_in", data_in)

        packed = int(self._lib.dll.qe6502_tick(self._ptr, data_in))
        self._cache_tick_state(packed)
        return packed

    def feed_data(self, byte: int) -> None:
        self._assert_alive()
        self._data_in = _assert_u8("byte", byte)

    def read_data(self) -> int:
        self._assert_alive()
        return self._data_out

    def dump(self) -> Snapshot:
        self._assert_alive()

        packed = int(self._lib.dll.qe6502_dump(self._ptr)) & 0xFFFFFFFFFFFFFFFF

        pc = (packed & 0xFF) | (((packed >> 8) & 0xFF) << 8)
        istate = (packed >> 56) & 0xFF
        not_serializable = ((packed >> 63) & 0x01) != 0

        return Snapshot(
            pc=pc,
            a=(packed >> 16) & 0xFF,
            x=(packed >> 24) & 0xFF,
            y=(packed >> 32) & 0xFF,
            s=(packed >> 40) & 0xFF,
            p=(packed >> 48) & 0xFF,
            model=(packed >> 56) & 0x07,
            nmi=bool((packed >> 60) & 0x01),
            irq=bool((packed >> 61) & 0x01),
            waiting=bool((packed >> 62) & 0x01),
            serializable=not not_serializable,
            istate=istate,
        )

    def recover(self, snapshot: Snapshot) -> None:
        self._assert_alive()

        if not isinstance(snapshot, Snapshot):
            raise TypeError("recover() expects a Snapshot returned by dump()")

        if not snapshot.serializable:
            raise ValueError("cannot recover from a non-serializable snapshot")

        self._recover_raw(
            pc=snapshot.pc,
            a=snapshot.a,
            x=snapshot.x,
            y=snapshot.y,
            s=snapshot.s,
            p=snapshot.p,
            istate=snapshot.istate,
        )

    def reset_registers(
        self,
        *,
        pc: int,
        a: int = 0,
        x: int = 0,
        y: int = 0,
        s: int = 0xFD,
        p: int = 0x24,
        model: int = MODEL_MOS,
    ) -> None:
        model = _assert_model(model)

        self.power_on(model)
        self._recover_raw(
            pc=pc,
            a=a,
            x=x,
            y=y,
            s=s,
            p=p,
            istate=model,
        )

    def raise_for_error(self) -> None:
        if not self.ok:
            raise QE6502Error(self.error_code, self.error_string)

    def nmi_hi(self) -> None:
        self._assert_alive()
        self._lib.dll.qe6502_nmi_hi(self._ptr)
        self._refresh_state()

    def nmi_lo(self) -> None:
        self._assert_alive()
        self._lib.dll.qe6502_nmi_lo(self._ptr)
        self._refresh_state()

    def irq_hi(self) -> None:
        self._assert_alive()
        self._lib.dll.qe6502_irq_hi(self._ptr)
        self._refresh_state()

    def irq_lo(self) -> None:
        self._assert_alive()
        self._lib.dll.qe6502_irq_lo(self._ptr)
        self._refresh_state()

    @property
    def ok(self) -> bool:
        self._assert_alive()
        return self._ok

    @property
    def error_code(self) -> int:
        self._assert_alive()
        return int(self._lib.dll.qe6502_error_code(self._ptr))

    @property
    def error_string(self) -> str:
        self._assert_alive()

        if hasattr(self._lib.dll, "qe6502_error_string"):
            return _decode_c_string(self._lib.dll.qe6502_error_string(self._ptr))

        return decode_error(self.error_code)

    @property
    def address(self) -> int:
        self._assert_alive()
        return self._address

    @property
    def needs_data(self) -> bool:
        self._assert_alive()
        return not self._is_write

    @property
    def has_data(self) -> bool:
        self._assert_alive()
        return self._is_write

    @property
    def is_instr_done(self) -> bool:
        self._assert_alive()
        return self._is_instr_done

    @property
    def is_started(self) -> bool:
        self._assert_alive()
        return self._is_started

    @property
    def model(self) -> int:
        self._assert_alive()
        return int(self._lib.dll.qe6502_model(self._ptr))

    @property
    def nmi_pin(self) -> bool:
        self._assert_alive()
        return bool(self._lib.dll.qe6502_read_nmi_pin(self._ptr))

    @property
    def irq_pin(self) -> bool:
        self._assert_alive()
        return bool(self._lib.dll.qe6502_read_irq_pin(self._ptr))

    def _recover_raw(
        self,
        *,
        pc: int,
        a: int,
        x: int,
        y: int,
        s: int,
        p: int,
        istate: int,
    ) -> None:
        pc = _assert_u16("pc", pc)
        a = _assert_u8("a", a)
        x = _assert_u8("x", x)
        y = _assert_u8("y", y)
        s = _assert_u8("s", s)
        p = _assert_u8("p", p)
        istate = _assert_u8("istate", istate)

        pcl = pc & 0xFF
        pch = (pc >> 8) & 0xFF

        packed = (
            pcl
            | (pch << 8)
            | (a << 16)
            | (x << 24)
            | (y << 32)
            | (s << 40)
            | (p << 48)
            | (istate << 56)
        )

        self._lib.dll.qe6502_recover(self._ptr, ctypes.c_uint64(packed))
        self._refresh_state()

    def _cache_tick_state(self, packed: int) -> None:
        self._address = packed & _TICK_ADDRESS_MASK
        self._is_write = (packed & _TICK_BUS_WRITE) != 0
        self._is_started = (packed & _TICK_STARTING) == 0
        self._is_instr_done = (packed & _TICK_INSTR_DONE) != 0
        self._ok = (packed & _TICK_NOT_OK) == 0
        self._data_out = (packed >> _TICK_DATA_SHIFT) & 0xFF

    def _refresh_state(self) -> None:
        has_data = bool(self._lib.dll.qe6502_has_data(self._ptr))
        needs_data = bool(self._lib.dll.qe6502_needs_data(self._ptr))

        self._address = int(self._lib.dll.qe6502_address(self._ptr))
        self._is_write = has_data and not needs_data
        self._is_started = bool(self._lib.dll.qe6502_is_started(self._ptr))
        self._is_instr_done = bool(self._lib.dll.qe6502_is_instr_done(self._ptr))
        self._ok = bool(self._lib.dll.qe6502_ok(self._ptr))
        self._data_out = int(self._lib.dll.qe6502_read_data(self._ptr)) if self._is_write else 0