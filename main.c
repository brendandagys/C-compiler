#include "definitions.h"

#define extern_
#include "data.h"
#undef extern_

#include "declarations.h"
#include <errno.h>

// Compiler set-up and top-level execution

// Initialize global variables
static void init() {
  Line = 1;
  Putback = '\n';
  Globals = 0;
  Locals = NSYMBOLS - 1;
  O_dumpAST = 0;
}

// Print instructions if program arguments are incorrect
static void usage(char *prog) {
  fprintf(stderr, "Usage: %s [-T] infile\n", prog);
  exit(1);
}

// Open/scan the file and its tokens
int main(int argc, char *argv[]) {
  int i;

  init();  // Initialize globals

  // Scan for command-line options
  for (i = 1; i < argc; i++) {
    if (*argv[i] != '-') break;

    for (int j = 1; argv[i][j]; j++) {
      switch (argv[i][j]) {
        case 'T':
          O_dumpAST = 1;
          break;
        default:
          usage(argv[0]);
      }
    }
  }

  // Ensure we have an input file argument
  if (i >= argc)
    usage(argv[0]);

  if ((Infile = fopen(argv[i], "r")) == NULL) {
    fprintf(stderr, "Unable to open %s: %s\n", argv[i], strerror(errno));
    exit(1);
  }

  if ((Outfile = fopen("out.s", "w")) == NULL) {
    fprintf(stderr, "Unable to create `out.s`: %s\n", strerror(errno));
    exit(1);
  }

  // For now, ensure that `void printint()` is defined
  addglobal("printint", P_INT, S_FUNCTION, 0, 0);
  addglobal("printchar", P_VOID, S_FUNCTION, 0, 0);

  scan(&Token);  // Get the first token from the input
  genpreamble();
  global_declarations();  // Parse the global declarations
  genpostamble();

  fclose(Infile);
  fclose(Outfile);

  return 0;
}
