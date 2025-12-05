#ifndef ASCII_UTILS_HPP
#define ASCII_UTILS_HPP

#include <cstddef>

namespace vix::utils::ascii
{
    bool is_ascii(char c);
    bool is_printable_ascii(char c);
    bool is_digit_ascii(char c);
    bool is_alpha_ascii(char c);
    bool is_upper_ascii(char c);
    bool is_lower_ascii(char c);

    char to_upper_ascii(char c);
    char to_lower_ascii(char c);

    bool ascii_code(char c);

    void print_ascii_table(std::size_t columns = 16);
} // vix::utils::ascii

#endif