#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Struct and enum definitions

#define TEXTLEN 512 // Length of symbols in input

// Token types
enum
{
  T_EOF,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  T_INTLIT,
  T_SEMI,
  T_PRINT
};

struct token
{
  int token;    // From enum, above
  int intvalue; // For `T_INTLIT`
};

// AST node types
enum
{
  A_ADD,
  A_SUBTRACT,
  A_MULTIPLY,
  A_DIVIDE,
  A_INTLIT
};

struct ASTnode
{
  int op;               // Operation to be performed on this tree
  struct ASTnode *left; // Left and right child trees
  struct ASTnode *right;
  int intvalue; // For `A_INTLIT`
};
