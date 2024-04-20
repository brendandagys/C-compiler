#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Code generator for x86-64

// b: Byte (8 bits)
// w: Word (16 bits)
// l: Long (32 bits)
// q: Quadword (64 bits)

// Flag to specify the section being output to
enum
{
  no_seg,
  text_seg,
  data_seg
} currSeg = no_seg;

void cgtextseg()
{
  if (currSeg != text_seg)
  {
    fputs("\t.text\n", Outfile);
    currSeg = text_seg;
  }
}

void cgdataseg()
{
  if (currSeg != data_seg)
  {
    fputs("\t.data\n", Outfile);
    currSeg = data_seg;
  }
}

// Position of next local variable relative to the stack base pointer.
// We store the offset as positive to make aligning the stack pointer easier.
static int localOffset;
static int stackOffset;

// Create the position of a new local variable
static int newlocaloffset(int type)
{
  // Decrement offset by a minimum of 4 bytes and allocate on the stack
  localOffset += (cgprimsize(type) > 4) ? cgprimsize(type) : 4;
  return -localOffset;
}

// List of available registers and their names.
// We need a list of byte and double-word registers, too.
// The list also includes the registers used to hold function parameters.
#define NUMFREEREGS 4
#define FIRSTPARAMREG 9 // Position of first parameter register
static int freereg[NUMFREEREGS];
static char *reglist[] =
    {"%r10", "%r11", "%r12", "%r13", "%r9", "%r8", "%rcx", "%rdx", "%rsi", "%rdi"};
static char *breglist[] =
    {"%r10b", "%r11b", "%r12b", "%r13b", "%r9b", "%r8b", "%cl", "%dl", "%sil", "%dil"};
static char *dreglist[] =
    {"%r10d", "%r11d", "%r12d", "%r13d", "%r9d", "%r8d", "%ecx", "%edx", "%esi", "%edi"};

