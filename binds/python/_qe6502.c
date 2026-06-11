#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "qe6502/qe6502_abi.h"

#include <stdint.h>
#include <string.h>

#define PY_QE6502_EXPECTED_ABI_VERSION QE6502_ABI_VERSION
#define PY_QE6502_TICK_ADDRESS_MASK UINT32_C(0x0000ffff)
#define PY_QE6502_TICK_STATUS_MASK  (UINT32_C(0x000000ff) << QE6502_ABI_TICK_STATUS_SHIFT)
#define PY_QE6502_TICK_BUS_MASK     (UINT32_C(0x000000ff) << QE6502_ABI_TICK_BUS_SHIFT)

typedef struct {
    PyObject_HEAD
    qe6502abi_context_t ctx;
    qe6502abi_tick_t tick;
} Qe6502CPUObject;

typedef uint32_t (*Qe6502Getter)(const qe6502abi_context_t* ctx);
typedef void (*Qe6502Setter)(qe6502abi_context_t* ctx, uint32_t value);

typedef struct {
    Qe6502Getter get;
    Qe6502Setter set;
    uint32_t max_value;
} Qe6502RegisterAccess;

typedef struct {
    uint8_t mask;
} Qe6502FlagAccess;

static int
check_abi_version(void)
{
    const uint32_t version = qe6502abi_version();

    if (version != PY_QE6502_EXPECTED_ABI_VERSION) {
        PyErr_Format(
            PyExc_RuntimeError,
            "unsupported qe6502 ABI version 0x%08x; expected 0x%08x",
            (unsigned int)version,
            (unsigned int)PY_QE6502_EXPECTED_ABI_VERSION);
        return -1;
    }

    return 0;
}

static int
uint32_from_object(PyObject* object, const char* name, uint32_t max_value, uint32_t* out)
{
    unsigned long value;

    value = PyLong_AsUnsignedLong(object);

    if (PyErr_Occurred()) {
        return -1;
    }

    if (value > (unsigned long)max_value) {
        PyErr_Format(PyExc_ValueError, "%s must be in range 0..%lu", name, (unsigned long)max_value);
        return -1;
    }

    *out = (uint32_t)value;
    return 0;
}

static int
bool_from_object(PyObject* object, uint8_t* out)
{
    const int value = PyObject_IsTrue(object);

    if (value < 0) {
        return -1;
    }

    *out = value ? 1u : 0u;
    return 0;
}

static PyObject*
uint32_to_python(uint32_t value)
{
    return PyLong_FromUnsignedLong((unsigned long)value);
}

static int
CPU_init(Qe6502CPUObject* self, PyObject* args, PyObject* kwargs)
{
    unsigned long model = QE6502_ABI_MODEL_NMOS;

    static char* kwlist[] = {
        "model",
        NULL,
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|k", kwlist, &model)) {
        return -1;
    }

    if (model >= QE6502_ABI_MODEL_COUNT) {
        PyErr_SetString(PyExc_ValueError, "model must be a valid qe6502 model");
        return -1;
    }

    qe6502abi_setup(&self->ctx, (uint32_t)model);
    self->tick = 0u;

    return 0;
}

static PyObject*
CPU_setup(Qe6502CPUObject* self, PyObject* arg)
{
    uint32_t model;

    if (uint32_from_object(arg, "model", QE6502_ABI_MODEL_COUNT - 1u, &model) < 0) {
        return NULL;
    }

    qe6502abi_setup(&self->ctx, model);
    self->tick = 0u;

    return uint32_to_python(self->tick);
}

static PyObject*
CPU_restart(Qe6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    self->tick = qe6502abi_restart(&self->ctx);
    return uint32_to_python(self->tick);
}

static PyObject*
CPU_jump_to(Qe6502CPUObject* self, PyObject* arg)
{
    uint32_t address;

    if (uint32_from_object(arg, "address", 0xffffu, &address) < 0) {
        return NULL;
    }

    self->tick = qe6502abi_goto(&self->ctx, address);
    return uint32_to_python(self->tick);
}

static PyObject*
CPU_tick(Qe6502CPUObject* self, PyObject* args)
{
    unsigned long bus = 0u;

    if (!PyArg_ParseTuple(args, "|k", &bus)) {
        return NULL;
    }

    if (bus > 0xfful) {
        PyErr_SetString(PyExc_ValueError, "data must be in range 0..255");
        return NULL;
    }

    self->tick = qe6502abi_tick(&self->ctx, (uint32_t)bus);
    return uint32_to_python(self->tick);
}

