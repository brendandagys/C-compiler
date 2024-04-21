#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of declarations

// Parse the current token and return a primitive type enum value.
// Also scan in the next token.
int parse_type(void) {
  int type;
  switch (Token.token) {
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

  while (1) {
    scan(&Token);
    if (Token.token != T_STAR)
      break;

    type = pointer_to(type);
  }

  return type;  // Exit with next token already scanned
}

// Parse the declaration of a scalar variable or an array with a given size.
// The identifier has been scanned and we have the type.
void variable_declaration(int type, int class) {
  if (Token.token == T_LBRACKET) {
    scan(&Token);

    if (Token.token == T_INTLIT) {
      // Add as a known array and generate its space in assembly. Treat as a pointer to its elements' type.
      if (class == C_LOCAL)
        fatal("For now, declaration of local arrays is not implemented");

      addglobal(Text, pointer_to(type), S_ARRAY, class, 0, Token.intvalue);
    }

    scan(&Token);
    match(T_RBRACKET, "]");
  } else {
    if (class == C_LOCAL) {
      if (addlocal(Text, type, S_VARIABLE, class, 1) == -1)  // Text contains last identifier, via `scanident()`
        fatals("Duplicate local variable declaration", Text);
    } else
      addglobal(Text, type, S_VARIABLE, class, 0, 1);
  }
}

// Parse the parameters in parentheses after a function name.
// Add them as symbols to the symbol table and return the number of parameters.
// If `id` is not -1, its the symbol slot number of an existing function prototype.
static int param_declaration(int id) {
  int type, param_id, orig_paramcount, paramcount = 0;

  param_id = id + 1;  // Now either 0 (no prototype), or at position of first parameter in symbol table

  // Get any existing prototype parameter count
  if (param_id)
    orig_paramcount = Symtable[id].numelems;

  while (Token.token != T_RPAREN) {
    type = parse_type();
    ident();

    if (param_id) {
      if (type != Symtable[id].type)
        fatald("Type doesn't match prototype for parameter", paramcount + 1);

      param_id++;
    } else {
      variable_declaration(type, C_PARAM);
    }

    paramcount++;

    switch (Token.token) {
      case T_COMMA:
        scan(&Token);
        break;
      case T_RPAREN:
        break;
      default:
        fatald("Unexpected token in parameter list", Token.token);
    }
  }

  if ((id != -1) && (paramcount != orig_paramcount))
    fatals("Parameter count mismatch for function", Symtable[id].name);

  return paramcount;
}

// Parse the declaration of a function.
// The identifier has been scanned and we have the type.
struct ASTnode *function_declaration(int type) {
  struct ASTnode *tree, *finalstatement;
  int index, nameslot, endlabel, paramcount;

  // If identifier in `Text` exists and is a function, get its symbol table index
  if ((index = findsymbol(Text)) != -1) {
    if (Symtable[index].stype != S_FUNCTION)
      index = -1;
  }

  // If existing function identifier not found, get a label ID for the end label
  // and add a new function symbol to the symbol table.
  if (index == -1) {
    endlabel = genlabel();
    nameslot = addglobal(Text, type, S_FUNCTION, C_GLOBAL, endlabel, 0);
  }

  lparen();
  paramcount = param_declaration(index);
  rparen();

  // If a new function declaration, update the function symbol table entry with the parameter count
  if (index == -1)
    Symtable[index].numelems = paramcount;

  if (Token.token == T_SEMI) {
    scan(&Token);
    return NULL;
  }

  if (index == -1)
    index = nameslot;

  copyfuncparams(index);

  Functionid = index;  // Set global to the function's symbol table index

  tree = compound_statement();

  // If the function doesn't return `void`, ensure the last AST
  // operation in the compound statement was a `return` statement
  if (type != P_VOID) {
    if (tree == NULL)
      fatal("No statements in function with non-void type");

    finalstatement = (tree->op == A_GLUE) ? tree->right : tree;
    if (finalstatement == NULL || finalstatement->op != A_RETURN)
      fatal("No `return` for function with non-void type");
  }

  return mkastunary(A_FUNCTION, type, tree, index);
}

// Parse one or more global declarations, either variables or functions
void global_declarations(void) {
  struct ASTnode *tree;
  int type;

  while (1) {
    type = parse_type();
    ident();

    if (Token.token == T_LPAREN) {
      tree = function_declaration(type);

      if (tree == NULL)  // Only a prototype; no body
        continue;

      if (O_dumpAST) {
        dumpAST(tree, NOLABEL, 0);
        fprintf(stdout, "\n\n");
      }

      genAST(tree, NOLABEL, 0);

      freelocalsymbols();  // Free the symbols associated with this function
    } else {
      variable_declaration(type, C_GLOBAL);
      semi();
    }

    if (Token.token == T_EOF)
      break;
  }
}
