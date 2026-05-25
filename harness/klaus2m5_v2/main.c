#include "qe6502.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char* test_klaus2m5_v2(uint8_t cpu_model,
                             uint8_t* memory,
                             uint16_t success_address,
                             uint64_t expected_cycles,
                             uint8_t* result);

void copy_klaus2m5_image(uint8_t* dst, uint16_t* success_address, uint64_t* expected_cycles);
void copy_klaus2m5_extended_image(uint8_t* dst, uint16_t* success_address, uint64_t* expected_cycles);

static void print_usage(const char* exe)
{
    fprintf(stderr,
        "Usage: %s <model> <test> <cycle_reset>\n"
        "       %s\n"
        "\n"
        "With no arguments, runs the default v2 Klaus suite:\n"
        "  nmos standard off\n"
        "  wdc standard off\n"
        "  wdc extended off\n"
        "  rw standard off\n"
        "  rw extended off\n"
        "\n"
        "Models:\n"
        "  nmos      NMOS 6502 v2\n"
        "  wdc       WDC 65C02 v2\n"
        "  rw        Rockwell 65C02 v2\n"
        "\n"
        "Tests:\n"
        "  standard\n"
        "  extended\n"
        "\n"
        "Cycle Reset:\n"
        "  off       only supported mode for v2\n",
        exe,
        exe
    );
}

static int parse_model(const char* model, uint8_t* out_model)
{
    if (strcmp(model, "nmos") == 0 || strcmp(model, "mos") == 0)
    {
        *out_model = qe6502_model_nmos;
        return 1;
    }

    if (strcmp(model, "wdc") == 0)
    {
        *out_model = qe6502_model_wdc;
        return 1;
    }

    if (strcmp(model, "rw") == 0 || strcmp(model, "rockwell") == 0)
    {
        *out_model = qe6502_model_rw;
        return 1;
    }

    if (strcmp(model, "st") == 0 || strcmp(model, "synertek") == 0)
    {
        *out_model = qe6502_model_st;
        return 1;
    }

    return 0;
}

static const char* model_display_name(const char* model)
{
    if (strcmp(model, "nmos") == 0 || strcmp(model, "mos") == 0)
    {
        return "NMOS 6502 v2";
    }

    if (strcmp(model, "wdc") == 0)
    {
        return "WDC 65C02 v2";
    }

    if (strcmp(model, "rw") == 0 || strcmp(model, "rockwell") == 0)
    {
        return "Rockwell 65C02 v2";
    }

    if (strcmp(model, "st") == 0 || strcmp(model, "synertek") == 0)
    {
        return "Synertek 65C02 v2";
    }

    return "Unknown v2";
}

static int test_model(const char* exec_name,
                      const char* model_arg,
                      const char* test_arg,
                      const char* reset_cycle)
{
    uint16_t success_address = 0;
    uint64_t expected_cycles = 0;
    uint8_t memory[0x10000];
    uint8_t result = 0;
    const char* msg = NULL;
    uint8_t parsed_model = 0;

    if (!parse_model(model_arg, &parsed_model))
    {
        fprintf(stderr, "Unknown model: %s\n\n", model_arg);
        print_usage(exec_name);
        return 1;
    }

    if (strcmp(test_arg, "standard") != 0 &&
        strcmp(test_arg, "extended") != 0)
    {
        fprintf(stderr, "Unknown test: %s\n\n", test_arg);
        print_usage(exec_name);
        return 1;
    }

    if (strcmp(test_arg, "extended") == 0 &&
        parsed_model != qe6502_model_wdc &&
        parsed_model != qe6502_model_rw)
    {
        fprintf(stderr,
            "Extended test is only valid for WDC/Rockwell 65C02 v2 models, not %s.\n",
            model_display_name(model_arg)
        );
        return 1;
    }

    if (strcmp(reset_cycle, "off") != 0)
    {
        fprintf(stderr, "Cycle reset mode is not supported by klaus2m5_v2 yet.\n\n");
        print_usage(exec_name);
        return 1;
    }

    if (strcmp(test_arg, "standard") == 0)
    {
        copy_klaus2m5_image(
            memory,
            &success_address,
            &expected_cycles
        );
    }
    else
    {
        copy_klaus2m5_extended_image(
            memory,
            &success_address,
            &expected_cycles
        );
    }

    msg = test_klaus2m5_v2(
        parsed_model,
        memory,
        success_address,
        expected_cycles,
        &result
    );

    printf(
        "%s CPU %s test, reset: %s %s : normal %s\n",
        model_display_name(model_arg),
        test_arg,
        reset_cycle,
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

        failed += test_model(exec_name, "nmos", "standard", "off");
        failed += test_model(exec_name, "wdc", "standard", "off");
        failed += test_model(exec_name, "wdc", "extended", "off");
        failed += test_model(exec_name, "rw", "standard", "off");
        failed += test_model(exec_name, "rw", "extended", "off");
        failed += test_model(exec_name, "st", "standard", "off");

        return failed;
    }

    if (argc != 4)
    {
        print_usage(exec_name);
        return 1;
    }

    return test_model(exec_name, argv[1], argv[2], argv[3]);
}
