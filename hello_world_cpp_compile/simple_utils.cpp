#include "simple_utils.h"
#include <algorithm>

namespace simple_utils {

    int sum(int a, int b) {
        return a + b;
    }

    int multiply(int a, int b) {
        return a * b;
    }

    std::string reverse_string(const std::string &input) {
        std::string reversed = input;
        std::reverse(reversed.begin(), reversed.end());
        return reversed;

    }

    bool is_palindrome(const std::string& input) {
        std::string reversed = reverse_string(input);
        return input == reversed;
    }

}
