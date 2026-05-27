#include "trace_script.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace qe6502_trace
{
namespace
{
constexpr std::uint32_t ByteMax = 0xffu;
constexpr std::uint32_t WordMax = 0xffffu;
constexpr std::uint32_t AddressSpaceSize = 0x10000u;
constexpr std::uint32_t StepCountMax = 1000000000u;

bool is_space(char ch)
{
    return (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r');
}

bool is_digit(char ch)
{
    return (ch >= '0') && (ch <= '9');
}

bool is_hex_digit(char ch)
{
    return ((ch >= '0') && (ch <= '9')) ||
           ((ch >= 'a') && (ch <= 'f'));
}

bool is_binary_digit(char ch)
{
    return (ch == '0') || (ch == '1');
}

bool is_name_char(char ch)
{
    return (ch >= 'a') && (ch <= 'z');
}

char lower_ascii(char ch)
{
    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return static_cast<char>(ch - 'A' + 'a');
    }

    return ch;
}

std::uint32_t decimal_value(char ch)
{
    return static_cast<std::uint32_t>(ch - '0');
}

std::uint32_t hex_value(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
    {
        return static_cast<std::uint32_t>(ch - '0');
    }

    return static_cast<std::uint32_t>(ch - 'a' + 10);
}

std::string normalize_input(const std::string& text)
{
    std::string normalized;
    normalized.reserve(text.size());

    for (std::size_t i = 0u; i < text.size(); ++i)
    {
        if ((text[i] == '/') && ((i + 1u) < text.size()) && (text[i + 1u] == '/'))
        {
            i += 2u;
            while ((i < text.size()) && (text[i] != '\n'))
            {
                ++i;
            }

            if (i < text.size())
            {
                normalized.push_back('\n');
            }
        }
        else
        {
            normalized.push_back(lower_ascii(text[i]));
        }
    }

    return normalized;
}

class Parser
{
public:
    explicit Parser(const std::string& text) :
        text_(normalize_input(text))
    {
    }

    bool parse(TraceScript& script, std::string& error)
    {
        script.memory_writes.clear();
        script.commands.clear();

        while (true)
        {
            skip_space();

            while (consume(';'))
            {
                skip_space();
            }

            if (at_end())
            {
                return true;
            }

            if (!parse_statement(script, error))
            {
                return false;
            }

            skip_space();
            if (at_end())
            {
                return true;
            }

            if (!consume(';'))
            {
                error_here(error, "expected ';' after trace statement");
                return false;
            }
        }
    }

private:
    bool parse_statement(TraceScript& script, std::string& error)
    {
        const std::string name = parse_name();
        if (name.empty())
        {
            error_here(error, "expected trace command");
            return false;
        }

        if (name == "mem")
        {
            return parse_memory_write(script, error);
        }

        if (name == "begin")
        {
            TraceCommand command;
            command.type = TraceCommandType::Begin;
            script.commands.push_back(command);
            return true;
        }

        if ((name == "s") || (name == "step"))
        {
            return parse_step(script, error);
        }

        if ((name == "irq") || (name == "nmi"))
        {
            return parse_interrupt(script, name, error);
        }

        if (name == "reset")
        {
            TraceCommand command;
            command.type = TraceCommandType::Reset;
            script.commands.push_back(command);
            return true;
        }

        error_here(error, "unknown trace command '" + name + "'");
        return false;
    }

    bool parse_memory_write(TraceScript& script, std::string& error)
    {
        if (!expect('[', error))
        {
            return false;
        }

        std::uint32_t address = 0u;
        if (!parse_word_value(address, error))
        {
            return false;
        }

        if (!expect(']', error) || !expect('=', error) || !expect('{', error))
        {
            return false;
        }

        MemoryWrite write;
        write.address = static_cast<std::uint16_t>(address);

        if (!parse_memory_byte_list(write, error))
        {
            return false;
        }

        const std::uint32_t bytes_available = AddressSpaceSize - address;
        if (write.bytes.size() > bytes_available)
        {
            error_here(error, "memory write crosses the end of the 64K address space");
            return false;
        }

        script.memory_writes.push_back(write);
        return true;
    }

    bool parse_memory_byte_list(MemoryWrite& write, std::string& error)
    {
        skip_space();
        if (peek() == '}')
        {
            error_here(error, "memory write byte list must not be empty");
            return false;
        }

        while (true)
        {
            std::uint32_t byte_value = 0u;
            if (!parse_byte_value(byte_value, error))
            {
                return false;
            }

            write.bytes.push_back(static_cast<std::uint8_t>(byte_value));

            skip_space();
            if (consume(','))
            {
                continue;
            }

            break;
        }

        return expect('}', error);
    }

    bool parse_step(TraceScript& script, std::string& error)
    {
        std::uint32_t count = 0u;
        if (!parse_step_count(count, error))
        {
            return false;
        }

        if (count == 0u)
        {
            error_here(error, "step count must be greater than zero");
            return false;
        }

        TraceCommand command;
        command.type = TraceCommandType::Step;
        command.count = count;
        script.commands.push_back(command);
        return true;
    }

    bool parse_interrupt(TraceScript& script, const std::string& name, std::string& error)
    {
        std::uint32_t level = 0u;
        if (!parse_level(level, error))
        {
            return false;
        }

        TraceCommand command;
        command.type = (name == "irq") ? TraceCommandType::Irq : TraceCommandType::Nmi;
        command.level = static_cast<std::uint8_t>(level);
        script.commands.push_back(command);
        return true;
    }

    bool parse_level(std::uint32_t& value, std::string& error)
    {
        if (!parse_unsigned_number(1u, value, error))
        {
            return false;
        }

        return true;
    }

    bool parse_step_count(std::uint32_t& value, std::string& error)
    {
        return parse_unsigned_number(StepCountMax, value, error);
    }

    bool parse_word_value(std::uint32_t& value, std::string& error)
    {
        return parse_unsigned_number(WordMax, value, error);
    }

    bool parse_byte_value(std::uint32_t& value, std::string& error)
    {
        return parse_unsigned_number(ByteMax, value, error);
    }

    bool parse_unsigned_number(std::uint32_t max_value, std::uint32_t& value, std::string& error)
    {
        skip_space();

        value = 0u;
        std::uint32_t base = 10u;
        std::size_t first_digit = pos_;

        if ((peek() == '0') && (peek_next() == 'x'))
        {
            base = 16u;
            pos_ += 2u;
            first_digit = pos_;

            while (is_hex_digit(peek()))
            {
                const std::uint32_t digit = hex_value(peek());
                if (!append_digit(base, digit, max_value, value, error))
                {
                    return false;
                }
                ++pos_;
            }

            if (pos_ == first_digit)
            {
                error_here(error, "expected hexadecimal digits after 0x");
                return false;
            }

            return true;
        }

        if (peek() == 'b')
        {
            base = 2u;
            ++pos_;
            first_digit = pos_;

            while (is_binary_digit(peek()))
            {
                const std::uint32_t digit = decimal_value(peek());
                if (!append_digit(base, digit, max_value, value, error))
                {
                    return false;
                }
                ++pos_;
            }

            if (pos_ == first_digit)
            {
                error_here(error, "expected binary digits after b");
                return false;
            }

            return true;
        }

        while (is_digit(peek()))
        {
            const std::uint32_t digit = decimal_value(peek());
            if (!append_digit(base, digit, max_value, value, error))
            {
                return false;
            }
            ++pos_;
        }

        if (pos_ == first_digit)
        {
            error_here(error, "expected number");
            return false;
        }

        return true;
    }

    bool append_digit(std::uint32_t base,
                      std::uint32_t digit,
                      std::uint32_t max_value,
                      std::uint32_t& value,
                      std::string& error)
    {
        if ((digit > max_value) || (value > ((max_value - digit) / base)))
        {
            error_here(error, "number is outside the allowed range");
            return false;
        }

        value = static_cast<std::uint32_t>((value * base) + digit);
        return true;
    }

    std::string parse_name()
    {
        skip_space();

        const std::size_t start = pos_;
        while (is_name_char(peek()))
        {
            ++pos_;
        }

        return text_.substr(start, pos_ - start);
    }

    bool expect(char expected, std::string& error)
    {
        skip_space();
        if (consume(expected))
        {
            return true;
        }

        error_here(error, std::string("expected '") + expected + "'");
        return false;
    }

    bool consume(char expected)
    {
        if (peek() == expected)
        {
            ++pos_;
            return true;
        }

        return false;
    }

    char peek() const
    {
        if (pos_ >= text_.size())
        {
            return '\0';
        }

        return text_[pos_];
    }

    char peek_next() const
    {
        if ((pos_ + 1u) >= text_.size())
        {
            return '\0';
        }

        return text_[pos_ + 1u];
    }

    bool at_end() const
    {
        return pos_ >= text_.size();
    }

    void skip_space()
    {
        while (is_space(peek()))
        {
            ++pos_;
        }
    }

    void error_here(std::string& error, const std::string& message) const
    {
        error = message + " at input offset " + std::to_string(pos_);
    }

    std::string text_;
    std::size_t pos_ = 0u;
};

} // namespace

bool parse_trace_backend(const std::string& text, TraceBackend& backend)
{
    std::string normalized;
    normalized.reserve(text.size());
    for (char ch : text)
    {
        normalized.push_back(lower_ascii(ch));
    }

    if ((normalized == "perfect6502") || (normalized == "perfect"))
    {
        backend = TraceBackend::Perfect6502;
        return true;
    }

    if ((normalized == "qe6502") || (normalized == "qe"))
    {
        backend = TraceBackend::Qe6502;
        return true;
    }

    return false;
}

bool parse_trace_script(const std::string& text, TraceScript& script, std::string& error)
{
    Parser parser(text);
    return parser.parse(script, error);
}

const char* trace_backend_name(TraceBackend backend)
{
    switch (backend)
    {
        case TraceBackend::Perfect6502:
            return "perfect6502";
        case TraceBackend::Qe6502:
            return "qe6502";
    }

    return "unknown";
}

const char* trace_command_name(TraceCommandType type)
{
    switch (type)
    {
        case TraceCommandType::Begin:
            return "begin";
        case TraceCommandType::Step:
            return "s";
        case TraceCommandType::Irq:
            return "irq";
        case TraceCommandType::Nmi:
            return "nmi";
        case TraceCommandType::Reset:
            return "reset";
    }

    return "unknown";
}

} // namespace qe6502_trace
