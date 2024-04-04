#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of statements

// Prototypes
static struct ASTnode *single_statement(void);

static struct ASTnode *print_statement(void)
{
  struct ASTnode *tree;
  int reg;

  match(T_PRINT, "print");
  tree = binexpr(0);
  tree = mkastunary(A_PRINT, tree, 0);
  // Semi handled in `compound_statement()`
  return tree;
}

static struct ASTnode *assignment_statement(void)
{
  struct ASTnode *left, *right, *tree;
  int id;

  ident();

  if ((id = findglob(Text)) == -1)
  {
    fatals("Unknown variable", Text);
  }

  right = mkastleaf(A_LVIDENT, id);
  match(T_ASSIGN, "=");
  left = binexpr(0);
  tree = mkastnode(A_ASSIGN, left, NULL, right, 0); // Assign left child to right child
  // Semi handled in `compound_statement()`
  return tree;
}

// Parse an IF statement, including an optional ELSE clause, and return its AST
static struct ASTnode *if_statement(void)
{
  struct ASTnode *condAST, *trueAST, *falseAST = NULL;

  match(T_IF, "if");
  lparen();

  condAST = binexpr(0);

  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("Bad comparison operator");
  rparen();

  trueAST = compound_statement();

  if (Token.token == T_ELSE)
  {
    scan(&Token);
    falseAST = compound_statement();
  }

  return mkastnode(A_IF, condAST, trueAST, falseAST, 0);
}

// Parse a WHILE statement and return its AST
static struct ASTnode *while_statement(void)
{
  struct ASTnode *condAST, *bodyAST;

  match(T_WHILE, "while");
  lparen();

  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("Bad comparison operator");
  rparen();

  bodyAST = compound_statement();

  return mkastnode(A_WHILE, condAST, NULL, bodyAST, 0);
}

static struct ASTnode *for_statement(void)
{
  struct ASTnode *condAST, *bodyAST, *preopAST, *postopAST, *tree;

  match(T_FOR, "for");
  lparen();
  preopAST = single_statement();
  semi();
  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    fatal("Bad comparison operator");
  semi();
  postopAST = single_statement();
  rparen();
  bodyAST = compound_statement();

  //          A_GLUE
  //         /     \
  //      preop   A_WHILE
  //              /    \
  //        decision   A_GLUE
  //                   /    \
  //             compound  postop

  // Glue the compound statement and post-op tree together
  tree = mkastnode(A_GLUE, bodyAST, NULL, postopAST, 0);
  // Make a WHILE loop with the condition and the body
  tree = mkastnode(A_WHILE, condAST, NULL, tree, 0);
  // Glue the pre-op tree and WHILE tree together
  return mkastnode(A_GLUE, preopAST, NULL, tree, 0);
}

// Parse a single statement and return its AST
static struct ASTnode *single_statement(void)
{
  switch (Token.token)
  {
  case T_PRINT:
    return print_statement();
  case T_INT:
    variable_declaration();
    return NULL; // No AST generated
  case T_IDENT:
    return assignment_statement();
  case T_IF:
    return if_statement();
  case T_WHILE:
    return while_statement();
  case T_FOR:
    return for_statement();
  default:
    fatald("Syntax error, token", Token.token);
    __builtin_unreachable();
  }
}

// Parse a compound statement and return its AST
struct ASTnode *compound_statement(void)
{
  struct ASTnode *left = NULL;
  struct ASTnode *tree;

  lbrace();

  while (1)
  {
    tree = single_statement();

    // Some statements must be followed by a semicolon
    if (tree != NULL && (tree->op == A_PRINT || tree->op == A_ASSIGN))
      semi();

    // For each new tree, either save it in left (if empty), or glue current
    // left and the new tree together
    if (tree != NULL)
    {
      if (left == NULL)
        left = tree;
      else
        left = mkastnode(A_GLUE, left, NULL, tree, 0);
    }

    // When we reach this token, skip past it and return the AST
    if (Token.token == T_RBRACE)
    {
      rbrace();
      return left;
    }
  }
}
