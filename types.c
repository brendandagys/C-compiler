#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Types and type handling

// Return true if a type is an int type of any size
int inttype(int type)
{
  return (type == P_CHAR || type == P_INT || type == P_LONG) ? 1 : 0;
}

// Return true if a type is a pointer type
static int ptrtype(int type)
{
  return (type == P_VOIDPTR || type == P_CHARPTR || type == P_INTPTR || type == P_LONGPTR) ? 1 : 0;
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

// Given an AST tree and a type that we want it to become, possibly modify the
// tree by widening or scaling so that it is compatible with this type. Return
// the original tree if no changes occurred, a modified tree, or NULL if the
// tree is not compatible with the given type.
// If this will be part of a binary operation, the AST op is not zero.
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op)
{
  int ltype;
  int lsize, rsize;

  ltype = tree->type;

  // Compare scalar int types
  if (inttype(ltype) && inttype(rtype))
  {
    if (ltype == rtype)
      return tree;

    lsize = genprimsize(ltype);
    rsize = genprimsize(rtype);

    if (lsize > rsize)
      return NULL;

    if (lsize < rsize)
      return mkastunary(A_WIDEN, rtype, tree, 0);
  }

  if (ptrtype(ltype)) // For pointers on the left
  {
    if (op == 0 && ltype == rtype) // Okay if same type on right and not doing a binary op
      return tree;
  }

  if (op == A_ADD || op == A_SUBTRACT) // Can only scale for these operations
  {
    // Left in the int; right is the pointer.
    if (inttype(ltype) && ptrtype(rtype))
    {
      rsize = genprimsize(value_at(rtype));
      if (rsize > 1)
        return mkastunary(A_SCALE, rtype, tree, rsize);
      else
        return tree;
    }
  }

  return NULL;
}
