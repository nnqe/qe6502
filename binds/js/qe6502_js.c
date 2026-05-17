#include <qe6502/qe6502.h>
#include <qe/api_private.h>
#include <qe/log.h>

#define QE6502_MEM_ALLOC_CAPACITY   (256)

QE_IMPORT("env", "qe6502ext_debug_log")
extern void qe6502ext_debug_log(const char* topic, const char* message);

QE_FFI_API(void)        qe6502_init_js(void);
QE_FFI_API(void)        qe6502_cpu_pool_reset(void);
QE_FFI_API(void*)       qe6502_cpu_alloc(void);
QE_FFI_API(void)        qe6502_cpu_free(void* ptr);

QE_STATIC_ASSERT(QE6502_MEM_ALLOC_CAPACITY > 0, "Memory pool should have at least 1 element capacity");

static qe6502_cpu_t memory_pool[QE6502_MEM_ALLOC_CAPACITY];
static qe6502_cpu_t* free_chunks[QE6502_MEM_ALLOC_CAPACITY];
static qe_bool is_free_lookup[QE6502_MEM_ALLOC_CAPACITY];
static size_t free_chunk_idx = 0;
static qe_bool is_mem_inited = qe_false;

static void call_js_logger(void* context, const char* topic, const char* message)
{
    (void)context;
    qe6502ext_debug_log(topic? topic : "", message? message : "");
}

QE_FFI_API_IMPL(void)
qe6502_init_js(void)
{
    qe6502_set_logger(&call_js_logger, QE_NULL);
    qe6502_cpu_pool_reset();
}

QE_FFI_API_IMPL(void)
qe6502_cpu_pool_reset(void)
{
    for(size_t i = 0; i < QE6502_MEM_ALLOC_CAPACITY; i++)
    {
        free_chunks[i] = memory_pool + i;
        is_free_lookup[i] = qe_true;
    }
    free_chunk_idx = QE6502_MEM_ALLOC_CAPACITY;
    is_mem_inited = qe_true;
    qe_log_info("Memory pool reset");
}

QE_FFI_API_IMPL(void*)
qe6502_cpu_alloc(void)
{
    if (!is_mem_inited)
    {
        qe6502_cpu_pool_reset();
    }
    if (0 == free_chunk_idx)
    {
        qe_log_error("Memory pool empty");
        return QE_NULL;
    }
    free_chunk_idx--;
    ptrdiff_t pool_idx = free_chunks[free_chunk_idx] - memory_pool;
    is_free_lookup[(size_t)pool_idx] = qe_false;

    qe_log_info("CPU allocated");
    return free_chunks[free_chunk_idx];
}

QE_FFI_API_IMPL(void)
qe6502_cpu_free(void* ptr)
{
    if (!is_mem_inited)
    {
        qe_log_error("Deallocating memory before memory init");
        return;
    }
    if (!ptr)
    {
        qe_log_warn("Deallocating null pointer");
        return;
    }

    uintptr_t ptr_int = (uintptr_t)ptr;
    uintptr_t pool_first = (uintptr_t)memory_pool;
    uintptr_t pool_end = (uintptr_t)(memory_pool + QE6502_MEM_ALLOC_CAPACITY);

    if (ptr_int < pool_first || ptr_int >= pool_end)
    {
        qe_log_error("Deallocating out of bounds pointer");
        return;
    }
    if(((ptr_int - pool_first) % sizeof(qe6502_cpu_t)) != 0)
    {
        qe_log_error("Deallocated pointer error");
        return;
    }
    ptrdiff_t pool_idx = (qe6502_cpu_t*)ptr - memory_pool;
    if (is_free_lookup[(size_t)pool_idx])
    {
        qe_log_warn("Multiple memory deallocation");
        return;
    }
    if (free_chunk_idx >= QE6502_MEM_ALLOC_CAPACITY)
    {
        qe_log_error("Memory pool full");
        return;
    }
    free_chunks[free_chunk_idx] = (qe6502_cpu_t*)ptr;
    is_free_lookup[(size_t)pool_idx] = qe_true;
    free_chunk_idx++;
    qe_log_info("CPU Deallocated");
}
