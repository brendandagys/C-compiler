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

// Given a primitive type, return its respective pointer type
int pointer_to(int type)
{
  int newtype;

  switch (type)
  {
  case P_VOID:
    newtype = P_VOIDPTR;
    break;
  case P_CHAR:
    newtype = P_CHARPTR;
    break;
  case P_INT:
    newtype = P_INTPTR;
    break;
  case P_LONG:
    newtype = P_LONGPTR;
    break;
  default:
    fatald("Unrecognized in `pointer_to()`: type", type);
    __builtin_unreachable();
  }

  return newtype;
}

// Given a primitive pointer type, return its respective primitive type
int value_at(int type)
{
  int newtype;

  switch (type)
  {
  case P_VOIDPTR:
    newtype = P_VOID;
    break; // TODO: return directly
  case P_CHARPTR:
    newtype = P_CHAR;
    break;
  case P_INTPTR:
    newtype = P_INT;
    break;
  case P_LONGPTR:
    newtype = P_LONG;
    break;
  default:
    fatald("Unrecognised in value_at: type", type);
    __builtin_unreachable();
  }

  return newtype;
}
