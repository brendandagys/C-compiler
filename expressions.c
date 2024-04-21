#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of expressions with Pratt parsing

// Parse a list of 0+ comma-separated expressions and return an AST composed of
// `A_GLUE` nodes, with the left-hand child being the sub-tree of previous
// expressions (or NULL), and the right-hand child being the next expression.
// Each `A_GLUE` node will have its size field set to the number of expressions
// in the tree at this point. If no expressions are parsed, NULL is returned.
static struct ASTnode *expression_list(void)
{
  struct ASTnode *tree = NULL;
  struct ASTnode *child = NULL;
  int exprcount = 0;

  while (Token.token != T_RPAREN)
  {
    child = binexpr(0);
    exprcount++;

    tree = mkastnode(A_GLUE, P_NONE, tree, NULL, child, exprcount);

    switch (Token.token)
    {
    case T_COMMA:
      scan(&Token);
      break;
    case T_RPAREN:
      break;
    default:
      fatald("Unexpected token in expression list", Token.token);
    }
  }

  return tree;
}

// Parse a function call and return its AST
static struct ASTnode *funccall(void)
{
  struct ASTnode *tree;
  int id;

  if ((id = findsymbol(Text)) == -1)
  {
    fatals("Undeclared function", Text);
  }

  if (Symtable[id].stype != S_FUNCTION)
  {
    fatals("Not a function", Text);
  }

  lparen();
  tree = expression_list();

  // TODO: Check type of each argument against the function's prototype

  tree = mkastunary(A_FUNCCALL, Symtable[id].type, tree, id);
  rparen();
  return tree;
}

// Parse the index into an array and return an AST node for it
static struct ASTnode *array_access(void)
{
  struct ASTnode *left, *right;
  int id;

  if ((id = findsymbol(Text)) == -1 || Symtable[id].stype != S_ARRAY)
  {
    fatals("Undeclared array", Text);
  }

  left = mkastleaf(A_ADDR, Symtable[id].type, id);

  scan(&Token); // Get the '['
  right = binexpr(0);
  match(T_RBRACKET, "]");

  if (!inttype(right->type))
    fatal("Array index is not of integer type");

  right = modify_type(right, left->type, A_ADD); // Scale index by size of element's type

  // Return an AST node where the array's base has the offset added to it.
  // Dereference the element. It's still an l-value at this point.
  left = mkastnode(A_ADD, Symtable[id].type, left, NULL, right, 0);
  left = mkastunary(A_DEREF, value_at(left->type), left, 0);
  return left;
}

// Parse a postfix expression and return an AST node representing it.
// The identifier is already in `Text`.
static struct ASTnode *postfix(void)
{
  struct ASTnode *n;
  int id;

  scan(&Token); // Either a function call, array index, or variable

  if (Token.token == T_LPAREN)
    return funccall();

  if (Token.token == T_LBRACKET)
    return array_access();

  if ((id = findsymbol(Text)) == -1 || Symtable[id].stype != S_VARIABLE)
    fatals("Unknown variable", Text);

  switch (Token.token)
  {
  case T_INC:
    scan(&Token);
    n = mkastleaf(A_POSTINC, Symtable[id].type, id);
    break;
  case T_DEC:
    scan(&Token);
    n = mkastleaf(A_POSTDEC, Symtable[id].type, id);
    break;
  default:
    n = mkastleaf(A_IDENT, Symtable[id].type, id);
  }

  return n;
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

  case T_STRLIT:
    id = genglobalstr(Text);
    n = mkastleaf(A_STRLIT, P_CHARPTR, id); // `id` is the string's label
    break;

  case T_IDENT:
    return postfix();

  case T_LPAREN: // Parenthesised expression
    scan(&Token);
    n = binexpr(0);
    rparen();
    return n;

  default:
    fatald("Syntax error, token", Token.token);
    __builtin_unreachable();
  }

  scan(&Token);
  return n;
}

// Convert a binary operator token into a binary AST operation.
// We rely on a 1:1 mapping from token to AST operation.
static int binastop(int tokentype)
{
  if (tokentype > T_EOF && tokentype <= T_SLASH)
    return tokentype;
  fatald("Syntax error, token", tokentype);
  __builtin_unreachable();
}

// Return true if a token is right-associative
static int rightassoc(int tokentype)
{
  return (tokentype == T_ASSIGN) ? 1 : 0;
}

