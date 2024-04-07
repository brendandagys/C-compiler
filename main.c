#include "definitions.h"

#define extern_
#include "data.h"
#undef extern_

#include "declarations.h"
#include <errno.h>

// Compiler set-up and top-level execution

// Initialize global variables
static void init()
{
  Line = 1;
  Putback = '\n';
  Globs = 0;
}

// Print instructions if program arguments are incorrect
static void usage(char *prog)
{
  fprintf(stderr, "Usage: %s infile\n", prog);
  exit(1);
}

// Open/scan the file and its tokens
int main(int argc, char *argv[])
{
  struct ASTnode *tree;

  if (argc != 2)
    usage(argv[0]);

  init();

  if ((Infile = fopen(argv[1], "r")) == NULL)
  {
    fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
    exit(1);
  }

  if ((Outfile = fopen("out.s", "w")) == NULL)
  {
    fprintf(stderr, "Unable to create `out.s`: %s\n", strerror(errno));
    exit(1);
  }

  // For now, ensure that `void printint()` is defined
  addglob("printint", P_CHAR, S_FUNCTION, 0);

  scan(&Token); // Get the first token from the input
  genpreamble();

  while (1)
  {
    tree = function_declaration();
    genAST(tree, NOREG, 0);

    if (Token.token == T_EOF)
      break;
  }

  genpostamble();

  fclose(Infile);
  fclose(Outfile);

  return 0;
}
