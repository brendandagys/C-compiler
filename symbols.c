#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Symbol table functions

static int Globs = 0; // Index of next free global symbol slot

// Determine if a symbol is in the global symbol table. Return its index or -1.
int findglob(char *s)
{
  for (int i = 0; i < Globs; i++)
  {
    if (*s == *Gsym[i].name && !strcmp(s, Gsym[i].name))
      return i;
  }

  return -1;
}

// Get the index of a new symbol; terminate if no more positions
static int newglob(void)
{
  int p;

  if ((p = Globs++) >= NSYMBOLS)
    fatal("Too many global symbols");

  return p;
}

// Add a symbol to the symbol table, set up its type and structural type, and return its index
int addglob(char *name, int type, int stype, int endlabel)
{
  int y;

  if ((y = findglob(name)) > -1)
    return y;

  y = newglob();
  Gsym[y].name = strdup(name);
  Gsym[y].type = type;
  Gsym[y].stype = stype;
  Gsym[y].endlabel = endlabel;
  return y;
}
