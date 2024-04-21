#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Lexical scanning

// Return the position of character `c` in string `s`, or -1 if not found
static int chrpos(char *s, int c) {
  char *p;
  p = strchr(s, c);  // Returns NULL pointer if not found
  return p ? p - s : -1;
}

// Get the next character from the input file
static int next(void) {
  int c;

  if (Putback) {
    c = Putback;
    Putback = 0;
    return c;
  }

  c = fgetc(Infile);  // Read from input file

  if ('\n' == c)
    Line++;

  return c;
}

static void putback(int c) {
  Putback = c;
}

// Skip past unwanted input (e.g. whitespace, newlines) and then return the first character
static int skip(void) {
  int c;
  c = next();

  while (' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c)
    c = next();

  return c;
}

// Return the next character from a character or string literal.
// Doesn't recognize octal character codings, etc.
static int scanch(void) {
  int c = next();

  if (c == '\\') {
    switch (c = next()) {
      case 'a':
        return '\a';
      case 'b':
        return '\b';
      case 'f':
        return '\f';
      case 'n':
        return '\n';
      case 'r':
        return '\r';
      case 't':
        return '\t';
      case 'v':
        return '\v';
      case '\\':
        return '\\';
      case '"':
        return '"';
      case '\'':
        return '\'';
      default:
        fatalc("unknown escape sequence", c);
    }
  }

  return c;  // An ordinary character
}

// Scan an integer literal from the input file and return it
static int scanint(int c) {
  int k, val = 0;

  // Convert each character into an int value
  while ((k = chrpos("0123456789", c)) >= 0) {
    val = val * 10 + k;
    c = next();
  }

  // We hit a non-integer character. Put it back.
  putback(c);
  return val;
}

// Scan in a string literal and store it in `buf[]`. Return the string's length.
static int scanstr(char *buf) {
  int i, c;

  for (i = 0; i < TEXTLEN - 1; i++) {
    if ((c = scanch()) == '"') {
      buf[i] = '\0';
      return i;
    }
    buf[i] = c;
  }

  fatal("String literal too long");

  return 0;
}

// Scan an identifier from the input file and store it in `buf[]`. Return the identifier's length.
static int scanident(int c, char *buf, int lim) {
  int i = 0;

  while (isalpha(c) || isdigit(c) || '_' == c) {
    if (i >= lim - 1)
      fatal("Identifier too long");

    buf[i++] = c;

    c = next();
  }

  // Put back final non-identifier character read. Terminate buffer and return its length.
  putback(c);
  buf[i] = '\0';
  return i;
}

// Return the token number for a given word from the input, or 0
// if not a keyword. Switch on first character for efficiency.
static int keyword(char *s) {
  switch (*s) {
    case 'c':
      if (!strcmp(s, "char"))
        return T_CHAR;
      break;
    case 'e':
      if (!strcmp(s, "else"))
        return T_ELSE;
      break;
    case 'f':
      if (!strcmp(s, "for"))
        return T_FOR;
      break;
    case 'i':
      if (!strcmp(s, "if"))
        return T_IF;
      if (!strcmp(s, "int"))
        return T_INT;
      break;
    case 'l':
      if (!strcmp(s, "long"))
        return T_LONG;
      break;
    case 'r':
      if (!strcmp(s, "return"))
        return T_RETURN;
      break;
    case 'w':
      if (!strcmp(s, "while"))
        return T_WHILE;
      break;
    case 'v':
      if (!strcmp(s, "void"))
        return T_VOID;
      break;
  }

  return 0;
}

// A pointer to a rejected token
static struct token *Rejtoken = NULL;

// Reject the last token scanned
void reject_token(struct token *t) {
  if (Rejtoken != NULL)
    fatal("Can't reject token twice");
  Rejtoken = t;
}

// Updates passed-in `token` and returns 1 if valid token, 0 if EOF
int scan(struct token *t) {
  int c, tokentype;

  if (Rejtoken != NULL) {
    t = Rejtoken;
    Rejtoken = NULL;
    return 1;
  }

  c = skip();

  switch (c) {
    case EOF:
      t->token = T_EOF;
      return 0;
    case '+':
      if ((c = next()) == '+') {
        t->token = T_INC;
      } else {
        putback(c);
        t->token = T_PLUS;
      }
      break;
    case '-':
      if ((c = next()) == '-') {
        t->token = T_DEC;
      } else {
        putback(c);
        t->token = T_MINUS;
      }
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
    case '{':
      t->token = T_LBRACE;
      break;
    case '}':
      t->token = T_RBRACE;
      break;
    case '(':
      t->token = T_LPAREN;
      break;
    case ')':
      t->token = T_RPAREN;
      break;
    case '[':
      t->token = T_LBRACKET;
      break;
    case ']':
      t->token = T_RBRACKET;
      break;
    case '~':
      t->token = T_INVERT;
      break;
    case '^':
      t->token = T_XOR;
      break;
    case ',':
      t->token = T_COMMA;
      break;
    case '=':
      if ((c = next()) == '=')
        t->token = T_EQ;
      else {
        putback(c);
        t->token = T_ASSIGN;
      }
      break;
    case '!':
      if ((c = next()) == '=')
        t->token = T_NE;
      else {
        putback(c);
        t->token = T_LOGNOT;
      }
      break;
    case '<':
      if ((c = next()) == '=')
        t->token = T_LE;
      else if (c == '<') {
        t->token = T_LSHIFT;
      } else {
        putback(c);
        t->token = T_LT;
      }
      break;
    case '>':
      if ((c = next()) == '=')
        t->token = T_GE;
      else if (c == '>') {
        t->token = T_RSHIFT;
      } else {
        putback(c);
        t->token = T_GT;
      }
      break;
    case '&':
      if ((c = next()) == '&')
        t->token = T_LOGAND;
      else {
        putback(c);
        t->token = T_AMPER;
      }
      break;
    case '|':
      if ((c = next()) == '|') {
        t->token = T_LOGOR;
      } else {
        putback(c);
        t->token = T_OR;
      }
      break;
    case '\'':
      // If a quote, scan in the literal character value and the trailing quote
      t->intvalue = scanch();
      t->token = T_INTLIT;
      if (next() != '\'')
        fatal("Expected '\\'' at end of char literal");
      break;
    case '"':
      // Scan in a literal string
      scanstr(Text);
      t->token = T_STRLIT;
      break;
    default:
      if (isdigit(c)) {
        t->intvalue = scanint(c);
        t->token = T_INTLIT;
        break;
      } else if (isalpha(c) || '_' == c) {
        // Read in a keyword or identifier
        scanident(c, Text, TEXTLEN);

        if ((tokentype = keyword(Text))) {
          t->token = tokentype;
          break;
        }

        // Not a recognized keyword; must be an identifier
        t->token = T_IDENT;
        break;
      }

      fatalc("Unrecognized character", c);
  }

  return 1;  // We found a token
}
