#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Types and type handlind

// Given 2 primitive types, return `1` if compatible, else `0`.
// `left` and `right` become either `0` or `A_WIDEN`.
// `onlyright`: only widen left to right.

int type_compatible(int *left, int *right, int onlyright)
{
  // Void not compatible with anything
  if ((*left == P_VOID) || (*right == P_VOID))
    return 0;

  // Same types are compatible
  if (*left == *right)
  {
    *left = *right = 0;
    return 1;
  }

  // Widen `P_CHAR` to `P_INT`
  if ((*left == P_CHAR) && (*right == P_INT))
  {
    *left = A_WIDEN;
    *right = 0;
    return 1;
  }

  if ((*left == P_INT) && (*right == P_CHAR))
  {
    if (onlyright) // For `A_ASSIGN`; don't let a `P_INT` go to a `P_CHAR` variable
      return 0;

    *left = 0;
    *right = A_WIDEN;
    return 1;
  }

  // Anything left is compatible
  *left = *right = 0;
  return 1;
}
