// qe6502/_qe6502.c

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "qe6502.h"

#include <stdint.h>

#define PY_QE6502_ADDRESS_MASK 0x0000FFFFu
#define PY_QE6502_BUS_WRITE    0x00010000u
#define PY_QE6502_STARTING     0x00020000u
#define PY_QE6502_INSTR_DONE   0x00040000u
#define PY_QE6502_NOT_OK       0x00800000u
#define PY_QE6502_DATA_SHIFT   24u

typedef struct {
    PyObject_HEAD
    qe6502_cpu_t cpu;
    uint32_t packed;
    int powered_on;
} QE6502CPUObject;


static uint32_t
pack_state_slow(qe6502_cpu_t* cpu)
{
    uint32_t packed = 0;
    uint8_t has_data;
    uint8_t needs_data;
    uint8_t is_write;
    uint8_t data_out;

    packed |= (uint32_t)qe6502_address(cpu);

    has_data = qe6502_has_data(cpu) != 0;
    needs_data = qe6502_needs_data(cpu) != 0;

    is_write = has_data && !needs_data;

    if (is_write) {
        packed |= PY_QE6502_BUS_WRITE;
        data_out = qe6502_read_data(cpu);
        packed |= ((uint32_t)data_out << PY_QE6502_DATA_SHIFT);
    }

    if (!qe6502_is_started(cpu)) {
        packed |= PY_QE6502_STARTING;
    }

    if (qe6502_is_instr_done(cpu)) {
        packed |= PY_QE6502_INSTR_DONE;
    }

    if (!qe6502_ok(cpu)) {
        packed |= PY_QE6502_NOT_OK;
    }

    return packed;
}


static PyObject*
CPU_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    QE6502CPUObject* self;

    self = (QE6502CPUObject*)type->tp_alloc(type, 0);

    if (!self) {
        return NULL;
    }

    self->packed = 0;
    self->powered_on = 0;

    return (PyObject*)self;
}


static int
CPU_init(QE6502CPUObject* self, PyObject* args, PyObject* kwargs)
{
    int model = -1;

    static char* kwlist[] = {
        "model",
        NULL,
    };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &model)) {
        return -1;
    }

    if (model >= 0) {
        if (model > 255) {
            PyErr_SetString(PyExc_ValueError, "model must be in range 0..255");
            return -1;
        }

        qe6502_cpu_power_on(&self->cpu, (uint8_t)model);
        self->packed = pack_state_slow(&self->cpu);
        self->powered_on = 1;
    }

    return 0;
}


static PyObject*
CPU_power_on(QE6502CPUObject* self, PyObject* arg)
{
    unsigned long model;

    model = PyLong_AsUnsignedLong(arg);

    if (PyErr_Occurred()) {
        return NULL;
    }

    if (model > 255ul) {
        PyErr_SetString(PyExc_ValueError, "model must be in range 0..255");
        return NULL;
    }

    qe6502_cpu_power_on(&self->cpu, (uint8_t)model);
    self->packed = pack_state_slow(&self->cpu);
    self->powered_on = 1;

    Py_RETURN_NONE;
}


static PyObject*
CPU_tick_u8(QE6502CPUObject* self, PyObject* arg)
{
    unsigned long data_in;

    data_in = PyLong_AsUnsignedLong(arg);

    if (PyErr_Occurred()) {
        return NULL;
    }

    if (data_in > 255ul) {
        PyErr_SetString(PyExc_ValueError, "data_in must be in range 0..255");
        return NULL;
    }

    self->packed = qe6502_cpu_tick_ex(&self->cpu, (uint8_t)data_in);

    return PyLong_FromUnsignedLong((unsigned long)self->packed);
}


static PyObject*
CPU_tick0(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    self->packed = qe6502_cpu_tick_ex(&self->cpu, 0);

    return PyLong_FromUnsignedLong((unsigned long)self->packed);
}


