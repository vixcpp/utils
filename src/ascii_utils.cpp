#include <vix/utils/ascii_utils.hpp>
#include <iostream>

namespace vix::utils::ascii
{
    bool is_ascii(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        return uc <= 127;
    }

    bool is_printable_ascii(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        return uc >= 32 && uc <= 126;
    }

    bool is_digit_ascii(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        return uc >= '0' && uc <= '9';
    }

    bool is_alpha_ascii(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        return ((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z'));
    }

    bool is_upper_ascii(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        return (uc >= 'A' && uc <= 'Z');
    }

    bool is_lower_ascii(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        return (uc >= 'a' && uc <= 'z');
    }

    char to_upper_ascii(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 'a' && uc <= 'z')
        {
            return uc - ('a' - 'A');
        }
        return c;
    }

    char to_lower_ascii(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 'A' && uc <= 'Z')
        {
            return uc + ('a' - 'A');
        }
        return c;
    }

    bool ascii_code(char c)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        return static_cast<int>(uc);
    }

    void print_ascii_code(std::size_t columns = 16)
    {
        if (columns == 0)
            columns = 16;

        std::cout << "Printable ASCII [32...126]\n";

        std::size_t count = 0;
        for (unsigned char c = 32; c <= 126; ++c)
        {
            std::cout << c << ' ';
            ++count;

            if (count % columns == 0)
            {
                std::cout << '\n';
            }
        }

        if (count % columns != 0)
        {
            std::cout << '\n';
        }

        std::cout << '\n';
    }
} // vix::utils::ascii