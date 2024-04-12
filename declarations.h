// Function prototypes for all compiler files

// `scanner.c`
void reject_token(struct token *t);
int scan(struct token *t);

// `tree.c`
struct ASTnode *mkastnode(int op, int type, struct ASTnode *left, struct ASTnode *mid, struct ASTnode *right, int intvalue);
struct ASTnode *mkastleaf(int op, int type, int intvalue);
struct ASTnode *mkastunary(int op, int type, struct ASTnode *left, int intvalue);
void dumpAST(struct ASTnode *n, int label, int parentASTop);

// `code_generation.c`
int genlabel(void);
int genAST(struct ASTnode *n, int label, int parentASTop);
void genpreamble(void);
void genpostamble(void);
void genfreeregs(void);
void genprintint(int reg);
void genglobsym(int id);
int genprimsize(int type);

// `code_generation_x86-64.c`
void freeall_registers(void);
void cgpreamble(void);
void cgpostamble(void);
void cgfuncpreamble(int id);
void cgfuncpostamble(int id);
int cgloadint(int value, int type);
int cgloadglob(int id);
int cgadd(int r1, int r2);
int cgsub(int r1, int r2);
int cgmul(int r1, int r2);
int cgdiv(int r1, int r2);
int cgshlconst(int r, int val);
void cgprintint(int r);
int cgcall(int r, int id);
int cgstorglob(int r, int id);
void cgglobsym(int id);
int cgcompare_and_set(int ASTop, int r1, int r2);
int cgcompare_and_jump(int ASTop, int r1, int r2, int label);
void cglabel(int l);
void cgjump(int l);
int cgwiden(int r, int oldtype, int newtype);
int cgprimsize(int type);
void cgreturn(int reg, int id);
int cgaddress(int id);
int cgderef(int r, int type);
int cgstorderef(int r1, int r2, int type);

// `expressions.c`
struct ASTnode *binexpr(int ptp);

// `statements.c`
struct ASTnode *compound_statement(void);

// `miscellaneous.c`
void match(int t, char *what);
void semi(void);
void lbrace(void);
void rbrace(void);
void lparen(void);
void rparen(void);
void ident(void);
void fatal(char *s);
void fatals(char *s1, char *s2);
void fatald(char *s, int d);
void fatalc(char *s, int c);

// `symbols.c`
int findglob(char *s);
int addglob(char *name, int type, int stype, int endlabel, int size);

// `declarations.c`
void variable_declaration(int type);
struct ASTnode *function_declaration(int type);
void global_declarations(void);

// `types.c`
int inttype(int type);
int parse_type(void);
int pointer_to(int type);
int value_at(int type);
struct ASTnode *modify_type(struct ASTnode *tree, int rtype, int op);