// Set all registers as available
void freeall_registers(void)
{
  freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

// Allocate a free register. Return the number of the register. Terminate if none free.
static int alloc_register(void)
{
  for (int i = 0; i < NUMFREEREGS; i++)
  {
    if (freereg[i])
    {
      freereg[i] = 0;
      return i;
    }
  }

  fatal("Out of registers");
  __builtin_unreachable();
}

// Return a register to the list of available registers. Terminate if already available.
static void free_register(int reg)
{
  if (freereg[reg] != 0)
  {
    fatald("Error trying to free register", reg);
  }
  freereg[reg] = 1;
}

// Print out the assembly preamble
void cgpreamble(void)
{
  freeall_registers();
  fputs("\textern\tprintint\n", Outfile);
  fputs("\textern\tprintchar\n", Outfile);

  /*
  |        Higher Addresses        |
  +--------------------------------+
  |  Local Variables (16 bytes)    | <- %rsp (stack pointer)
  |--------------------------------|
  |  Saved %rbp (previous value)   |
  |--------------------------------|
  |        Lower Addresses         |
  +--------------------------------+

  Stack fills downward...
  */

  // fputs("\t.text\n",
  // ".LC0:\n"                    // Label to reference a string literal ("%d\n")
  // "\t.string\t\"%d\\n\"\n"
  // "printint:\n"                // Function label
  // // SET UP STACK FRAME
  // "\tpushq\t%rbp\n"            // Push base pointer onto stack (reference point for
  //                              // accessing local variables and parameters within a
  //                              // function)
  // "\tmovq\t%rsp, %rbp\n"       // Move stack pointer to be at base pointer

  // "\tsubq\t$16, %rsp\n"        // Subtract 16 bytes from stack pointer (allocate
  //                              // space on stack for local variables)
  // "\tmovl\t%edi, -4(%rbp)\n"   // Move `printint` argument to stack
  // "\tmovl\t-4(%rbp), %eax\n"   // Move `printint` argument from stack to EAX
  //                              // register
  // "\tmovl\t%eax, %esi\n"       // Move `printint` argument into ESI (second
  //                              // argument for `printf`)
  // "\tleaq	.LC0(%rip), %rdi\n" // Load address of string literal into RDI
  //                              // (first argument for `printf`)
  // "\tmovl	$0, %eax\n"         // Clear EAX
  // "\tcall	printf@PLT\n"
  // "\tnop\n"
  // "\tleave\n"                  // Clean up stack frame before returning
  // "\tret\n"
  // "\n",
  // Outfile);
}

// Nothing to do
void cgpostamble() {}

// Print out a function preamble
void cgfuncpreamble(int id)
{
  char *name = Symtable[id].name;
  int i;
  int paramOffset = 16;         // Any pushed parameters start at this stack offset
  int paramReg = FIRSTPARAMREG; // Index to the first parameter register in above reg lists

  cgtextseg();
  localOffset = 0;

  fprintf(Outfile,
          "\t.globl\t%s\n"
          "\t.type\t%s, @function\n"
          "%s:\n"
          "\tpushq\t%%rbp\n"
          "\tmovq\t%%rsp, %%rbp\n",
          name, name, name);

  // Copy any in-register parameters to the stack.
  // Stop after no more than six parameter registers.
  for (i = NSYMBOLS - 1; i > Locals; i--)
  {
    if (Symtable[i].class != C_PARAM)
      break;
    if (i < NSYMBOLS - 6)
      break;
    Symtable[i].position = newlocaloffset(Symtable[i].type);
    cgstorlocal(paramReg--, i);
  }

  // For the remainder, if they are a parameter then they are already on the stack.
  // If only a local, make a stack position.
  for (; i > Locals; i--)
  {
    if (Symtable[i].class == C_PARAM)
    {
      Symtable[i].position = paramOffset;
      paramOffset += 8;
    }
    else
    {
      Symtable[i].position = newlocaloffset(Symtable[i].type);
    }
  }

  // Align the stack pointer to be a multiple of 16
  // less than its previous value
  stackOffset = (localOffset + 15) & ~15;
  fprintf(Outfile, "\taddq\t$%d,%%rsp\n", -stackOffset);
}

// Print out a function postamble
void cgfuncpostamble(int id)
{
  cglabel(Symtable[id].endlabel);
  fprintf(Outfile, "\taddq\t$%d,%%rsp\n", stackOffset);
  fputs("\tpopq	%rbp\n"
        "\tret\n",
        Outfile);
}

// Load an integer literal value into a register and return the register number.
// For x86-64, we don't need to worry about the type.
int cgloadint(int value, int type)
{
  int r = alloc_register();
  fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
  return r;
}

// Load a value from a global variable into a register and return the number of the register.
// If the operation is pre- or post-increment/decrement, also perform this action.
int cgloadglobal(int id, int op)
{
  int r = alloc_register();

  switch (Symtable[id].type)
  {
  case P_CHAR:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincb\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecb\t%s(%%rip)\n", Symtable[id].name);

    fprintf(Outfile, "\tmovzbq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);

    if (op == A_POSTINC)
      fprintf(Outfile, "\tincb\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecb\t%s(%%rip)\n", Symtable[id].name);
    break;

  case P_INT:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincl\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecl\t%s(%%rip)\n", Symtable[id].name);

    fprintf(Outfile, "\tmovslq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);

    if (op == A_POSTINC)
      fprintf(Outfile, "\tincl\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecl\t%s(%%rip)\n", Symtable[id].name);
    break;

  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincq\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecq\t%s(%%rip)\n", Symtable[id].name);

    fprintf(Outfile, "\tmovq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);

    if (op == A_POSTINC)
      fprintf(Outfile, "\tincq\t%s(%%rip)\n", Symtable[id].name);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecq\t%s(%%rip)\n", Symtable[id].name);
    break;

  default:
    fatald("Bad type in `cgloadglobal()`:", Symtable[id].type);
  }

  return r;
}

// Load a value from a local variable into a register and return the number of the register.
// If the operation is pre- or post-increment/decrement, also perform this action.
int cgloadlocal(int id, int op)
{
  int r = alloc_register();

  switch (Symtable[id].type)
  {
  case P_CHAR:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincb\t%d(%%rbp)\n", Symtable[id].position);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", Symtable[id].position);

    fprintf(Outfile, "\tmovzbq\t%d(%%rbp), %s\n", Symtable[id].position, reglist[r]);

    if (op == A_POSTINC)
      fprintf(Outfile, "\tincb\t%d(%%rbp)\n", Symtable[id].position);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecb\t%d(%%rbp)\n", Symtable[id].position);
    break;

  case P_INT:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincl\t%d(%%rbp)\n", Symtable[id].position);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", Symtable[id].position);

    fprintf(Outfile, "\tmovslq\t%d(%%rbp), %s\n", Symtable[id].position, reglist[r]);

    if (op == A_POSTINC)
      fprintf(Outfile, "\tincl\t%d(%%rbp)\n", Symtable[id].position);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecl\t%d(%%rbp)\n", Symtable[id].position);
    break;

  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    if (op == A_PREINC)
      fprintf(Outfile, "\tincq\t%d(%%rbp)\n", Symtable[id].position);
    if (op == A_PREDEC)
      fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", Symtable[id].position);

    fprintf(Outfile, "\tmovq\t%d(%%rbp), %s\n", Symtable[id].position, reglist[r]);

    if (op == A_POSTINC)
      fprintf(Outfile, "\tincq\t%d(%%rbp)\n", Symtable[id].position);
    if (op == A_POSTDEC)
      fprintf(Outfile, "\tdecq\t%d(%%rbp)\n", Symtable[id].position);
    break;

  default:
    fatald("Bad type in `cgloadlocal()`:", Symtable[id].type);
  }

  return r;
}

// Given the label number of a global string, load its address into a new register
int cgloadglobalstr(int id)
{
  int r = alloc_register();
  fprintf(Outfile, "\tleaq\tL%d(%%rip), %s\n", id, reglist[r]);
  return r;
}

// Add 2 registers and return the number of the register with the result
int cgadd(int r1, int r2)
{
  fprintf(Outfile, "\taddq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return r2;
}

// Subtract the second register from the first
int cgsub(int r1, int r2)
{
  fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2],
          reglist[r1]); // source, destination ... (d - s) -> d
  free_register(r2);
  return r1;
}

// Multiply 2 registers and return the number of the register with the result
int cgmul(int r1, int r2)
{
  fprintf(Outfile, "\timulq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return r2;
}

// Divide the first register by the sceond and return the number of the register
// with the result
int cgdiv(int r1, int r2)
{
  fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n"); // Extend %rax to 8 bytes
  fprintf(Outfile, "\tidivq\t%s\n",
          reglist[r2]); // (%rax / r2) -> quotient in %rax, remainder in %rdx
  fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
  free_register(r2);
  return r1;
}

int cgand(int r1, int r2)
{
  fprintf(Outfile, "\tandq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return r2;
}

int cgor(int r1, int r2)
{
  fprintf(Outfile, "\torq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return r2;
}

int cgxor(int r1, int r2)
{
  fprintf(Outfile, "\txorq\t%s, %s\n", reglist[r1], reglist[r2]);
  free_register(r1);
  return r2;
}

int cgshl(int r1, int r2)
{
  fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
  fprintf(Outfile, "\tshlq\t%%cl, %s\n", reglist[r1]);
  free_register(r2);
  return r1;
}

int cgshr(int r1, int r2)
{
  fprintf(Outfile, "\tmovb\t%s, %%cl\n", breglist[r2]);
  fprintf(Outfile, "\tshrq\t%%cl, %s\n", reglist[r1]);
  free_register(r2);
  return r1;
}

// Negate a register's value
int cgnegate(int r)
{
  fprintf(Outfile, "\tnegq\t%s\n", reglist[r]);
  return r;
}

// Invert a register's value
int cginvert(int r)
{
  fprintf(Outfile, "\tnotq\t%s\n", reglist[r]);
  return r;
}

// Logically negate a register's value
int cglognot(int r)
{
  // `test` essentially ANDs the register with itself to set the zero and negative flags
  fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);
  // Then, set register to 1 if it is equal to zero (`sete`)
  fprintf(Outfile, "\tsete\t%s\n", breglist[r]);
  fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]); // 8-bit -> 64-bit
  return r;
}

// Convert an integer value to a boolean value. Jump if it's an IF or WHILE operation.
int cgboolean(int r, int op, int label)
{
  fprintf(Outfile, "\ttest\t%s, %s\n", reglist[r], reglist[r]);

  if (op == A_IF || op == A_WHILE)
    fprintf(Outfile, "\tje\tL%d\n", label); // Jump if result of `test` was false
  else
  {
    fprintf(Outfile, "\tsetnz\t%s\n", breglist[r]);
    fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r], reglist[r]);
  }

  return r;
}