static PyObject*
CPU_save(Qe6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    uint8_t snapshot[QE6502_ABI_SNAPSHOT_SIZE];

    qe6502abi_save(&self->ctx, self->tick, snapshot);

    return PyBytes_FromStringAndSize((const char*)snapshot, (Py_ssize_t)QE6502_ABI_SNAPSHOT_SIZE);
}

static PyObject*
CPU_load(Qe6502CPUObject* self, PyObject* arg)
{
    Py_buffer view;

    if (PyObject_GetBuffer(arg, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }

    if (view.len != (Py_ssize_t)QE6502_ABI_SNAPSHOT_SIZE) {
        PyBuffer_Release(&view);
        PyErr_SetString(PyExc_ValueError, "snapshot must be exactly 64 bytes");
        return NULL;
    }

    self->tick = qe6502abi_load(&self->ctx, (const uint8_t*)view.buf);
    PyBuffer_Release(&view);

    return uint32_to_python(self->tick);
}

static PyObject*
CPU_get_raw_tick(Qe6502CPUObject* self, void* Py_UNUSED(closure))
{
    return uint32_to_python(self->tick);
}

static PyObject*
CPU_get_register(Qe6502CPUObject* self, void* closure)
{
    const Qe6502RegisterAccess* access = (const Qe6502RegisterAccess*)closure;
    return uint32_to_python(access->get(&self->ctx));
}

static int
CPU_set_register(Qe6502CPUObject* self, PyObject* value, void* closure)
{
    const Qe6502RegisterAccess* access = (const Qe6502RegisterAccess*)closure;
    uint32_t raw;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete qe6502 CPU register");
        return -1;
    }

    if (uint32_from_object(value, "register", access->max_value, &raw) < 0) {
        return -1;
    }

    access->set(&self->ctx, raw);
    return 0;
}

static PyObject*
CPU_get_flag(Qe6502CPUObject* self, void* closure)
{
    const Qe6502FlagAccess* access = (const Qe6502FlagAccess*)closure;
    const uint32_t p = qe6502abi_get_p(&self->ctx);

    if ((p & access->mask) != 0u) {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}

static int
CPU_set_flag(Qe6502CPUObject* self, PyObject* value, void* closure)
{
    const Qe6502FlagAccess* access = (const Qe6502FlagAccess*)closure;
    uint8_t raw;
    uint32_t p;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete qe6502 CPU flag");
        return -1;
    }

    if (bool_from_object(value, &raw) < 0) {
        return -1;
    }

    p = qe6502abi_get_p(&self->ctx);

    if (raw) {
        p |= access->mask;
    } else {
        p &= ~(uint32_t)access->mask;
    }

    qe6502abi_set_p(&self->ctx, p);
    return 0;
}

static PyObject*
CPU_get_nmi_asserted(Qe6502CPUObject* self, void* Py_UNUSED(closure))
{
    if (qe6502abi_is_nmi_asserted(&self->ctx) != 0u) {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}

static int
CPU_set_nmi_asserted(Qe6502CPUObject* self, PyObject* value, void* Py_UNUSED(closure))
{
    uint8_t raw;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete nmi_asserted");
        return -1;
    }

    if (bool_from_object(value, &raw) < 0) {
        return -1;
    }

    qe6502abi_nmi_assert(&self->ctx, raw);
    return 0;
}

static PyObject*
CPU_get_irq_asserted(Qe6502CPUObject* self, void* Py_UNUSED(closure))
{
    if (qe6502abi_is_irq_asserted(&self->ctx) != 0u) {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}

static int
CPU_set_irq_asserted(Qe6502CPUObject* self, PyObject* value, void* Py_UNUSED(closure))
{
    uint8_t raw;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "cannot delete irq_asserted");
        return -1;
    }

    if (bool_from_object(value, &raw) < 0) {
        return -1;
    }

    qe6502abi_irq_assert(&self->ctx, raw);
    return 0;
}

static Qe6502RegisterAccess register_pc = {qe6502abi_get_pc, qe6502abi_set_pc, 0xffffu};
static Qe6502RegisterAccess register_s = {qe6502abi_get_s, qe6502abi_set_s, 0xffu};
static Qe6502RegisterAccess register_a = {qe6502abi_get_a, qe6502abi_set_a, 0xffu};
static Qe6502RegisterAccess register_x = {qe6502abi_get_x, qe6502abi_set_x, 0xffu};
static Qe6502RegisterAccess register_y = {qe6502abi_get_y, qe6502abi_set_y, 0xffu};
static Qe6502RegisterAccess register_p = {qe6502abi_get_p, qe6502abi_set_p, 0xffu};
static Qe6502RegisterAccess register_model = {qe6502abi_get_model, qe6502abi_set_model, QE6502_ABI_MODEL_COUNT - 1u};

