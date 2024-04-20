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
  // Binary operators
  T_ASSIGN,
  T_LOGOR,
  T_LOGAND,
  T_OR,
  T_XOR,
  T_AMPER,
  T_EQ,
  T_NE,
  T_LT,
  T_GT,
  T_LE,
  T_GE,
  T_LSHIFT,
  T_RSHIFT,
  T_PLUS,
  T_MINUS,
  T_STAR,
  T_SLASH,
  // Other operators
  T_INC,
  T_DEC,
  T_INVERT,
  T_LOGNOT,
  // Type keywords
  T_VOID,
  T_CHAR,
  T_INT,
  T_LONG,
  // Other keywords
  T_IF,
  T_ELSE,
  T_WHILE,
  T_FOR,
  T_RETURN,
  // Structural tokens
  T_INTLIT,
  T_STRLIT,
  T_SEMI,
  T_IDENT,
  T_LBRACE,
  T_RBRACE,
  T_LPAREN,
  T_RPAREN,
  T_LBRACKET,
  T_RBRACKET,
  T_COMMA,
};

struct token
{
  int token;    // From enum, above
  int intvalue; // For `T_INTLIT`
};

// AST node types (op)
enum
{
  A_ASSIGN = 1,
  A_LOGOR,
  A_LOGAND,
  A_OR,
  A_XOR,
  A_AND,
  A_EQ,
  A_NE,
  A_LT,
  A_GT,
  A_LE,
  A_GE,
  A_LSHIFT,
  A_RSHIFT,
  A_ADD, // These 4 line up with the tokens above
  A_SUBTRACT,
  A_MULTIPLY,
  A_DIVIDE,
  A_INTLIT,
  A_STRLIT,
  A_IDENT,
  A_GLUE,

  A_IF,
  A_WHILE,
  A_FUNCTION,
  A_WIDEN,
  A_RETURN,

  A_FUNCCALL,
  A_DEREF, // Dereference the pointer in the child node
  A_ADDR,  // Get the address of the identifier in this node
  A_SCALE,

  A_PREINC,
  A_PREDEC,
  A_POSTINC,
  A_POSTDEC,

  A_NEGATE,
  A_INVERT,
  A_LOGNOT,
  A_TOBOOL,
};

// Primitive types
enum
{
  P_NONE,
  P_VOID,
  P_CHAR,
  P_INT,
  P_LONG,
  P_VOIDPTR,
  P_CHARPTR,
  P_INTPTR,
  P_LONGPTR,
};

struct ASTnode
{
  int op;               // "Operation" to be performed on this tree
  int type;             // Type of any expression this tree generates
  int rvalue;           // True if the node is an r-value
  struct ASTnode *left; // Left, middle, and right child trees
  struct ASTnode *mid;
  struct ASTnode *right;
  union
  {
    int intvalue; // For `A_INTLIT`: the integer value
    int id;       // For `A_IDENT` + `A_FUNCTION` + `A_FUNCCALL`: the symbol slot number
    int size;     // For `A_SCALE`: the size to scale by
  } v;
};

#define NOREG -1  // Use when the AST generation functions have no register to return
#define NOLABEL 0 // Use when we have no label to pass to `genAST()`

// Structural types
enum
{
  S_VARIABLE,
  S_FUNCTION,
  S_ARRAY,
};

// Storage classes
enum
{
  C_GLOBAL = 1, // Globally visible symbol
  C_LOCAL,      // Locally visible symbol
};

// Symbol table structure
struct symtable
{
  char *name;   // Name of a symbol
  int type;     // Primitive type for the symbol
  int stype;    // Structural type for the symbol
  int class;    // Storage class for the symbol
  int endlabel; // For `S_FUNCTION`, the end label
  int size;     // Number of elements in the symbol
  int position; // For locals, the negative offset from the stack base pointer
};
