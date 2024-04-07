#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of declarations

// Parse the current token and return a primitive type enum value
int parse_type(int t)
{
  if (t == T_CHAR)
    return P_CHAR;
  if (t == T_INT)
    return P_INT;
  if (t == T_LONG)
    return P_LONG;
  if (t == T_VOID)
    return P_VOID;

  fatald("Illegal type, token", t);
  __builtin_unreachable();
}

// Parse the declaration of a variable.
// Ensure `int <identifier>;`
void variable_declaration(void)
{
  int id, type;

  type = parse_type(Token.token);
  scan(&Token);
  ident();
  id = addglob(Text, type, S_VARIABLE, 0); // Text contains last identifier, via `scanident()`
  genglobsym(id);
  semi();
}

// Parse the declaration of a function
struct ASTnode *function_declaration(void)
{
  struct ASTnode *tree, *finalstatement;
  int nameslot, type, endlabel;

  type = parse_type(Token.token);
  scan(&Token);
  ident();

  // Get a label ID for the end label, add the function to the symbol table,
  // and set the `Functionid` global to the function's symbol ID
  endlabel = genlabel();
  nameslot = addglob(Text, type, S_FUNCTION, endlabel);
  Functionid = nameslot;

  lparen();
  rparen();
  tree = compound_statement();

  // If the function doesn't return `void`, ensure the last AST
  // operation in the compound statement was a `return` statement
  if (type != P_VOID)
  {
    finalstatement = (tree->op == A_GLUE) ? tree->right : tree;
    if (finalstatement == NULL || finalstatement->op != A_RETURN)
      fatal("No `return` for function with non-void type");
  }

  return mkastunary(A_FUNCTION, type, tree, nameslot);
}
