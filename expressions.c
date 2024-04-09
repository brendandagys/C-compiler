#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of expressions with Pratt parsing

// Parse a function call with a single expression argument. Return its AST.
struct ASTnode *funccall(void)
{
  struct ASTnode *tree;
  int id;

  if ((id = findglob(Text)) == -1)
  {
    fatals("Undeclared function", Text);
  }

  if (Gsym[id].stype != S_FUNCTION)
  {
    fatals("Not a function", Text);
  }

  lparen();
  tree = binexpr(0);
  tree = mkastunary(A_FUNCCALL, Gsym[id].type, tree, id);
  rparen();
  return tree;
}

// Parse a primary factor and return an AST node representing it
static struct ASTnode *primary(void)
{
  struct ASTnode *n;
  int id;

  switch (Token.token)
  {
  case T_INTLIT:
    if ((Token.intvalue >= 0) && (Token.intvalue < 256))
      n = mkastleaf(A_INTLIT, P_CHAR, Token.intvalue);
    else
      n = mkastleaf(A_INTLIT, P_INT, Token.intvalue);
    break;

  case T_IDENT:
    // Could be a variable or a function call; scan in next token to find out
    scan(&Token);

    if (Token.token == T_LPAREN)
      return funccall();

    reject_token(&Token);

    if ((id = findglob(Text)) == -1)
      fatals("Unknown variable", Text);
    n = mkastleaf(A_IDENT, Gsym[id].type, id); // ID is index in global symbol table
    break;

  default:
    fatald("Syntax error, token", Token.token);
    __builtin_unreachable();
  }

  scan(&Token);

  return n;
}

// Convert a binary operator token into an AST operation.
// We rely on a 1:1 mapping from token to AST operation.
static int arithop(int tokentype)
{
  if (tokentype > T_EOF && tokentype < T_INTLIT)
    return tokentype;
  fatald("Syntax error, token", tokentype);
  __builtin_unreachable();
}

// Operator precedence for each token. MUST MATCH ORDER OF TOKENS (`definitions.h`).
static int OpPrec[] = {
    0, 10, 10,     // T_EOF, T_PLUS, T_MINUS
    20, 20,        // T_STAR, T_SLASH
    30, 30,        // T_EQ, T_NE
    40, 40, 40, 40 // T_LT, T_GT, T_LE, T_GE
};

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

// Parse a prefix expression and return an AST node representing it
static struct ASTnode *prefix(void)
{
  struct ASTnode *tree;

  switch (Token.token)
  {
  case T_AMPER:
    // Get next token and parse it recursively as a prefix expression
    scan(&Token);
    tree = prefix();

    if (tree->op != A_IDENT)
      fatal("& operator must be followed by an identifier");

    // Not a parent node; `A_IDENT` replaced with `A_ADDR`
    tree->op = A_ADDR;
    tree->type = pointer_to(tree->type);
    break;
  case T_STAR:
    scan(&Token);
    tree = prefix();

    if (tree->op != A_IDENT && tree->op != A_DEREF)
      fatal("* operator must be followed by an identifier or *");

    tree = mkastunary(A_DEREF, value_at(tree->type), tree, 0); // Parent node...
    break;
  default:
    tree = primary();
  }

  return tree;
}

// Return an AST tree whose root is a binary operator
struct ASTnode *binexpr(int ptp) // `ptp`: previous token precedence
{
  struct ASTnode *left, *right;
  int lefttype, righttype;
  int tokentype;

  // Get primary tree on the left. Fetch next token at the same time.
  left = prefix();

  tokentype = Token.token;
  if (tokentype == T_SEMI || tokentype == T_RPAREN)
    return left;

  // While current token precedence > previous token precedence...
  while (op_precedence(tokentype) > ptp)
  {
    // Read in the next integer literal or identifier
    scan(&Token);

    // Recursively build a sub-tree with binexpr(<token precedence>)
    right = binexpr(OpPrec[tokentype]);

    lefttype = left->type;
    righttype = right->type;

    if (!type_compatible(&lefttype, &righttype, 0))
      fatal("Incompatible types");

    if (lefttype) // Will be `A_WIDEN` or `0`
      left = mkastunary(lefttype, right->type, left, 0);
    if (righttype)
      right = mkastunary(righttype, left->type, right, 0);

    // Join that sub-tree with the left-hand sub-tree.
    // Convert the token into an AST operation at the same time.
    left = mkastnode(arithop(tokentype), left->type, left, NULL, right, 0);

    tokentype = Token.token; // Update details of current token
    if (tokentype == T_SEMI || tokentype == T_RPAREN)
      return left;
  }

  // Return the current tree when current precedence <= previous precedence
  return left;
}
