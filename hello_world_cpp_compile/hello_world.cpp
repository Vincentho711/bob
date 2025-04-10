#include <iostream>
#include "simple_utils.h"

int main() {
    int a = 3;
    int b = 7;
    std::cout << "Sum of " << a << " and " << b << " : " << simple_utils::sum(a, b) << std::endl;
    std::cout << "Multiple of " << a << " and " << b << " : " << simple_utils::multiply(a, b) << std::endl;

    std::string text = "radar";
    std::cout << "Reversed: " << simple_utils::reverse_string(text) << std::endl;
    std::cout << "Is_palindrome ? " << (simple_utils::is_palindrome(text) ? "Yes" : "No") << std::endl;
    return 0;
}
