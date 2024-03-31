// Function prototypes for all compiler files

// `scanner.c`
int scan(struct token *t);

// `tree.c`
struct ASTnode *mkastnode(int op, struct ASTnode *left, struct ASTnode *right, int intvalue);
struct ASTnode *mkastleaf(int op, int intvalue);
struct ASTnode *mkastunary(int op, struct ASTnode *left, int intvalue);

// `code_generation.c`
int genAST(struct ASTnode *n);
void genpreamble();
void genpostamble();
void genfreeregs();
void genprintint(int reg);

// `code_generation_x86-64.c`
void freeall_registers(void);
void cgpreamble();
void cgpostamble();
int cgload(int value);
int cgadd(int r1, int r2);
int cgsub(int r1, int r2);
int cgmul(int r1, int r2);
int cgdiv(int r1, int r2);
void cgprintint(int r);

// `expressions.c`
struct ASTnode *binexpr(int ptp);

// `statements.c`
void statements(void);

// `miscellaneous.c`
void match(int t, char *what);
void semi(void);
