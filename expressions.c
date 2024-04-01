#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of expressions with Pratt parsing

// Parse a primary factor and return an AST node representing it
static struct ASTnode *primary(void)
{
  struct ASTnode *n;
  int id;

  switch (Token.token)
  {
  case T_INTLIT:
    n = mkastleaf(A_INTLIT, Token.intvalue);
    break;
  case T_IDENT:
    id = findglob(Text);
    if (id == -1)
      fatals("Unknown variable", Text);

    n = mkastleaf(A_IDENT, id); // ID is index in global symbol table
    break;
  default:
    fatald("Syntax error, token", Token.token);
  }

  scan(&Token);

  return n;
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
    fatald("Syntax error, token", tokentype);
    __builtin_unreachable();
  }
}

// Operator precedence for each token
// T_EOF T_PLUS T_MINUS T_STAR T_SLASH T_INTLIT T_SEMI T_PRINT
static int OpPrec[] = {0, 10, 10, 20, 20, 0};

// Check that we have a binary operator and return its precedence
static int op_precedence(int tokentype)
{
  int prec = OpPrec[tokentype];
  if (prec == 0)
  {
    fatald("Syntax error, token", tokentype);
  }
  return prec;
}

// Return an AST tree whose root is a binary operator
// `ptp` = previous token precedence
struct ASTnode *binexpr(int ptp)
{
  struct ASTnode *left, *right;
  int tokentype;

  // Get primary tree on the left. Fetch next token at the same time.
  left = primary();

  tokentype = Token.token;
  if (tokentype == T_SEMI)
    return left;

  // While current token precedence > previous token precedence...
  while (op_precedence(tokentype) > ptp)
  {
    // Read in the next integer literal or identifier
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
