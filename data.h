#ifndef extern_
#define extern_ extern
#endif

// Global variables

extern_ int Line;        // Current line number
extern_ int Putback;     // Character "put back" by scanner
extern_ int Functionid;  // Symbol ID of the current function
extern_ int Globals;     // Index of next free global symbol slot
extern_ int Locals;      // Index of next free local symbol slot
extern_ FILE *Infile;
extern_ FILE *Outfile;
extern_ struct token Token;                  // Last token scanned
extern_ char Text[TEXTLEN + 1];              // Last identifier, via `scanident()`
extern_ struct symtable Symtable[NSYMBOLS];  // Global symbol table

extern_ int O_dumpAST;
