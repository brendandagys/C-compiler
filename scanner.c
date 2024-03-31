#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Lexical scanning

// Return the position of character `c` in string `s`, or -1 if not found
static int chrpos(char *s, int c)
{
  char *p;
  p = strchr(s, c); // Returns NULL pointer if not found
  return p ? p - s : -1;
}

// Get the next character from the input file
static int next(void)
{
  int c;

  if (Putback)
  {
    c = Putback;
    Putback = 0;
    return c;
  }

  c = fgetc(Infile); // Read from input file

  if ('\n' == c)
    Line++;

  return c;
}

static void putback(int c)
{
  Putback = c;
}

// Skip past unwanted input (e.g. whitespace, newlines) and then return the first character
static int skip(void)
{
  int c;
  c = next();

  while (' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c)
    c = next();

  return c;
}

static int scanint(int c)
{
  int k, val = 0;

  // Convert each character into an int value
  while ((k = chrpos("0123456789", c)) >= 0)
  {
    val = val * 10 + k;
    c = next();
  }

  // We hit a non-integer character. Put it back.
  putback(c);
  return val;
}

// Scan an identifier from the input file and store it in `buf[]`. Return the identifier's length.
static int scanident(int c, char *buf, int lim)
{
  int i = 0;

  while (isalpha(c) || isdigit(c) || '_' == c)
  {
    if (lim - 1 == i)
    {
      printf("Identifier too long on line %d\n", Line);
      exit(1);
    }
    else if (i < lim - 1) // Can remove this...
    {
      buf[i++] = c;
    }

    c = next();
  }

  // Put back the last character read that was not part of the identifier
  // NULL-terminate the buffer and return its length
  putback(c);
  buf[i] = '\0';
  return i;
}

// Return the token number for a given word from the input, or 0
// if not a keyword. Switch on first character for efficiency.
static int keyword(char *s)
{
  switch (*s)
  {
  case 'p':
    if (!strcmp(s, "print"))
      return T_PRINT;
    break;
  }

  return 0;
}

// Updates passed-in `token` and returns 1 if valid token, 0 if EOF
int scan(struct token *t)
{
  int c, tokentype;

  c = skip();

  switch (c)
  {
  case EOF:
    t->token = T_EOF;
    return 0;
  case '+':
    t->token = T_PLUS;
    break;
  case '-':
    t->token = T_MINUS;
    break;
  case '*':
    t->token = T_STAR;
    break;
  case '/':
    t->token = T_SLASH;
    break;
  case ';':
    t->token = T_SEMI;
    break;
  default:
    if (isdigit(c))
    {
      t->intvalue = scanint(c);
      t->token = T_INTLIT;
      break;
    }
    else if (isalpha(c) || '_' == c)
    {
      // Read in a keyword or identifier
      scanident(c, Text, TEXTLEN);

      if ((tokentype = keyword(Text)))
      {
        t->token = tokentype;
        break;
      }

      printf("Unrecognized symbol %s on line %d\n", Text, Line);
      exit(1);
    }

    printf("Unrecognized character %c on line %d\n", c, Line);
    exit(1);
  }

  return 1;
}
