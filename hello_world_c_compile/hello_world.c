#include <stdio.h>
#include "sum.h"
#include "subtract.h"

int main() {
  // printf() displays the string inside quotation
  printf("Hello, World!\n");
  int num1 = 2, num2 = 7;
  int add_result = add(num1, num2);
  printf("The sum of %d and %d is %d\n", num1, num2, add_result);

  int num3 = 4, num4 = 1;
  int subtract_result = subtract(num3, num4);
  printf("The subtraction of %d and %d is %d\n", num3, num4, subtract_result);
  return 0;
}
