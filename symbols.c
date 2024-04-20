#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Symbol table functions

// 0xxxx......................................xxxxxxxxxxxxNSYMBOLS-1
//      ^                                    ^
//      |                                    |
//    Globals                              Locals

// Determine if a symbol is in the global symbol table. Return its index or -1.
int findglobal(char *s)
{
  for (int i = 0; i < Globals; i++)
  {
    if (Symtable[i].class == C_PARAM)
      continue;
    if (*s == *Symtable[i].name && !strcmp(s, Symtable[i].name))
      return i;
  }

  return -1;
}

// Get the index of a new global symbol; terminate if no more positions
static int newglobal(void)
{
  int p;

  if ((p = Globals++) >= Locals)
    fatal("Too many global symbols");

  return p;
}

// Determine if a symbol is in the local symbol table. Return its index or -1.
int findlocal(char *s)
{
  for (int i = Locals + 1; i < NSYMBOLS; i++)
  {
    if (*s == *Symtable[i].name && !strcmp(s, Symtable[i].name))
      return i;
  }

  return -1;
}

// Get the index of a new local symbol; terminate if no more positions
static int newlocal(void)
{
  int p;

  if ((p = Locals--) <= Globals)
    fatal("Too many local symbols");

  return p;
}

// Clear all entries in the local symbol table
void freelocalsymbols(void)
{
  Locals = NSYMBOLS - 1;
}

// Update a symbol at the given slot number in the symbol table. Set up its:
// - type: char, int etc.
// - structural type: var, function, array etc.
// - class: global or local
// - endlabel: if it is a function
// - size: number of elements
// - position: Position information for local symbols
static void updatesymbol(
    int slot, char *name, int type, int stype, int class, int endlabel, int size, int position)
{
  if (slot < 0 || slot >= NSYMBOLS)
    fatal("Invalid symbol slot number in `updatesymbol()`");
  Symtable[slot].name = strdup(name);
  Symtable[slot].type = type;
  Symtable[slot].stype = stype;
  Symtable[slot].class = class;
  Symtable[slot].endlabel = endlabel;
  Symtable[slot].size = size;
  Symtable[slot].position = position;
}

// Add a global symbol to the symbol table and return its index. Set up its:
// - type: char, int etc.
// - structural type: var, function, array etc.
// - endlabel: if this is a function
// - size: number of elements
int addglobal(char *name, int type, int stype, int endlabel, int size)
{
  int slot;

  if ((slot = findglobal(name)) != -1)
    return slot;

  slot = newglobal();
  updatesymbol(slot, name, type, stype, C_GLOBAL, endlabel, size, 0);
  genglobalsym(slot);

  return slot;
}

// Add a local symbol to the symbol table and return its index or -1 if duplicate.
// Set up its:
// - type: char, int etc.
// - structural type: var, function, array etc.
// - size: number of elements
// - isparam: if true, this is a parameter to the function
int addlocal(char *name, int type, int stype, int isparam, int size)
{
  int localslot, globalslot;

  if ((localslot = findlocal(name)) != -1)
    return -1;

  localslot = newlocal();

  if (isparam)
  {
    updatesymbol(localslot, name, type, stype, C_PARAM, 0, size, 0);
    globalslot = newglobal();
    updatesymbol(globalslot, name, type, stype, C_PARAM, 0, size, 0);
  }
  else
  {
    updatesymbol(localslot, name, type, stype, C_LOCAL, 0, size, 0);
  }

  return localslot;
}

// Determine if the symbol is in the symbol table. Return its index or -1.
int findsymbol(char *s)
{
  int slot = findlocal(s);

  if (slot == -1)
    slot = findglobal(s);

  return slot;
}
