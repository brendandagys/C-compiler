#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Generic code generator

// Generate and return a new label number
int genlabel(void) {
  static int id = 1;
  return id++;
}

// Generate the code for an IF statement and an optional ELSE clause
static int genIF(struct ASTnode *n) {
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
  genAST(n->mid, NOLABEL, n->op);
  genfreeregs();

  // If ELSE clause, generate the jump to skip to the end
  if (n->right)
    cgjump(Lend);

  cglabel(Lfalse);

  // If ELSE clause, generate the false compound statement and `Lend` label
  if (n->right) {
    genAST(n->right, NOLABEL, n->op);
    genfreeregs();
    cglabel(Lend);
  }

  return NOREG;
}

// Generate the code for a WHILE statement and an optional ELSE clause
static int genWHILE(struct ASTnode *n) {
  int Lstart = genlabel();
  int Lend = genlabel();

  cglabel(Lstart);

  // Generate condition code followed by a zero jump to `Lend`.
  // We cheat by sending the `Lend` label as a register.
  genAST(n->left, Lend, n->op);
  genfreeregs();

  // Generate the compound statement in the loop body
  genAST(n->right, NOLABEL, n->op);
  genfreeregs();

  cgjump(Lstart);  // Jump back to condition
  cglabel(Lend);   // Jump here when condition is false

  return NOREG;
}

// Generate code to copy the arguments of a function call to its parameters.
// Then, call the function itself. Return the register with the return value.
static int gen_funccall(struct ASTnode *n) {
  struct ASTnode *gluetree = n->left;
  int reg, numargs = 0;

  // If there's a list of arguments, walk it from the last argument (right child) to the first
  while (gluetree) {
    reg = genAST(gluetree->right, NOLABEL, gluetree->op);
    cgcopyarg(reg, gluetree->v.size);

    if (numargs == 0)  // Keep the first (highest) number of arguments
      numargs = gluetree->v.size;

    genfreeregs();
    gluetree = gluetree->left;
  }

  // Call the function, clean up the stack (based on number of arguments), and return its result
  return cgcall(n->v.id, numargs);
}