// Operator precedence for each token. MUST MATCH ORDER OF TOKENS (`definitions.h`).
static int OpPrec[] = {
    0, 10, 20, 30,  // T_EOF, T_ASSIGN, T_LOGOR, T_LOGAND
    40, 50, 60,     // T_OR, T_XOR, T_AMPER
    70, 70,         // T_EQ, T_NE
    80, 80, 80, 80, // T_LT, T_GT, T_LE, T_GE
    90, 90,         // T_LSHIFT, T_RSHIFT
    100, 100,       // T_PLUS, T_MINUS
    110, 110,       // T_STAR, T_SLASH
};

// Check that we have a binary operator and return its precedence
static int op_precedence(int tokentype)
{
  if (tokentype > T_SLASH)
    fatald("Token with no precedence in `op_precedence()`:", tokentype);

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
    // `tree` is an l-value...we want address of the variable, not its value
    break;
  case T_STAR:
    scan(&Token);
    tree = prefix();

    if (tree->op != A_IDENT && tree->op != A_DEREF)
      fatal("* operator must be followed by an identifier or *");

    tree = mkastunary(A_DEREF, value_at(tree->type), tree, 0); // Parent node...
    break;
  case T_MINUS:
    scan(&Token);
    tree = prefix();
    tree->rvalue = 1;
    // Widen to int so that it's signed (char may not be). Must be signed to negate.
    tree = modify_type(tree, P_INT, 0);
    tree = mkastunary(A_NEGATE, tree->type, tree, 0);
    break;
  case T_INVERT:
    scan(&Token);
    tree = prefix();
    tree->rvalue = 1;
    tree = mkastunary(A_INVERT, tree->type, tree, 0);
    break;
  case T_LOGNOT:
    scan(&Token);
    tree = prefix();
    tree->rvalue = 1;
    tree = mkastunary(A_LOGNOT, tree->type, tree, 0);
    break;
  case T_INC:
    scan(&Token);
    tree = prefix();

    if (tree->op != A_IDENT)
      fatal("++ operator must be followed by an identifier");

    tree = mkastunary(A_PREINC, tree->type, tree, 0);
    break;
  case T_DEC:
    scan(&Token);
    tree = prefix();

    if (tree->op != A_IDENT)
      fatal("-- operator must be followed by an identifier");

    tree = mkastunary(A_PREDEC, tree->type, tree, 0);
    break;
  default:
    tree = primary(); // Identifier or integer literal...
  }

  return tree;
}

// Return an AST tree whose root is a binary operator
struct ASTnode *binexpr(int ptp) // `ptp`: previous token precedence
{
  struct ASTnode *left, *right;
  struct ASTnode *ltemp, *rtemp;
  int ASTop;
  int tokentype;

  // Get primary tree on the left. Fetch next token at the same time.
  left = prefix();

  tokentype = Token.token;
  if (tokentype == T_SEMI || tokentype == T_RPAREN || tokentype == T_RBRACKET)
  {
    left->rvalue = 1;
    return left;
  }

  // While current token precedence > previous token precedence...
  while ((op_precedence(tokentype) > ptp) ||
         (rightassoc(tokentype) && op_precedence(tokentype) == ptp))
  {
    // Read in the next integer literal or identifier
    scan(&Token);

    // Recursively build a sub-tree with binexpr(<token precedence>)
    right = binexpr(OpPrec[tokentype]);

    ASTop = binastop(tokentype);

    if (ASTop == A_ASSIGN)
    {
      right->rvalue = 1; // Mark the right as an r-value
      right = modify_type(right, left->type, 0);
      if (right == NULL)
        fatal("Incompatible expression in assignment");

      ltemp = left; // Swap
      left = right;
      right = ltemp;
    }
    else
    {
      // Not doing an assignment, so both trees should be r-values...
      left->rvalue = 1;
      right->rvalue = 1;

      ltemp = modify_type(left, right->type, ASTop);
      rtemp = modify_type(right, left->type, ASTop);

      if (ltemp == NULL && rtemp == NULL)
        fatal("Incompatible types in binary expression");

      if (ltemp != NULL) // Left <= right
        left = ltemp;
      if (rtemp != NULL) // Right <= left
        right = rtemp;
    }
    // Join that sub-tree with the left-hand sub-tree.
    // Convert the token into an AST operation at the same time.
    left = mkastnode(binastop(tokentype), left->type, left, NULL, right, 0);

    tokentype = Token.token; // Update details of current token
    if (tokentype == T_SEMI || tokentype == T_RPAREN || tokentype == T_RBRACKET)
    {
      left->rvalue = 1;
      return left;
    }
  }

  // Return the current tree when current precedence <= previous precedence
  left->rvalue = 1;
  return left;
}
