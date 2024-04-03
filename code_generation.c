#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Generic code generator

// Generate and return a new label number
static int label(void)
{
  static int id = 1;
  return id++;
}

// Generate the code for an IF statement and an optional ELSE clause
static int genIFAST(struct ASTnode *n)
{
  int Lfalse, Lend;

  // Generate 2 labels: one for the false compound statement, and one for the
  // end of the overall IF statement. When no ELSE, `Lfalse` is the ending
  // label.
  Lfalse = label();
  if (n->right)
    Lend = label();

  // Generate the condition code, followed by a zero jump to `Lfalse`.
  // We cheat by sending the `Lfalse` label as a register.
  genAST(n->left, Lfalse, n->op);
  genfreeregs();

  // Generate the true compound statement
  genAST(n->mid, NOREG, n->op);
  genfreeregs();

  // If ELSE clause, generate the jump to skip to the end
  if (n->right)
    cgjump(Lend);

  cglabel(Lfalse);

  // If ELSE clause, generate the false compound statement and `Lend` label
  if (n->right)
  {
    genAST(n->right, NOREG, n->op);
    genfreeregs();
    cglabel(Lend);
  }

  return NOREG;
}

// Given an AST node, the register (if any) holding the previous rvalue, and the
// AST op of the parent, recursively generate assembly code. Return the register
// with the final tree value.
int genAST(struct ASTnode *n, int reg, int parentASTop)
{
  int leftreg, rightreg;

  // Specific AST node handling
  switch (n->op)
  {
  case A_IF:
    return genIFAST(n);
  case A_GLUE:
    // Do each child statement. Free the registers after each child.
    genAST(n->left, NOREG, n->op);
    genfreeregs();
    genAST(n->right, NOREG, n->op);
    genfreeregs();
    return NOREG;
  }

  // General AST node handling

  // Get left and right sub-tree values
  if (n->left)
    leftreg = genAST(n->left, NOREG, n->op);
  if (n->right)
    rightreg = genAST(n->right, leftreg, n->op);

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
  case A_EQ:
  case A_NE:
  case A_LT:
  case A_GT:
  case A_LE:
  case A_GE:
    // If parent AST node is A_IF, generate a compare followed by a jump.
    // Otherwise, compare registers and set one to 1 or 0 based on the
    // comparison.
    if (parentASTop == A_IF)
      return cgcompare_and_jump(n->op, leftreg, rightreg, reg);
    else
      return cgcompare_and_set(n->op, leftreg, rightreg);
  case A_INTLIT:
    return cgloadint(n->v.intvalue);
  case A_IDENT:
    return cgloadglob(Gsym[n->v.id].name);
  case A_LVIDENT:
    return cgstorglob(reg, Gsym[n->v.id].name);
  case A_ASSIGN:
    return rightreg; // Work is already done; return result
  case A_PRINT:
    // Print left child's value and return no register
    genprintint(leftreg);
    genfreeregs();
    return NOREG;
  default:
    fatald("Unknown AST operator", n->op);
    __builtin_unreachable();
  }
}

void genpreamble(void) { cgpreamble(); }

void genpostamble(void) { cgpostamble(); }

void genfreeregs(void) { freeall_registers(); }

void genprintint(int reg) { cgprintint(reg); }

void genglobsym(char *s) { cgglobsym(s); }