static Qe6502FlagAccess flag_carry = {QE6502_ABI_FLAG_C};
static Qe6502FlagAccess flag_zero = {QE6502_ABI_FLAG_Z};
static Qe6502FlagAccess flag_interrupt_disable = {QE6502_ABI_FLAG_I};
static Qe6502FlagAccess flag_decimal = {QE6502_ABI_FLAG_D};
static Qe6502FlagAccess flag_break = {QE6502_ABI_FLAG_B};
static Qe6502FlagAccess flag_unused = {QE6502_ABI_FLAG_UN};
static Qe6502FlagAccess flag_overflow = {QE6502_ABI_FLAG_V};
static Qe6502FlagAccess flag_negative = {QE6502_ABI_FLAG_N};

static PyMethodDef CPU_methods[] = {
    {"setup", (PyCFunction)CPU_setup, METH_O, "Set up the CPU model and clear the cached tick."},
    {"restart", (PyCFunction)CPU_restart, METH_NOARGS, "Start the reset sequence and return the first packed tick."},
    {"jump_to", (PyCFunction)CPU_jump_to, METH_O, "Place the CPU at an address and return the first packed fetch tick."},
    {"tick", (PyCFunction)CPU_tick, METH_VARARGS, "Run one bus cycle and return the packed tick."},
    {"save", (PyCFunction)CPU_save, METH_NOARGS, "Save the portable 64-byte CPU snapshot including the cached tick."},
    {"load", (PyCFunction)CPU_load, METH_O, "Load a portable 64-byte CPU snapshot and return its packed tick."},
    {NULL, NULL, 0, NULL}
};

