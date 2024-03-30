#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Generic code generator

// Given an AST, recursively generate assembly code
static int genAST(struct ASTnode *n)
{
  int leftreg, rightreg;

  if (n->left)
    leftreg = genAST(n->left);
  if (n->right)
    rightreg = genAST(n->right);

  switch (n->op)
  {
  case A_ADD:
    return cgadd(leftreg, rightreg);
  case A_SUBTRACT:
    return cgsub(leftreg, rightreg);
  case A_MULTIPLY:
    return cgmul(leftreg, rightreg);
  case A_DIVIDE:
    return cgdiv(leftreg, rightreg);
  case A_INTLIT:
    return cgload(n->intvalue);
  default:
    fprintf(stderr, "Unknown AST operator %d\n", n->op);
    exit(1);
  }
}

void generatecode(struct ASTnode *n)
{
  cgpreamble();
  int reg = genAST(n);
  cgprintint(reg);
  cgpostamble();
}