// Given an AST node, the register (if any) holding the previous rvalue, and the
// AST op of the parent, recursively generate assembly code. Return the register
// with the final tree value.
int genAST(struct ASTnode *n, int label, int parentASTop) {
  int leftreg, rightreg;

  // Specific AST node handling
  switch (n->op) {
    case A_IF:
      return genIF(n);
    case A_WHILE:
      return genWHILE(n);
    case A_FUNCCALL:
      return gen_funccall(n);
    case A_GLUE:
      // Do each child statement. Free the registers after each child.
      genAST(n->left, NOLABEL, n->op);
      genfreeregs();
      genAST(n->right, NOLABEL, n->op);
      genfreeregs();
      return NOREG;
    case A_FUNCTION:
      cgfuncpreamble(n->v.id);
      genAST(n->left, NOLABEL, n->op);
      cgfuncpostamble(n->v.id);
      return NOREG;
  }

  // General AST node handling

  // Get left and right sub-tree values
  if (n->left)
    leftreg = genAST(n->left, NOLABEL, n->op);
  if (n->right)
    rightreg = genAST(n->right, NOLABEL, n->op);

  switch (n->op) {
    case A_ADD:
      return cgadd(leftreg, rightreg);
    case A_SUBTRACT:
      return cgsub(leftreg, rightreg);
    case A_MULTIPLY:
      return cgmul(leftreg, rightreg);
    case A_DIVIDE:
      return cgdiv(leftreg, rightreg);
    case A_AND:
      return cgand(leftreg, rightreg);
    case A_OR:
      return cgor(leftreg, rightreg);
    case A_XOR:
      return cgxor(leftreg, rightreg);
    case A_LSHIFT:
      return cgshl(leftreg, rightreg);
    case A_RSHIFT:
      return cgshr(leftreg, rightreg);
    case A_EQ:
    case A_NE:
    case A_LT:
    case A_GT:
    case A_LE:
    case A_GE:
      // If parent AST node is A_IF or A_WHILE, generate a compare followed by a jump.
      // Otherwise, compare registers and set one to 1 or 0 based on the comparison.
      if (parentASTop == A_IF || parentASTop == A_WHILE)
        return cgcompare_and_jump(n->op, leftreg, rightreg, label);
      else
        return cgcompare_and_set(n->op, leftreg, rightreg);
    case A_INTLIT:
      return cgloadint(n->v.intvalue, n->type);
    case A_STRLIT:
      return cgloadglobalstr(n->v.id);
    case A_IDENT:
      // Load value if an r-value or are being dereferenced
      if (n->rvalue || parentASTop == A_DEREF)
        return (Symtable[n->v.id].class == C_LOCAL)
                   ? cgloadlocal(n->v.id, n->op)
                   : cgloadglobal(n->v.id, n->op);
      else
        return NOREG;
    case A_ASSIGN:
      // Are we assigning to an identifier or through a pointer?
      switch (n->right->op) {
        case A_IDENT:
          return (Symtable[n->right->v.id].class == C_LOCAL)
                     ? cgstorlocal(leftreg, n->right->v.id)
                     : cgstoreglobal(leftreg, n->right->v.id);
        case A_DEREF:
          return cgstorederef(leftreg, rightreg, n->right->type);
        default:
          fatald("Can't `A_ASSIGN` in `genAST()`, op", n->op);
      }
    case A_WIDEN:  // `cgwiden()` does nothing, but leave this node for other hardware platforms
      // Widen child's type to the parent's type
      return cgwiden(leftreg, n->left->type, n->type);
    case A_RETURN:
      cgreturn(leftreg, Functionid);
      return NOREG;
    case A_ADDR:
      return cgaddress(n->v.id);
    case A_DEREF:
      if (n->rvalue)  // Dereference to get the value pointed at
        return cgderef(leftreg, n->left->type);
      else  // Leave for `A_ASSIGN` to store through the pointer
        return leftreg;
    case A_SCALE:
      // Small optimization: use bit-shift if scale value is a known power of 2
      switch (n->v.size) {
        case 2:
          return cgshlconst(leftreg, 1);
        case 4:
          return cgshlconst(leftreg, 2);
        case 8:
          return cgshlconst(leftreg, 3);
        default:
          // Load a register with the size and multiple `leftreg` by it
          rightreg = cgloadint(n->v.size, P_INT);
          return cgmul(leftreg, rightreg);
      }
    case A_POSTINC:
      return cgloadglobal(n->v.id, n->op);
    case A_POSTDEC:
      return cgloadglobal(n->v.id, n->op);
    case A_PREINC:
      return cgloadglobal(n->left->v.id, n->op);
    case A_PREDEC:
      return cgloadglobal(n->left->v.id, n->op);
    case A_NEGATE:
      return cgnegate(leftreg);
    case A_INVERT:
      return cginvert(leftreg);
    case A_LOGNOT:
      return cglognot(leftreg);
    case A_TOBOOL:
      // If the parent AST node is an `A_IF` or `A_WHILE`, generate
      // a compare followed by a jump. Otherwise, set the register
      // to 0 or 1 based on it's zero-ness or non-zero-ness.
      return cgboolean(leftreg, parentASTop, label);
    default:
      fatald("Unknown AST operator", n->op);
      __builtin_unreachable();
  }
}

void genpreamble(void) { cgpreamble(); }

void genpostamble(void) { cgpostamble(); }

void genfreeregs(void) { freeall_registers(); }

void genglobalsym(int id) { cgglobalsym(id); }

int genglobalstr(char *strvalue) {
  int l = genlabel();
  cgglobalstr(l, strvalue);
  return l;
}

int genprimsize(int type) { return cgprimsize(type); }
