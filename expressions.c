#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of expressions with Pratt parsing

// For an INTLIT token (primary factor) only, make/return a leaf AST node
// for it and scan in the next token. Otherwise, syntax error.
static struct ASTnode *primary(void)
{
  struct ASTnode *n;

  switch (Token.token)
  {
  case T_INTLIT:
    n = mkastleaf(A_INTLIT, Token.intvalue);
    scan(&Token);
    return n;
  default:
    fprintf(stderr, "Syntax error on line %d, token %d\n", Line, Token.token);
    exit(1);
  }
}

// Convert a binary operator token into an AST operation
int arithop(int tokentype)
{
  switch (tokentype)
  {
  case T_PLUS:
    return A_ADD;
  case T_MINUS:
    return A_SUBTRACT;
  case T_STAR:
    return A_MULTIPLY;
  case T_SLASH:
    return A_DIVIDE;
  default:
    fprintf(stderr, "Syntax error on line %d, token %d\n", Line, tokentype);
    exit(1);
  }
}

// Operator precedence for each token
// T_EOF T_PLUS T_MINUS T_STAR T_SLASH T_INTLIT T_SEMI T_PRINT
static int OpPrec[8] = {0, 10, 10, 20, 20, 0, 0, 0};

// Check that we have a binary operator and return its precedence
static int op_precedence(int tokentype)
{
  int prec = OpPrec[tokentype];
  if (prec == 0)
  {
    fprintf(stderr, "Syntax error on line %d, token %d\n", Line, tokentype);
    exit(1);
  }
  return prec;
}

// Return an AST tree whose root is a binary operator
// `ptp` = previous token precedence
struct ASTnode *binexpr(int ptp)
{
  struct ASTnode *left, *right;
  int tokentype;

  // Get the integer literal on the left. Fetch next token at the same time.
  left = primary();

  tokentype = Token.token;
  if (tokentype == T_SEMI)
    return left;

  // While current token precedence > previous token precedence...
  while (op_precedence(tokentype) > ptp)
  {
    // Read in the next integer literal
    scan(&Token);

    // Recursively build a sub-tree with binexpr(<token precedence>)
    right = binexpr(OpPrec[tokentype]);

    // Join that sub-tree with the left-hand sub-tree
    left = mkastnode(arithop(tokentype), left, right, 0);

    // Update details of current token. If none left, return the left node.
    tokentype = Token.token;
    if (tokentype == T_SEMI)
      return left;
  }

  // Return the current tree when current precedence <= previous precedence
  return left;
}
