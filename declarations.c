#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of declarations

// Parse the current token and return a primitive type enum value.
// Also scan in the next token.
int parse_type(void)
{
  int type;
  switch (Token.token)
  {
  case T_VOID:
    type = P_VOID;
    break;
  case T_CHAR:
    type = P_CHAR;
    break;
  case T_INT:
    type = P_INT;
    break;
  case T_LONG:
    type = P_LONG;
    break;
  default:
    fatald("Illegal type, token", Token.token);
  }

  while (1)
  {
    scan(&Token);
    if (Token.token != T_STAR)
      break;

    type = pointer_to(type);
  }

  return type; // Exit with next token already scanned
}

// Parse the declaration of a list of variables.
// The identifier has been scanned and we have the type.
void variable_declaration(int type)
{
  int id;

  if (Token.token == T_LBRACKET)
  {
    scan(&Token);

    if (Token.token == T_INTLIT)
    {
      // Add as a known array and generate its space in assembly. Treat as a pointer to its elements' type.
      id = addglob(Text, pointer_to(type), S_ARRAY, 0, Token.intvalue);
      genglobsym(id);
    }

    scan(&Token);
    match(T_RBRACKET, "]");
  }
  else
  {
    while (1)
    {
      id = addglob(Text, type, S_VARIABLE, 0, 1); // Text contains last identifier, via `scanident()`
      genglobsym(id);

      if (Token.token == T_SEMI)
      {
        scan(&Token);
        return;
      }

      if (Token.token == T_COMMA)
      {
        scan(&Token);
        ident();
        continue;
      }

      fatal("Missing `,` or `;` after identifier");
    }
  }
}

// Parse the declaration of a function.
// The identifier has been scanned and we have the type.
struct ASTnode *function_declaration(int type)
{
  struct ASTnode *tree, *finalstatement;
  int nameslot, endlabel;

  // Get a label ID for the end label, add the function to the symbol table,
  // and set the `Functionid` global to the function's symbol table index
  endlabel = genlabel();
  nameslot = addglob(Text, type, S_FUNCTION, endlabel, 0);
  Functionid = nameslot;

  lparen();
  rparen();

  tree = compound_statement();

  // If the function doesn't return `void`, ensure the last AST
  // operation in the compound statement was a `return` statement
  if (type != P_VOID)
  {
    if (tree == NULL)
      fatal("No statements in function with non-void type");

    finalstatement = (tree->op == A_GLUE) ? tree->right : tree;
    if (finalstatement == NULL || finalstatement->op != A_RETURN)
      fatal("No `return` for function with non-void type");
  }

  return mkastunary(A_FUNCTION, type, tree, nameslot);
}

// Parse one or more global declarations, either variables or functions
void global_declarations(void)
{
  struct ASTnode *tree;
  int type;

  while (1)
  {
    type = parse_type();
    ident();

    if (Token.token == T_LPAREN)
    {
      tree = function_declaration(type);

      if (O_dumpAST)
      {
        dumpAST(tree, NOLABEL, 0);
        fprintf(stdout, "\n\n");
      }

      genAST(tree, NOREG, 0);
    }
    else
    {
      variable_declaration(type);
    }

    if (Token.token == T_EOF)
      break;
  }
}
