#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Miscellaneous functions

// Ensure that the current token is `t` and fetch the next token
void match(int t, char *what)
{
  if (Token.token == t)
  {
    scan(&Token);
  }
  else
  {
    printf("%s expected on line %d\n", what, Line);
    exit(1);
  }
}

// Match a semicolon and fetch the next token
void semi(void)
{
  match(T_SEMI, ";");
}