// Call a function with one argument from the given register. Return register with result.
// Argument values goes into `%rdi`; return value comes from `%rax`.
int cgcall(int r, int id)
{
  // Get a new register
  int outr = alloc_register();
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\t%s\n", Symtable[id].name);
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
  free_register(r);
  return outr;
}

// Shift a register left by a constant
int cgshlconst(int r, int val)
{
  fprintf(Outfile, "\tsalq\t$%d, %s\n", val, reglist[r]);
  return r;
}

// Store a register's value into a global variable
int cgstoreglobal(int r, int id)
{
  switch (Symtable[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tmovb\t%s, %s(%%rip)\n", breglist[r], Symtable[id].name);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %s(%%rip)\n", dreglist[r], Symtable[id].name);
    break;
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t%s, %s(%%rip)\n", reglist[r], Symtable[id].name);
    break;
  default:
    fatald("Bad type in `cgstoreglobal()`:", Symtable[id].type);
  }

  return r;
}

// Store a register's value into a local variable
int cgstorlocal(int r, int id)
{
  switch (Symtable[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tmovb\t%s, %d(%%rbp)\n", breglist[r], Symtable[id].position);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %d(%%rbp)\n", dreglist[r], Symtable[id].position);
    break;
  case P_LONG:
  case P_CHARPTR:
  case P_INTPTR:
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t%s, %d(%%rbp)\n", reglist[r], Symtable[id].position);
    break;
  default:
    fatald("Bad type in cgstorlocal:", Symtable[id].type);
  }
  return r;
}

