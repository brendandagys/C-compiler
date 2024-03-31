#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of statements

// Parse one or more statements
void statements(void)
{
  struct ASTnode *tree;
  int reg;

  while (1)
  {
    // Match a `print` as the first token
    match(T_PRINT, "print");

    // Parse the following expression and generate the assembly code
    tree = binexpr(0);
    reg = genAST(tree);
    genprintint(reg);
    genfreeregs();

    // Match the following semicolon and stop if we reach EOF
    semi();

    if (Token.token == T_EOF)
      return;
  }
}
