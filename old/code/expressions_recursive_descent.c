#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of expressions with full recursive descent

/*
  A recursive descent parser with explicit operator precedence can be
  inefficient because of all the function calls needed to reach the
  right level of precedence. There also must be functions to deal with
  each level of operator precedence, so we end up with many lines of code.
*/

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
int arithop(int tok)
{
  switch (tok)
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
    fprintf(stderr, "Syntax error on line %d, token %d\n", Line, tok);
    exit(1);
  }
}

struct ASTnode *additive_expr(void);

// Return an AST tree whose root is a '*' or '/' binary operator
struct ASTnode *multiplicative_expr(void)
{
  struct ASTnode *left, *right;
  int tokentype;

  // Get the integer literal on the left.
  // Fetch the next token at the same time.
  left = primary();

  // If no tokens left, return only left node
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);

  // While the token is a '*' or '/'...
  while ((tokentype == T_STAR) || (tokentype == T_SLASH))
  {
    // Fetch in the next integer literal
    scan(&Token);
    right = primary();

    // Join that with the left integer literal
    left = mkastnode(arithop(tokentype), left, right, 0);

    // Update the details of the current token.
    // If no tokens left, return just the left node
    tokentype = Token.token;
    if (tokentype == T_EOF)
      break;
  }

  // Return whatever tree we have created
  return (left);
}

// Return an AST tree whose root is a '+' or '-' binary operator
struct ASTnode *additive_expr(void)
{
  struct ASTnode *left, *right;
  int tokentype;

  // Get the left sub-tree at a higher precedence than us
  left = multiplicative_expr();

  // If no tokens left, return just the left node
  tokentype = Token.token;
  if (tokentype == T_EOF)
    return (left);

  // Cache the '+' or '-' token type

  // Loop working on token at our level of precedence
  while (1)
  {
    // Fetch in the next integer literal
    scan(&Token);

    // Get the right sub-tree at a higher precedence than us
    right = multiplicative_expr();

    // Join the two sub-trees with our low-precedence operator
    left = mkastnode(arithop(tokentype), left, right, 0);

    // And get the next token at our precedence
    tokentype = Token.token;
    if (tokentype == T_EOF)
      break;
  }

  // Return whatever tree we have created
  return (left);
}

// Return an AST tree whose root is a binary operator
struct ASTnode *binexpr(int)
{
  return additive_expr();
}
