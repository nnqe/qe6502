#include "setup_input.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace perfect6502_debug
{
namespace
{
constexpr std::uint32_t ByteMax = 0xffu;
constexpr std::uint32_t WordMax = 0xffffu;
constexpr std::uint32_t AddressSpaceSize = 0x10000u;

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

    bool parse(SetupInput& setup, std::string& error)
    {
        setup = SetupInput();

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

            if (!parse_statement(setup, error))
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
                error_here(error, "expected ';' after setup statement");
                return false;
            }
        }
    }

private:
    bool parse_statement(SetupInput& setup, std::string& error)
    {
        const std::string name = parse_name();
        if (name.empty())
        {
            error_here(error, "expected setup target name");
            return false;
        }

        if (name == "mem")
        {
            return parse_memory_write(setup, error);
        }

        if ((name == "pc") || (name == "a") || (name == "x") ||
            (name == "y") || (name == "s") || (name == "p"))
        {
            return parse_register_write(setup, name, error);
        }

        error_here(error, "unknown setup target '" + name + "'");
        return false;
    }

    bool parse_register_write(SetupInput& setup, const std::string& name, std::string& error)
    {
        if (!expect('=', error))
        {
            return false;
        }

        const std::uint32_t max_value = (name == "pc") ? WordMax : ByteMax;
        std::uint32_t value = 0u;
        if (!parse_number(max_value, value, error))
        {
            return false;
        }

        if (name == "pc")
        {
            setup.registers.pc = static_cast<std::uint16_t>(value);
        }
        else if (name == "a")
        {
            setup.registers.a = static_cast<std::uint8_t>(value);
        }
        else if (name == "x")
        {
            setup.registers.x = static_cast<std::uint8_t>(value);
        }
        else if (name == "y")
        {
            setup.registers.y = static_cast<std::uint8_t>(value);
        }
        else if (name == "s")
        {
            setup.registers.s = static_cast<std::uint8_t>(value);
        }
        else
        {
            setup.registers.p = static_cast<std::uint8_t>(value);
        }

        return true;
    }

    bool parse_memory_write(SetupInput& setup, std::string& error)
    {
        if (!expect('[', error))
        {
            return false;
        }

        std::uint32_t address = 0u;
        if (!parse_number(WordMax, address, error))
        {
            return false;
        }

        if (!expect(']', error) || !expect('=', error) || !expect('{', error))
        {
            return false;
        }

        MemoryWrite write;
        write.address = static_cast<std::uint16_t>(address);

        skip_space();
        if (peek() == '}')
        {
            error_here(error, "memory write byte list must not be empty");
            return false;
        }

        while (true)
        {
            std::uint32_t byte_value = 0u;
            if (!parse_number(ByteMax, byte_value, error))
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

        if (!expect('}', error))
        {
            return false;
        }

        const std::uint32_t bytes_available = AddressSpaceSize - address;
        if (write.bytes.size() > bytes_available)
        {
            error_here(error, "memory write crosses the end of the 64K address space");
            return false;
        }

        setup.memory_writes.push_back(write);
        return true;
    }

    bool parse_number(std::uint32_t max_value, std::uint32_t& value, std::string& error)
    {
        skip_space();

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
        if (value > ((max_value - digit) / base))
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

SetupInput::SetupInput()
{
    registers.pc = 0x0200u;
    registers.a = 0x00u;
    registers.x = 0x00u;
    registers.y = 0x00u;
    registers.s = 0xfdu;
    registers.p = 0x24u;
    registers.ir = 0x00u;
}

bool parse_setup_input(const std::string& text, SetupInput& setup, std::string& error)
{
    Parser parser(text);
    return parser.parse(setup, error);
}

bool apply_setup_memory(PerfectMachine& machine, const SetupInput& setup, std::string& error)
{
    machine.fill_memory(setup.memory_fill_value);

    for (const MemoryWrite& write : setup.memory_writes)
    {
        if (write.bytes.empty())
        {
            error = "cannot apply an empty memory write";
            return false;
        }

        const std::uint32_t address = write.address;
        const std::uint32_t bytes_available = AddressSpaceSize - address;
        if (write.bytes.size() > bytes_available)
        {
            error = "memory write crosses the end of the 64K address space";
            return false;
        }

        for (std::size_t i = 0u; i < write.bytes.size(); ++i)
        {
            machine.write_memory(static_cast<std::uint16_t>(address + i), write.bytes[i]);
        }
    }

    return true;
}

} // namespace perfect6502_debug
