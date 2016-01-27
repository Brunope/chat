#include "utils.h"

/**
 * Return a mod b, NOT a % b, which in C is the remainder of  a / b. For
 * example, mod(-1, 100) = 99, but -1 % 100 = -1.
 */
int mod(int a, int b)
{
  int r = a % b;
  return r < 0 ? r + b : r;
}
