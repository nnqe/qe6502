#include "interrupt_tests.h"
#include <stdio.h>
#include <string.h>

typedef const char* (*interrupt_suite_fn)(uint8_t model);

typedef struct interrupt_suite
{
    const char* name;
    interrupt_suite_fn run;
} interrupt_suite;

typedef struct interrupt_model
{
    const char* name;
    uint8_t id;
    int enabled;
} interrupt_model;

static const interrupt_suite suites[] = {
    { "irq",      qe6502_interrupt_irq_tests },
    { "nmi",      qe6502_interrupt_nmi_tests },
    { "combined", qe6502_interrupt_combined_tests },
    { "nmos_quirks", qe6502_interrupt_nmos_quirk_tests },
    { "brk_nmi_hijack", qe6502_interrupt_nmos_brk_nmi_hijack_tests },
    { "brk_irq_hijack", qe6502_interrupt_nmos_brk_irq_hijack_tests },
    { "irq_nmi_hijack", qe6502_interrupt_nmos_irq_nmi_hijack_tests },
    { "lost_nmi", qe6502_interrupt_nmos_lost_nmi_tests },
    { "late_nmi", qe6502_interrupt_nmos_late_nmi_tests }
};

static const interrupt_model models[] = {
#if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
    { "mos", QE6502_MODEL_MOS, 1 },
    { "nes", QE6502_MODEL_NES, 1 },
#else
    { "mos", QE6502_MODEL_MOS, 0 },
    { "nes", QE6502_MODEL_NES, 0 },
#endif
#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
    { "wdc", QE6502_MODEL_WDC, 1 },
    { "rw",  QE6502_MODEL_RW,  1 },
    { "st",  QE6502_MODEL_ST,  1 }
#else
    { "wdc", QE6502_MODEL_WDC, 0 },
    { "rw",  QE6502_MODEL_RW,  0 },
    { "st",  QE6502_MODEL_ST,  0 }
#endif
};

static void print_usage(const char* exe)
{
    fprintf(stderr,
        "Usage: %s [irq|nmi|combined|nmos_quirks|brk_nmi_hijack|brk_irq_hijack|irq_nmi_hijack|lost_nmi|late_nmi|all] [mos|nes|wdc|rw|st|all]\n"
        "Default: %s all all\n",
        exe,
        exe);
}

static int name_matches(const char* selected, const char* name)
{
    return strcmp(selected, "all") == 0 || strcmp(selected, name) == 0;
}

static int known_suite(const char* selected)
{
    size_t i;
    if (strcmp(selected, "all") == 0)
    {
        return 1;
    }
    for (i = 0u; i < (sizeof(suites) / sizeof(suites[0])); ++i)
    {
        if (strcmp(selected, suites[i].name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int known_model(const char* selected)
{
    size_t i;
    if (strcmp(selected, "all") == 0)
    {
        return 1;
    }
    for (i = 0u; i < (sizeof(models) / sizeof(models[0])); ++i)
    {
        if (strcmp(selected, models[i].name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    const char* suite_arg = "all";
    const char* model_arg = "all";
    int failures = 0;
    int selected_count = 0;
    size_t si;
    size_t mi;

    if (argc > 3)
    {
        print_usage(argv[0]);
        return 2;
    }
    if (argc >= 2)
    {
        suite_arg = argv[1];
    }
    if (argc >= 3)
    {
        model_arg = argv[2];
    }

    if (!known_suite(suite_arg) || !known_model(model_arg))
    {
        print_usage(argv[0]);
        return 2;
    }

    for (mi = 0u; mi < (sizeof(models) / sizeof(models[0])); ++mi)
    {
        if (!name_matches(model_arg, models[mi].name))
        {
            continue;
        }
        if (!models[mi].enabled)
        {
            printf("[SKIP] model=%s is not enabled in this build\n", models[mi].name);
            continue;
        }

        for (si = 0u; si < (sizeof(suites) / sizeof(suites[0])); ++si)
        {
            const char* fail;
            if (!name_matches(suite_arg, suites[si].name))
            {
                continue;
            }
            ++selected_count;
            fail = suites[si].run(models[mi].id);
            if (fail)
            {
                ++failures;
                printf("[FAIL] model=%s suite=%s: %s\n", models[mi].name, suites[si].name, fail);
            }
            else
            {
                printf("[PASS] model=%s suite=%s\n", models[mi].name, suites[si].name);
            }
        }
    }

    if (selected_count == 0)
    {
        fprintf(stderr, "No enabled tests matched the requested arguments.\n");
        return 2;
    }

    return failures == 0 ? 0 : 1;
}
