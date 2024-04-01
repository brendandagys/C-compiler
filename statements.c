#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of statements

void print_statement(void)
{
  struct ASTnode *tree;
  int reg;

  match(T_PRINT, "print");
  tree = binexpr(0);
  reg = genAST(tree, -1);
  genprintint(reg);
  genfreeregs();
  semi();
}

void assignment_statement(void)
{
  struct ASTnode *left, *right, *tree;
  int id;

  ident();

  if ((id = findglob(Text)) == -1)
  {
    fatals("Unknown variable", Text);
  }

  right = mkastleaf(A_LVIDENT, id);

  match(T_EQUALS, "=");

  left = binexpr(0);

  tree = mkastnode(A_ASSIGN, left, right, 0); // Assign left child to right child

  genAST(tree, -1);
  genfreeregs();

  semi();
}

// Parse one or more statements
void statements(void)
{
  while (1)
  {
    switch (Token.token)
    {
    case T_PRINT:
      print_statement();
      break;
    case T_INT:
      variable_declaration();
      break;
    case T_IDENT:
      assignment_statement();
      break;
    case T_EOF:
      return;
    default:
      fatald("Syntax error, token", Token.token);
    }
  }
}
