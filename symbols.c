#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Symbol table functions

// 0xxxx......................................xxxxxxxxxxxxNSYMBOLS-1
//      ^                                    ^
//      |                                    |
//    Globals                              Locals/Parameters

// Determine if a symbol is in the global symbol table. Return its index or -1.
int findglobal(char *s) {
  for (int i = 0; i < Globals; i++) {
    if (Symtable[i].class == C_PARAM)
      continue;
    if (*s == *Symtable[i].name && !strcmp(s, Symtable[i].name))
      return i;
  }

  return -1;
}

// Get the index of a new global symbol; terminate if no more positions
static int newglobal(void) {
  int p;

  if ((p = Globals++) >= Locals)
    fatal("Too many global symbols");

  return p;
}

// Determine if a symbol is in the local symbol table. Return its index or -1.
int findlocal(char *s) {
  for (int i = Locals + 1; i < NSYMBOLS; i++) {
    if (*s == *Symtable[i].name && !strcmp(s, Symtable[i].name))
      return i;
  }

  return -1;
}

// Get the index of a new local symbol; terminate if no more positions
static int newlocal(void) {
  int p;

  if ((p = Locals--) <= Globals)
    fatal("Too many local symbols");

  return p;
}

// Clear all entries in the local symbol table
void freelocalsymbols(void) {
  Locals = NSYMBOLS - 1;
}

// Update a symbol at the given slot number in the symbol table. Set up its:
// - type: char, int etc.
// - structural type: var, function, array etc.
// - class: `C_GLOBAL` or `C_LOCAL` or `C_PARAM`
// - endlabel: if it is a function
// - size: number of elements
// - position: Position information for local symbols
static void updatesymbol(
    int slot, char *name, int type, int stype, int class, int endlabel, int size, int position) {
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
// - class: `C_GLOBAL` or `C_PARAM`
// - endlabel: if this is a function
// - size: number of elements
int addglobal(char *name, int type, int stype, int class, int endlabel, int size) {
  int globalslot;

  if ((globalslot = findglobal(name)) != -1)
    return globalslot;

  globalslot = newglobal();
  updatesymbol(globalslot, name, type, stype, class, endlabel, size, 0);

  if (class == C_GLOBAL)
    genglobalsym(globalslot);

  return globalslot;
}

// Add a local symbol to the symbol table and return its index, or -1 if duplicate.
// Set up its:
// - type: char, int etc.
// - structural type: var, function, array etc.
// - class: `C_LOCAL` or `C_PARAM`
// - size: number of elements
int addlocal(char *name, int type, int stype, int class, int size) {
  int localslot;

  if ((localslot = findlocal(name)) != -1)
    return -1;

  localslot = newlocal();
  updatesymbol(localslot, name, type, stype, class, 0, size, 0);

  return localslot;
}

// Given a function's slot number, copy the global parameters from its
// prototype to be local parameters.
void copyfuncparams(int slot) {
  for (
      int i = 0, id = slot + 1;
      i < Symtable[slot].numelems;
      i++, id++) {
    addlocal(
        Symtable[id].name,
        Symtable[id].type,
        Symtable[id].stype,
        Symtable[id].class,
        Symtable[id].size);
  }
}

// Determine if the symbol is in the symbol table. Return its index or -1.
int findsymbol(char *s) {
  int slot = findlocal(s);

  if (slot == -1)
    slot = findglobal(s);

  return slot;
}
