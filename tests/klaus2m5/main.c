#include <qe6502/qe6502.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

const char* test_klaus2m5(uint8_t cpu_model,
                          uint8_t* memory,
                          uint16_t success_address,
                          uint64_t expected_cycles,
                          uint8_t* result);

void copy_klaus2m5_image( uint8_t* dst, uint16_t* success_address, uint64_t* expected_cycles );
void copy_klaus2m5_extended_image( uint8_t* dst, uint16_t* success_address, uint64_t* expected_cycles );


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <qe6502/qe6502.h>

static void print_usage(const char* exe)
{
    fprintf(stderr,
        "Usage: %s <model> <test>\n"
        "\n"
        "Models:\n"
        "  mos       MOS 6502\n"
        "  wdc       WDC 65C02\n"
        "  rw        Rockwell 65C02\n"
        "  st        Synertek 65C02\n"
        "\n"
        "Tests:\n"
        "  standard\n"
        "  extended\n",
        exe
    );
}

static int model_enabled(const char* model)
{
    if (strcmp(model, "mos") == 0) {
#if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
        return 1;
#else
        return 0;
#endif
    }

    if (
        strcmp(model, "wdc") == 0 ||
        strcmp(model, "rw") == 0 ||
        strcmp(model, "st") == 0
    ) {
#if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
        return 1;
#else
        return 0;
#endif
    }

    return 0;
}

static int parse_model(const char* model, uint8_t* out_model)
{
    if (strcmp(model, "mos") == 0) {
        *out_model = QE6502_MODEL_MOS;
        return 1;
    }

    if (strcmp(model, "wdc") == 0) {
        *out_model = QE6502_MODEL_WDC;
        return 1;
    }

    if (strcmp(model, "rw") == 0) {
        *out_model = QE6502_MODEL_RW;
        return 1;
    }

    if (strcmp(model, "st") == 0) {
        *out_model = QE6502_MODEL_ST;
        return 1;
    }

    return 0;
}

static const char* model_display_name(const char* model)
{
    if (strcmp(model, "mos") == 0) {
        return "MOS 6502";
    }

    if (strcmp(model, "wdc") == 0) {
        return "WDC 65C02";
    }

    if (strcmp(model, "rw") == 0) {
        return "Rockwell 65C02";
    }

    if (strcmp(model, "st") == 0) {
        return "Synertek 65C02";
    }

    return "Unknown";
}

static int test_model(const char* exec_name, const char* model_arg, const char* test_arg)
{
    uint16_t success_address = 0;
    uint64_t expected_cycles = 0;
    uint8_t memory[0x10000];
    uint8_t result = 0;
    const char* msg = NULL;

    uint8_t parsed_model;

    if (!parse_model(model_arg, &parsed_model)) {
        fprintf(stderr, "Unknown model: %s\n\n", model_arg);
        print_usage(exec_name);
        return 1;
    }

    if (!model_enabled(model_arg)) {
        fprintf(stderr,
            "Model '%s' is not enabled in the current build.\n",
            model_arg
        );
        return 1;
    }

    if (
        strcmp(test_arg, "standard") != 0 &&
        strcmp(test_arg, "extended") != 0
    ) {
        fprintf(stderr, "Unknown test: %s\n\n", test_arg);
        print_usage(exec_name);
        return 1;
    }

    if (strcmp(test_arg, "extended") == 0 && strcmp(model_arg, "mos") == 0) {
        fprintf(stderr,
            "Extended test is only valid for 65C02 models, not MOS 6502.\n"
        );
        return 1;
    }

    if (strcmp(test_arg, "standard") == 0) {
        copy_klaus2m5_image(
            memory,
            &success_address,
            &expected_cycles
        );
    } else {
        copy_klaus2m5_extended_image(
            memory,
            &success_address,
            &expected_cycles
        );
    }

    msg = test_klaus2m5(
        parsed_model,
        memory,
        success_address,
        expected_cycles,
        &result
    );

    printf(
        "%s CPU %s test %s : %s\n",
        model_display_name(model_arg),
        test_arg,
        result ? "[PASS]" : "[FAIL]",
        msg
    );

    return result ? 0 : 1;
}

int main(int argc, char** argv)
{
    const char* exec_name = argv[0];

    if (argc == 1)
    {
        int failed = 0;
#       if defined(QE6502_ENABLE_NMOS_6502) && (QE6502_ENABLE_NMOS_6502 == 1)
            failed += test_model(exec_name, "mos", "standard");
#       endif

#       if defined(QE6502_ENABLE_CMOS_65C02) && (QE6502_ENABLE_CMOS_65C02 == 1)
            failed += test_model(exec_name, "wdc", "standard");
            failed += test_model(exec_name, "rw",  "standard");
            failed += test_model(exec_name, "st",  "standard");

            failed += test_model(exec_name, "wdc", "extended");
            failed += test_model(exec_name, "rw",  "extended");
#       endif
        return failed;
    }


    if (argc != 3)
    {
        print_usage(argv[0]);
        return 1;
    }

    const char* model_arg = argv[1];
    const char* test_arg = argv[2];
    return test_model(exec_name, model_arg, test_arg);
}