// Array of type sizes in P_XXX order (0 means no size)
static int psize[] = {
    0, // `P_NONE`
    0, // `P_VOID`
    1, // `P_CHAR`
    4, // `P_INT`
    8, // `P_LONG`
    8, // `P_VOIDPTR`
    8, // `P_CHARPTR`
    8, // `P_INTPTR`
    8, // `P_LONGPTR`
};

// Given a P_XXX type value, return the size of a primitive type in bytes
int cgprimsize(int type)
{
  if (type < P_NONE || type > P_LONGPTR)
    fatal("Bad type in `cgprimsize()`");
  return psize[type];
}

// Generate a global symbol, but not functions
void cgglobalsym(int id)
{
  if (Symtable[id].stype == S_FUNCTION)
    return;

  int typesize = cgprimsize(Symtable[id].type);

  // Generate the global identity and the label
  cgdataseg();
  fprintf(Outfile, "\t.globl\t%s\n", Symtable[id].name);
  fprintf(Outfile, "%s:", Symtable[id].name);

  // Generate the space
  for (int i = 0; i < Symtable[id].size; i++)
  {
    switch (typesize)
    {
    case 1:
      fprintf(Outfile, "\t.byte\t0\n");
      break;
    case 4:
      fprintf(Outfile, "\t.long\t0\n");
      break;
    case 8:
      fprintf(Outfile, "\t.quad\t0\n");
      break;
    default:
      fatald("Unknown typesize in `cgglobalsym()`: ", typesize);
    }
  }
}

