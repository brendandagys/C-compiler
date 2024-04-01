#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Parsing of declarations

// Parse the declaration of a variable.
// Ensure `int <identifier>;`
void variable_declaration(void)
{
  match(T_INT, "int");
  ident();
  addglob(Text); // Text contains last identifier, via `scanident()`
  genglobsym(Text);
  semi();
}
