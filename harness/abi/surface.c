#include <stdio.h>
#include <string.h>

#define QE6502_ABI_SURFACE_MAX_FILE_SIZE 131072u
#define QE6502_ABI_SURFACE_MAX_SYMBOLS 64u
#define QE6502_ABI_SURFACE_MAX_SYMBOL_LENGTH 64u

static const char *const qe6502abi_expected[] = {
    "qe6502abi_get_a",
    "qe6502abi_get_irq",
    "qe6502abi_get_model",
    "qe6502abi_get_p",
    "qe6502abi_get_pc",
    "qe6502abi_get_s",
    "qe6502abi_get_x",
    "qe6502abi_get_y",
    "qe6502abi_goto",
    "qe6502abi_load",
    "qe6502abi_nmi",
    "qe6502abi_reset",
    "qe6502abi_restart",
    "qe6502abi_save",
    "qe6502abi_set_a",
    "qe6502abi_set_irq",
    "qe6502abi_set_model",
    "qe6502abi_set_p",
    "qe6502abi_set_pc",
    "qe6502abi_set_s",
    "qe6502abi_set_x",
    "qe6502abi_set_y",
    "qe6502abi_setup",
    "qe6502abi_tick",
    "qe6502abi_version"
};

static int qe6502abi_is_ident(char ch)
{
    return ((ch >= 'A') && (ch <= 'Z')) ||
           ((ch >= 'a') && (ch <= 'z')) ||
           ((ch >= '0') && (ch <= '9')) ||
           (ch == '_');
}

static int qe6502abi_read_file(const char *path, char *buffer, size_t buffer_size)
{
    FILE *file;
    size_t read_count;

    if(fopen_s(&file, path, "rb") != 0) {
        file = NULL;
    }

    if(file == NULL) {
        fprintf(stderr, "failed to open %s\n", path);
        return 1;
    }

    read_count = fread(buffer, 1u, buffer_size - 1u, file);
    if(ferror(file) != 0) {
        fprintf(stderr, "failed to read %s\n", path);
        (void)fclose(file);
        return 1;
    }

    if(read_count == (buffer_size - 1u)) {
        int extra;
        extra = fgetc(file);
        if(extra != EOF) {
            fprintf(stderr, "file too large: %s\n", path);
            (void)fclose(file);
            return 1;
        }
    }

    buffer[read_count] = '\0';
    if(fclose(file) != 0) {
        fprintf(stderr, "failed to close %s\n", path);
        return 1;
    }

    return 0;
}

static int qe6502abi_symbol_seen(char symbols[QE6502_ABI_SURFACE_MAX_SYMBOLS][QE6502_ABI_SURFACE_MAX_SYMBOL_LENGTH],
                                 size_t count,
                                 const char *symbol)
{
    size_t i;
    for(i = 0u; i < count; ++i) {
        if(strcmp(symbols[i], symbol) == 0) {
            return 1;
        }
    }
    return 0;
}

static int qe6502abi_add_symbol(char symbols[QE6502_ABI_SURFACE_MAX_SYMBOLS][QE6502_ABI_SURFACE_MAX_SYMBOL_LENGTH],
                                size_t *count,
                                const char *start,
                                size_t length)
{
    size_t i;

    if(length >= QE6502_ABI_SURFACE_MAX_SYMBOL_LENGTH) {
        fprintf(stderr, "symbol name is too long\n");
        return 1;
    }

    if(*count >= QE6502_ABI_SURFACE_MAX_SYMBOLS) {
        fprintf(stderr, "too many ABI symbols\n");
        return 1;
    }

    for(i = 0u; i < length; ++i) {
        symbols[*count][i] = start[i];
    }
    symbols[*count][length] = '\0';

    if(!qe6502abi_symbol_seen(symbols, *count, symbols[*count])) {
        *count += 1u;
    }

    return 0;
}

static int qe6502abi_extract_symbols(const char *text,
                                     char symbols[QE6502_ABI_SURFACE_MAX_SYMBOLS][QE6502_ABI_SURFACE_MAX_SYMBOL_LENGTH],
                                     size_t *count)
{
    const char *cursor;
    const char *name;
    const char *end;
    const char *after;
    const char *line_start;
    const char *api;
    const char name_token[] = "qe6502abi_";

    *count = 0u;
    cursor = text;

    for(;;) {
        name = strstr(cursor, name_token);
        if(name == NULL) {
            break;
        }

        end = name;
        while(qe6502abi_is_ident(*end) != 0) {
            ++end;
        }

        after = end;
        while((*after == ' ') || (*after == '\t') || (*after == '\r') || (*after == '\n')) {
            ++after;
        }

        line_start = name;
        while((line_start > text) && (*(line_start - 1) != '\n')) {
            --line_start;
        }

        api = strstr(line_start, "QE6502_ABI_API");
        if((api != NULL) && (api < name) && (*after == '(')) {
            if(qe6502abi_add_symbol(symbols, count, name, (size_t)(end - name)) != 0) {
                return 1;
            }
        }

        cursor = end;
    }

    return 0;
}

static int qe6502abi_is_expected(const char *symbol)
{
    size_t i;
    for(i = 0u; i < (sizeof(qe6502abi_expected) / sizeof(qe6502abi_expected[0])); ++i) {
        if(strcmp(symbol, qe6502abi_expected[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static int qe6502abi_validate_symbols(const char *label,
                                      char symbols[QE6502_ABI_SURFACE_MAX_SYMBOLS][QE6502_ABI_SURFACE_MAX_SYMBOL_LENGTH],
                                      size_t count)
{
    size_t i;
    int failures = 0;

    for(i = 0u; i < count; ++i) {
        if(!qe6502abi_is_expected(symbols[i])) {
            fprintf(stderr, "%s has unexpected ABI symbol: %s\n", label, symbols[i]);
            failures += 1;
        }
    }

    for(i = 0u; i < (sizeof(qe6502abi_expected) / sizeof(qe6502abi_expected[0])); ++i) {
        if(!qe6502abi_symbol_seen(symbols, count, qe6502abi_expected[i])) {
            fprintf(stderr, "%s is missing ABI symbol: %s\n", label, qe6502abi_expected[i]);
            failures += 1;
        }
    }

    if(failures != 0) {
        return 1;
    }

    return 0;
}

static int qe6502abi_check_file(const char *label, const char *path)
{
    static char text[QE6502_ABI_SURFACE_MAX_FILE_SIZE];
    char symbols[QE6502_ABI_SURFACE_MAX_SYMBOLS][QE6502_ABI_SURFACE_MAX_SYMBOL_LENGTH];
    size_t count = 0u;

    if(qe6502abi_read_file(path, text, sizeof(text)) != 0) {
        return 1;
    }

    if(qe6502abi_extract_symbols(text, symbols, &count) != 0) {
        return 1;
    }

    if(qe6502abi_validate_symbols(label, symbols, count) != 0) {
        return 1;
    }

    printf("%s ABI surface OK: %lu symbols\n", label, (unsigned long)count);
    return 0;
}

int main(void)
{
    int result = 0;

    result |= qe6502abi_check_file("header", QE6502_ABI_HEADER_PATH);
    result |= qe6502abi_check_file("source", QE6502_ABI_SOURCE_PATH);

    return result == 0 ? 0 : 1;
}