static PyGetSetDef CPU_getset[] = {
    {"raw_tick", (getter)CPU_get_raw_tick, NULL, "last packed ABI tick", NULL},
    {"pc", (getter)CPU_get_register, (setter)CPU_set_register, "program counter", &register_pc},
    {"s", (getter)CPU_get_register, (setter)CPU_set_register, "stack pointer", &register_s},
    {"a", (getter)CPU_get_register, (setter)CPU_set_register, "accumulator", &register_a},
    {"x", (getter)CPU_get_register, (setter)CPU_set_register, "X register", &register_x},
    {"y", (getter)CPU_get_register, (setter)CPU_set_register, "Y register", &register_y},
    {"p", (getter)CPU_get_register, (setter)CPU_set_register, "processor status", &register_p},
    {"model", (getter)CPU_get_register, (setter)CPU_set_register, "CPU model", &register_model},
    {"carry_flag", (getter)CPU_get_flag, (setter)CPU_set_flag, "carry flag", &flag_carry},
    {"zero_flag", (getter)CPU_get_flag, (setter)CPU_set_flag, "zero flag", &flag_zero},
    {"interrupt_disable_flag", (getter)CPU_get_flag, (setter)CPU_set_flag, "interrupt-disable flag", &flag_interrupt_disable},
    {"decimal_flag", (getter)CPU_get_flag, (setter)CPU_set_flag, "decimal flag", &flag_decimal},
    {"break_flag", (getter)CPU_get_flag, (setter)CPU_set_flag, "break flag", &flag_break},
    {"unused_flag", (getter)CPU_get_flag, (setter)CPU_set_flag, "unused status flag", &flag_unused},
    {"overflow_flag", (getter)CPU_get_flag, (setter)CPU_set_flag, "overflow flag", &flag_overflow},
    {"negative_flag", (getter)CPU_get_flag, (setter)CPU_set_flag, "negative flag", &flag_negative},
    {"nmi_asserted", (getter)CPU_get_nmi_asserted, (setter)CPU_set_nmi_asserted, "NMI input state", NULL},
    {"irq_asserted", (getter)CPU_get_irq_asserted, (setter)CPU_set_irq_asserted, "IRQ input state", NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

static PyTypeObject Qe6502CPUType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_qe6502.CPU",
    .tp_basicsize = sizeof(Qe6502CPUObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "qe6502 CPU ABI context",
    .tp_init = (initproc)CPU_init,
    .tp_methods = CPU_methods,
    .tp_getset = CPU_getset,
};

static void
CPU_type_init_runtime_slots(void)
{
    Qe6502CPUType.tp_new = PyType_GenericNew;
}

static PyObject*
module_version(PyObject* Py_UNUSED(self), PyObject* Py_UNUSED(ignored))
{
    return uint32_to_python(qe6502abi_version());
}

static PyMethodDef module_methods[] = {
    {"version", (PyCFunction)module_version, METH_NOARGS, "Return the qe6502 ABI version."},
    {NULL, NULL, 0, NULL}
};

static int
add_uint_constant(PyObject* module, const char* name, uint32_t value)
{
    PyObject* object = uint32_to_python(value);

    if (object == NULL) {
        return -1;
    }

    if (PyModule_AddObject(module, name, object) < 0) {
        Py_DECREF(object);
        return -1;
    }

    return 0;
}

static int
add_constants(PyObject* module)
{
    if (add_uint_constant(module, "ABI_VERSION", QE6502_ABI_VERSION) < 0) { return -1; }
    if (add_uint_constant(module, "SNAPSHOT_SIZE", QE6502_ABI_SNAPSHOT_SIZE) < 0) { return -1; }

    if (add_uint_constant(module, "MODEL_NMOS", QE6502_ABI_MODEL_NMOS) < 0) { return -1; }
    if (add_uint_constant(module, "MODEL_NES", QE6502_ABI_MODEL_NES) < 0) { return -1; }
    if (add_uint_constant(module, "MODEL_WDC", QE6502_ABI_MODEL_WDC) < 0) { return -1; }
    if (add_uint_constant(module, "MODEL_ROCKWELL", QE6502_ABI_MODEL_RW) < 0) { return -1; }
    if (add_uint_constant(module, "MODEL_SYNERTEK", QE6502_ABI_MODEL_ST) < 0) { return -1; }

    if (add_uint_constant(module, "FLAG_CARRY", QE6502_ABI_FLAG_C) < 0) { return -1; }
    if (add_uint_constant(module, "FLAG_ZERO", QE6502_ABI_FLAG_Z) < 0) { return -1; }
    if (add_uint_constant(module, "FLAG_INTERRUPT_DISABLE", QE6502_ABI_FLAG_I) < 0) { return -1; }
    if (add_uint_constant(module, "FLAG_DECIMAL", QE6502_ABI_FLAG_D) < 0) { return -1; }
    if (add_uint_constant(module, "FLAG_BREAK", QE6502_ABI_FLAG_B) < 0) { return -1; }
    if (add_uint_constant(module, "FLAG_UNUSED", QE6502_ABI_FLAG_UN) < 0) { return -1; }
    if (add_uint_constant(module, "FLAG_OVERFLOW", QE6502_ABI_FLAG_V) < 0) { return -1; }
    if (add_uint_constant(module, "FLAG_NEGATIVE", QE6502_ABI_FLAG_N) < 0) { return -1; }

    if (add_uint_constant(module, "TICK_ADDRESS_SHIFT", QE6502_ABI_TICK_ADDRESS_SHIFT) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_STATUS_SHIFT", QE6502_ABI_TICK_STATUS_SHIFT) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_BUS_SHIFT", QE6502_ABI_TICK_BUS_SHIFT) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_ADDRESS_MASK", PY_QE6502_TICK_ADDRESS_MASK) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_STATUS_MASK", PY_QE6502_TICK_STATUS_MASK) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_BUS_MASK", PY_QE6502_TICK_BUS_MASK) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_WRITING", QE6502_ABI_TICK_WRITING) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_FETCH", QE6502_ABI_TICK_FETCH) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_INTERNAL_RESET", QE6502_ABI_TICK_INTERNAL_RESET) < 0) { return -1; }
    if (add_uint_constant(module, "TICK_CPU_JAMMED", QE6502_ABI_TICK_CPU_JAMMED) < 0) { return -1; }

    return 0;
}

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_qe6502",
    "Fast CPython extension for the qe6502 ABI.",
    -1,
    module_methods,
    NULL,
    NULL,
    NULL,
    NULL,
};

PyMODINIT_FUNC
PyInit__qe6502(void)
{
    PyObject* module;

    if (check_abi_version() < 0) {
        return NULL;
    }

    CPU_type_init_runtime_slots();

    if (PyType_Ready(&Qe6502CPUType) < 0) {
        return NULL;
    }

    module = PyModule_Create(&moduledef);

    if (module == NULL) {
        return NULL;
    }

    Py_INCREF(&Qe6502CPUType);

    if (PyModule_AddObject(module, "CPU", (PyObject*)&Qe6502CPUType) < 0) {
        Py_DECREF(&Qe6502CPUType);
        Py_DECREF(module);
        return NULL;
    }

    if (add_constants(module) < 0) {
        Py_DECREF(module);
        return NULL;
    }

    return module;
}
