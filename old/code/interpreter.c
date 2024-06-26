#include "definitions.h"
#include "data.h"
#include "declarations.h"

// AST tree interpreter

// List of AST operators
static char *ASTop[] = {"+", "-", "*", "/"};

// Given an AST, interpret the operators in it and return a final value
int interpretAST(struct ASTnode *n)
{
  int leftval, rightval;

  if (n->left)
    leftval = interpretAST(n->left);
  if (n->right)
    rightval = interpretAST(n->right);

#ifdef DEBUG
  if (n->op == A_INTLIT)
    printf("int %d\n", n->intvalue);
  else
    printf("%d %s %d\n", leftval, ASTop[n->op], rightval);
#endif

  switch (n->op)
  {
  case A_ADD:
    return leftval + rightval;
  case A_SUBTRACT:
    return leftval - rightval;
  case A_MULTIPLY:
    return leftval * rightval;
  case A_DIVIDE:
    return leftval / rightval;
  case A_INTLIT:
    return n->intvalue;
  default:
    fprintf(stderr, "Unknown AST operator %d\n", n->op);
    exit(1);
  }
}
