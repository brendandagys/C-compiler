#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Types and type handlind

// Given 2 primitive types, return `1` if compatible, else `0`.
// `left` and `right` become either `0` or `A_WIDEN`.
// `onlyright`: only widen left to right.

int type_compatible(int *left, int *right, int onlyright)
{
  int leftsize, rightsize;

  if (*left == *right)
  {
    *left = *right = 0;
    return 1;
  }

  leftsize = genprimsize(*left);
  rightsize = genprimsize(*right);

  // Types with zero size are not compatible with anything
  if ((leftsize == 0 || rightsize == 0))
    return 0;

  if (leftsize < rightsize)
  {
    *left = A_WIDEN;
    *right = 0;
    return 1;
  }

  if (rightsize < leftsize)
  {
    if (onlyright)
      return 0;

    *left = 0;
    *right = A_WIDEN;
    return 1;
  }

  // Anything left is the same size and thus is compatible
  *left = *right = 0;
  return 1;
}