static PyObject*
CPU_packed_state(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    return PyLong_FromUnsignedLong((unsigned long)self->packed);
}


static PyObject*
CPU_refresh_state(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    self->packed = pack_state_slow(&self->cpu);

    return PyLong_FromUnsignedLong((unsigned long)self->packed);
}


static PyObject*
CPU_dump_raw(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    uint64_t packed;

    packed = qe6502_dump(&self->cpu);

    return PyLong_FromUnsignedLongLong((unsigned long long)packed);
}


static PyObject*
CPU_recover_raw(QE6502CPUObject* self, PyObject* arg)
{
    unsigned long long packed;

    packed = PyLong_AsUnsignedLongLong(arg);

    if (PyErr_Occurred()) {
        return NULL;
    }

    qe6502_recover(&self->cpu, (uint64_t)packed);
    self->packed = pack_state_slow(&self->cpu);

    Py_RETURN_NONE;
}


static PyObject*
CPU_error_code(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    return PyLong_FromUnsignedLong((unsigned long)qe6502_error_code(&self->cpu));
}


static PyObject*
CPU_error_string(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    const char* message;

    message = qe6502_error_string(&self->cpu);

    if (!message) {
        message = "";
    }

    return PyUnicode_FromString(message);
}


static PyObject*
CPU_ok(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    if (qe6502_ok(&self->cpu)) {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}


static PyObject*
CPU_model(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    return PyLong_FromUnsignedLong((unsigned long)qe6502_model(&self->cpu));
}


static PyObject*
CPU_nmi_hi(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    qe6502_nmi_hi(&self->cpu);
    self->packed = pack_state_slow(&self->cpu);

    Py_RETURN_NONE;
}


static PyObject*
CPU_nmi_lo(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    qe6502_nmi_lo(&self->cpu);
    self->packed = pack_state_slow(&self->cpu);

    Py_RETURN_NONE;
}


static PyObject*
CPU_irq_hi(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    qe6502_irq_hi(&self->cpu);
    self->packed = pack_state_slow(&self->cpu);

    Py_RETURN_NONE;
}


static PyObject*
CPU_irq_lo(QE6502CPUObject* self, PyObject* Py_UNUSED(ignored))
{
    qe6502_irq_lo(&self->cpu);
    self->packed = pack_state_slow(&self->cpu);

    Py_RETURN_NONE;
}


static PyMethodDef CPU_methods[] = {
    {
        "power_on",
        (PyCFunction)CPU_power_on,
        METH_O,
        "Power on the CPU with the given model."
    },
    {
        "tick_u8",
        (PyCFunction)CPU_tick_u8,
        METH_O,
        "Run one tick with an input byte and return packed bus state."
    },
    {
        "tick0",
        (PyCFunction)CPU_tick0,
        METH_NOARGS,
        "Run one tick with input byte 0 and return packed bus state."
    },
    {
        "packed_state",
        (PyCFunction)CPU_packed_state,
        METH_NOARGS,
        "Return the cached packed bus state."
    },
    {
        "refresh_state",
        (PyCFunction)CPU_refresh_state,
        METH_NOARGS,
        "Refresh and return the packed bus state using the granular C API."
    },
    {
        "dump_raw",
        (PyCFunction)CPU_dump_raw,
        METH_NOARGS,
        "Return raw 64-bit CPU snapshot."
    },
    {
        "recover_raw",
        (PyCFunction)CPU_recover_raw,
        METH_O,
        "Recover CPU from raw 64-bit snapshot."
    },
    {
        "error_code",
        (PyCFunction)CPU_error_code,
        METH_NOARGS,
        "Return CPU error code."
    },
    {
        "error_string",
        (PyCFunction)CPU_error_string,
        METH_NOARGS,
        "Return CPU error string."
    },
    {
        "ok",
        (PyCFunction)CPU_ok,
        METH_NOARGS,
        "Return whether CPU is OK."
    },
    {
        "model",
        (PyCFunction)CPU_model,
        METH_NOARGS,
        "Return current CPU model."
    },
    {
        "nmi_hi",
        (PyCFunction)CPU_nmi_hi,
        METH_NOARGS,
        "Set NMI pin high."
    },
    {
        "nmi_lo",
        (PyCFunction)CPU_nmi_lo,
        METH_NOARGS,
        "Set NMI pin low."
    },
    {
        "irq_hi",
        (PyCFunction)CPU_irq_hi,
        METH_NOARGS,
        "Set IRQ pin high."
    },
    {
        "irq_lo",
        (PyCFunction)CPU_irq_lo,
        METH_NOARGS,
        "Set IRQ pin low."
    },
    {NULL, NULL, 0, NULL}
};


static PyTypeObject QE6502CPUType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_qe6502.CPU",
    .tp_basicsize = sizeof(QE6502CPUObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = CPU_new,
    .tp_init = (initproc)CPU_init,
    .tp_methods = CPU_methods,
};


static PyObject*
module_version(PyObject* self, PyObject* Py_UNUSED(ignored))
{
    return PyLong_FromUnsignedLong((unsigned long)qe6502_version());
}


static PyObject*
module_decode_error(PyObject* self, PyObject* arg)
{
    unsigned long code;
    const char* message;

    code = PyLong_AsUnsignedLong(arg);

    if (PyErr_Occurred()) {
        return NULL;
    }

    if (code > 0xFFFFul) {
        PyErr_SetString(PyExc_ValueError, "error code must be in range 0..65535");
        return NULL;
    }

    message = qe6502_decode_error((uint16_t)code);

    if (!message) {
        message = "";
    }

    return PyUnicode_FromString(message);
}


static PyMethodDef module_methods[] = {
    {
        "version",
        (PyCFunction)module_version,
        METH_NOARGS,
        "Return QE6502 version."
    },
    {
        "decode_error",
        (PyCFunction)module_decode_error,
        METH_O,
        "Decode QE6502 error code."
    },
    {NULL, NULL, 0, NULL}
};


static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_qe6502",
    "Fast CPython extension for QE6502.",
    -1,
    module_methods,
};


