/*
 * MIT License
 *
 * Copyright (c) 2025 Nikolay Nedelchev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef QE_QE6502_FFI_H__
#define QE_QE6502_FFI_H__

#if defined(QE6502_ENABLE_FFI) && (QE6502_ENABLE_FFI == 1)

#include <qe_utils.h>

//

QE_FFI_API(size_t)
qe6502_ffi_cpu_size(void);

//

QE_FFI_API(uint8_t)
qe6502_ffi_cpu_create(void* cpu_memory, size_t memory_size);

QE_FFI_API(void)
qe6502_ffi_cpu_power_on(void* cpu, uint8_t model);

QE_FFI_API(void)
qe6502_ffi_cpu_tick(void* cpu);

//

QE_FFI_API(uint8_t)  qe6502_ffi_ok(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_ffi_needs_data(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_ffi_has_data(const void* cpu);
QE_FFI_API(void)     qe6502_ffi_feed_data(void* cpu, uint8_t byte);
QE_FFI_API(uint8_t)  qe6502_ffi_data(const void* cpu);
QE_FFI_API(uint16_t) qe6502_ffi_address(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_ffi_instr_done(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_ffi_started(const void* cpu);
QE_FFI_API(uint8_t)  qe6502_ffi_model(const void* cpu);

QE_FFI_API(uint8_t)  qe6502_ffi_nmi_pin(const void* cpu);
QE_FFI_API(void)     qe6502_ffi_nmi_hi(void* cpu);
QE_FFI_API(void)     qe6502_ffi_nmi_lo(void* cpu);

QE_FFI_API(uint8_t)  qe6502_ffi_irq_pin(const void* cpu);
QE_FFI_API(void)     qe6502_ffi_irq_hi(void* cpu);
QE_FFI_API(void)     qe6502_ffi_irq_lo(void* cpu);

#endif // QE6502_ENABLE_FFI
#endif // QE_QE6502_FFI_H__
