#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of statements

// Prototypes
static struct ASTnode *single_statement(void);

// Parse an IF statement, including an optional ELSE clause, and return its AST
static struct ASTnode *if_statement(void)
{
  struct ASTnode *condAST, *trueAST, *falseAST = NULL;

  match(T_IF, "if");
  lparen();

  condAST = binexpr(0);

  if (condAST->op < A_EQ || condAST->op > A_GE)
    condAST = mkastunary(A_TOBOOL, condAST->type, condAST, 0);
  rparen();

  trueAST = compound_statement();

  if (Token.token == T_ELSE)
  {
    scan(&Token);
    falseAST = compound_statement();
  }

  return mkastnode(A_IF, P_NONE, condAST, trueAST, falseAST, 0);
}

// Parse a WHILE statement and return its AST
static struct ASTnode *while_statement(void)
{
  struct ASTnode *condAST, *bodyAST;

  match(T_WHILE, "while");
  lparen();

  condAST = binexpr(0);
  if (condAST->op < A_EQ || condAST->op > A_GE)
    condAST = mkastunary(A_TOBOOL, condAST->type, condAST, 0);
  rparen();

  bodyAST = compound_statement();

  return mkastnode(A_WHILE, P_NONE, condAST, NULL, bodyAST, 0);
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
    condAST = mkastunary(A_TOBOOL, condAST->type, condAST, 0);
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
  tree = mkastnode(A_GLUE, P_NONE, bodyAST, NULL, postopAST, 0);
  // Make a WHILE loop with the condition and the body
  tree = mkastnode(A_WHILE, P_NONE, condAST, NULL, tree, 0);
  // Glue the pre-op tree and WHILE tree together
  return mkastnode(A_GLUE, P_NONE, preopAST, NULL, tree, 0);
}

// Parse a return statement and return its AST
static struct ASTnode *return_statement(void)
{
  struct ASTnode *tree;

  if (Symtable[Functionid].type == P_VOID)
    fatal("Can't return from a `void` function");

  match(T_RETURN, "return");
  lparen();

  tree = binexpr(0);
  tree = modify_type(tree, Symtable[Functionid].type, 0);

  if (tree == NULL)
    fatal("Incompatible type to return");

  tree = mkastunary(A_RETURN, P_NONE, tree, 0);

  rparen();
  return tree;
}

// Parse a single statement and return its AST
static struct ASTnode *single_statement(void)
{
  int type;

  switch (Token.token)
  {
  case T_CHAR:
  case T_INT:
  case T_LONG:
    // Beginning of a variable declaration.
    // Parse the type and get the identifier.
    // Then, parse the rest of the declaration.
    // CURRENTLY THESE ARE GLOBALS.
    type = parse_type();
    ident();
    variable_declaration(type, 1, 0);
    semi();
    return NULL; // No AST generated
  case T_IF:
    return if_statement();
  case T_WHILE:
    return while_statement();
  case T_FOR:
    return for_statement();
  case T_RETURN:
    return return_statement();
  default:
    // For now, see if this is an expression. This catches assignment statements.
    return binexpr(0);
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
    if (tree != NULL && (tree->op == A_ASSIGN ||
                         tree->op == A_RETURN ||
                         tree->op == A_FUNCCALL))
      semi();

    // For each new tree, either save it in left (if empty), or glue current
    // left and the new tree together
    if (tree != NULL)
    {
      if (left == NULL)
        left = tree;
      else
        left = mkastnode(A_GLUE, P_NONE, left, NULL, tree, 0);
    }

    // When we reach this token, skip past it and return the AST
    if (Token.token == T_RBRACE)
    {
      rbrace();
      return left;
    }
  }
}
