#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Struct and enum definitions

#define TEXTLEN 512   // Length of symbols in input
#define NSYMBOLS 1024 // Number of symbol table entries

// Token types
enum
{
  T_EOF,

  T_PLUS,
  T_MINUS,

  T_STAR,
  T_SLASH,

  T_EQ,
  T_NE,

  T_LT,
  T_GT,
  T_LE,
  T_GE,

  T_INTLIT,
  T_SEMI,
  T_ASSIGN,
  T_IDENT,

  T_LBRACE,
  T_RBRACE,
  T_LPAREN,
  T_RPAREN,
  // Keywords
  T_PRINT,
  T_INT,
  T_IF,
  T_ELSE,
  T_WHILE,
};

struct token
{
  int token;    // From enum, above
  int intvalue; // For `T_INTLIT`
};

// AST node types (op)
enum
{
  A_ADD = 1, // These 4 line up with the tokens above
  A_SUBTRACT,
  A_MULTIPLY,
  A_DIVIDE,

  A_EQ,
  A_NE,
  A_LT,
  A_GT,
  A_LE,
  A_GE,

  A_INTLIT,

  A_IDENT,
  A_LVIDENT,
  A_ASSIGN,
  A_PRINT,
  A_GLUE,

  A_IF,
  A_WHILE,
};

struct ASTnode
{
  int op;               // "Operation" to be performed on this tree
  struct ASTnode *left; // Left and right child trees
  struct ASTnode *mid;
  struct ASTnode *right;
  union
  {
    int intvalue; // For `A_INTLIT`, the integer value
    int id;       // For `A_IDENT`, the symbol slot number
  } v;
};

// Use when the AST generation functions have no register to return
#define NOREG -1

// Symbol table structure
struct symtable
{
  char *name; // Name of a symbol
};