// Generate a global string and its start label
void cgglobalstr(int l, char *strvalue)
{
  char *cptr;
  cglabel(l);
  for (cptr = strvalue; *cptr; cptr++)
  {
    fprintf(Outfile, "\t.byte\t%d\n", *cptr);
  }
  fprintf(Outfile, "\t.byte\t0\n");
}

// Comparison instructions in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *cmplist[] = {"sete", "setne", "setl", "setg", "setle", "setge"};

// Compare 2 registers and set `r2` if true
int cgcompare_and_set(int ASTop, int r1, int r2)
{
  // Check the range of the AST operation
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in `cgcompare_and_set()`");

  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\t%s\n", cmplist[ASTop - A_EQ], breglist[r2]);
  fprintf(Outfile, "\tmovzbq\t%s, %s\n", breglist[r2], reglist[r2]);
  free_register(r1);
  return r2;
}

// Generate a label
void cglabel(int l) { fprintf(Outfile, "L%d:\n", l); }

// Generate a jump to a label
void cgjump(int l) { fprintf(Outfile, "\tjmp\tL%d\n", l); }

// Inverted jump instructions: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] = {"jne", "je", "jge", "jle", "jg", "jl"};

// Compare 2 registers and jump if FALSE
int cgcompare_and_jump(int ASTop, int r1, int r2, int label)
{
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in `cgcompare_and_jump()`");

  fprintf(Outfile, "\tcmpq\t%s, %s\n", reglist[r2], reglist[r1]);
  fprintf(Outfile, "\t%s\tL%d\n", invcmplist[ASTop - A_EQ], label);
  freeall_registers();
  return NOREG;
}

// Widen the value in the register from the old to the new type and return the
// register with the new value
int cgwiden(int r, int oldtype, int newtype)
{
  // Nothing to do
  return r;
}

// Generate code to return a value from a function
void cgreturn(int reg, int id)
{
  switch (Symtable[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tmovzbl\t%s, %%eax\n", breglist[reg]);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %%eax\n", dreglist[reg]);
    break;
  case P_LONG:
    fprintf(Outfile, "\tmovq\t%s, %%rax\n", reglist[reg]);
    break;
  default:
    fatald("Bad function type in `cgreturn()`:", Symtable[id].type);
  }

  cgjump(Symtable[id].endlabel);
}

// Generate code to load the address of a global identifier into
// a variable. Return a new register.
int cgaddress(int id)
{
  int r = alloc_register();

  if (Symtable[id].class == C_LOCAL)
    fprintf(Outfile, "\tleaq\t%d(%%rbp), %s\n", Symtable[id].position, reglist[r]);
  else
    fprintf(Outfile, "\tleaq\t%s(%%rip), %s\n", Symtable[id].name, reglist[r]);

  return r;
}

// Dereference a pointer to get the value it points at in the same register
int cgderef(int r, int type)
{
  switch (type)
  {
  case P_CHARPTR:
    fprintf(Outfile, "\tmovzbq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  case P_INTPTR:
    fprintf(Outfile, "\tmovslq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  default:
    fatald("Can't `cgderef()` on type:", type);
  }

  return r;
}

// Store through a dereferenced pointer
int cgstorderef(int r1, int r2, int type)
{
  switch (type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tmovb\t%s, (%s)\n", breglist[r1], reglist[r2]);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovq\t%s, (%s)\n", reglist[r1], reglist[r2]);
    break;
  case P_LONG:
    fprintf(Outfile, "\tmovq\t%s, (%s)\n", reglist[r1], reglist[r2]);
    break;
  default:
    fatald("Can't `cgstoderef()` on type:", type);
  }

  return r1;
}
