#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Generic code generator

// Generate and return a new label number
int genlabel(void)
{
  static int id = 1;
  return id++;
}

// Generate the code for an IF statement and an optional ELSE clause
static int genIF(struct ASTnode *n)
{
  int Lfalse, Lend;

  // Generate 2 labels: one for the false compound statement, and one for the
  // end of the overall IF statement. When no ELSE, `Lfalse` is the ending
  // label.
  Lfalse = genlabel();
  if (n->right)
    Lend = genlabel();

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

// Generate the code for a WHILE statement and an optional ELSE clause
static int genWHILE(struct ASTnode *n)
{
  int Lstart = genlabel();
  int Lend = genlabel();

  cglabel(Lstart);

  // Generate condition code followed by a zero jump to `Lend`.
  // We cheat by sending the `Lend` label as a register.
  genAST(n->left, Lend, n->op);
  genfreeregs();

  // Generate the compound statement in the loop body
  genAST(n->right, NOREG, n->op);
  genfreeregs();

  cgjump(Lstart); // Jump back to condition
  cglabel(Lend);  // Jump here when condition is false

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
    return genIF(n);
  case A_WHILE:
    return genWHILE(n);
  case A_GLUE:
    // Do each child statement. Free the registers after each child.
    genAST(n->left, NOREG, n->op);
    genfreeregs();
    genAST(n->right, NOREG, n->op);
    genfreeregs();
    return NOREG;
  case A_FUNCTION:
    cgfuncpreamble(n->v.id);
    genAST(n->left, NOREG, n->op);
    cgfuncpostamble(n->v.id);
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
    // If parent AST node is A_IF or A_WHILE, generate a compare followed by a jump.
    // Otherwise, compare registers and set one to 1 or 0 based on the comparison.
    if (parentASTop == A_IF || parentASTop == A_WHILE)
      return cgcompare_and_jump(n->op, leftreg, rightreg, reg);
    else
      return cgcompare_and_set(n->op, leftreg, rightreg);
  case A_INTLIT:
    return cgloadint(n->v.intvalue, n->type);
  case A_IDENT:
    return cgloadglob(n->v.id);
  case A_LVIDENT:
    return cgstorglob(reg, n->v.id);
  case A_ASSIGN:
    return rightreg; // Work is already done; return result
  case A_PRINT:
    // Print left child's value and return no register
    genprintint(leftreg);
    genfreeregs();
    return NOREG;
  case A_WIDEN: // `cgwiden()` does nothing, but leave this node for other hardware platforms
    // Widen child's type to the parent's type
    return cgwiden(leftreg, n->left->type, n->type);
  case A_RETURN:
    cgreturn(leftreg, Functionid);
    return NOREG;
  case A_FUNCCALL:
    return cgcall(leftreg, n->v.id);
  default:
    fatald("Unknown AST operator", n->op);
    __builtin_unreachable();
  }
}

void genpreamble(void) { cgpreamble(); }

void genpostamble(void) { cgpostamble(); }

void genfreeregs(void) { freeall_registers(); }

void genprintint(int reg) { cgprintint(reg); }

void genglobsym(int id) { cgglobsym(id); }

int genprimsize(int type) { return cgprimsize(type); }
