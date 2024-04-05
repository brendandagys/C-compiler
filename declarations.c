#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of declarations

// Parse the declaration of a variable.
// Ensure `int <identifier>;`
void variable_declaration(void)
{
  match(T_INT, "int");
  ident();
  addglob(Text); // Text contains last identifier, via `scanident()`
  genglobsym(Text);
  semi();
}

// Parse the declaration of a function
struct ASTnode *function_declaration(void)
{
  struct ASTnode *tree;
  int nameslot;

  match(T_VOID, "void");
  ident();
  nameslot = addglob(Text);
  lparen();
  rparen();
  tree = compound_statement();
  return mkastunary(A_FUNCTION, tree, nameslot);
}
