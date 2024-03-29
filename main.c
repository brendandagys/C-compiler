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
}

// Print instructions if program arguments are incorrect
static void usage(char *prog)
{
  fprintf(stderr, "Usage: %s infile\n", prog);
  exit(1);
}

// List of printable tokens
char *tokstr[] = {"+", "-", "*", "/", "intlit"};

// Read in all tokens from the input file and print their details
static void scanfile()
{
  struct token T;

  while (scan(&T))
  {
    printf("Token %s", tokstr[T.token]);

    if (T.token == T_INTLIT)
      printf(", value %d", T.intvalue);

    printf("\n");
  }
}

// Open/scan the file and its tokens
int main(int argc, char *argv[])
{
  if (argc != 2)
    usage(argv[0]);

  init();

  if ((Infile = fopen(argv[1], "r")) == NULL)
  {
    fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
    exit(1);
  }

  scanfile();

  return 0;
}
