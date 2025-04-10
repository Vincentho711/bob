#ifndef SIMPLE_UTILS_H
#define SIMPLE_UTILS_H

#include <string>

namespace simple_utils {
    // Adds two integers
    int sum(int a, int b);

    // Multiplies two integers
    int multiply(int a, int b);

    // Reverses a string
    std::string reverse_string(const std::string& input);

    // Checks if a string is a palindrome
    bool is_palindrome(const std::string& input);
}

#endif // SIMPLE_UTILS_H