PyMODINIT_FUNC
PyInit__qe6502(void)
{
    PyObject* module;

    if (PyType_Ready(&QE6502CPUType) < 0) {
        return NULL;
    }

    module = PyModule_Create(&moduledef);

    if (!module) {
        return NULL;
    }

    Py_INCREF(&QE6502CPUType);

    if (PyModule_AddObject(module, "CPU", (PyObject*)&QE6502CPUType) < 0) {
        Py_DECREF(&QE6502CPUType);
        Py_DECREF(module);
        return NULL;
    }

    PyModule_AddIntConstant(module, "MODEL_MOS", QE6502_MODEL_MOS);
    PyModule_AddIntConstant(module, "MODEL_NES", QE6502_MODEL_NES);
    PyModule_AddIntConstant(module, "MODEL_WDC", QE6502_MODEL_WDC);
    PyModule_AddIntConstant(module, "MODEL_RW", QE6502_MODEL_RW);
    PyModule_AddIntConstant(module, "MODEL_ST", QE6502_MODEL_ST);

    PyModule_AddIntConstant(module, "ADDRESS_MASK", PY_QE6502_ADDRESS_MASK);
    PyModule_AddIntConstant(module, "BUS_WRITE", PY_QE6502_BUS_WRITE);
    PyModule_AddIntConstant(module, "STARTING", PY_QE6502_STARTING);
    PyModule_AddIntConstant(module, "INSTR_DONE", PY_QE6502_INSTR_DONE);
    PyModule_AddIntConstant(module, "NOT_OK", PY_QE6502_NOT_OK);
    PyModule_AddIntConstant(module, "DATA_SHIFT", PY_QE6502_DATA_SHIFT);

    return module;
}
