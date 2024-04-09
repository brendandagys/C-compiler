#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Code generator for x86-64

// q-suffix means 'quadword' (32 bits)

// List of available registers and their names
static int freereg[4];
static char *reglist[4] = {"%r8", "%r9", "%r10", "%r11"};
static char *breglist[4] = {"%r8b", "%r9b", "%r10b", "%r11b"};
static char *dreglist[4] = {"%r8d", "%r9d", "%r10d", "%r11d"};

// Set all registers as available
void freeall_registers(void)
{
  freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

// Allocate a free register. Return the number of the register. Terminate if none free.
static int alloc_register(void)
{
  for (int i = 0; i < 4; i++)
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

  fputs("\t.text\n",
        // ".LC0:\n"   // Label to reference a string literal ("%d\n")
        // "\t.string\t\"%d\\n\"\n"
        // "printint:\n" // Function label
        // // SET UP STACK FRAME
        // "\tpushq\t%rbp\n"      // Push base pointer onto stack (reference point for
        //                        // accessing local variables and parameters within a
        //                        // function)
        // "\tmovq\t%rsp, %rbp\n" // Move stack pointer to be at base pointer

        // "\tsubq\t$16, %rsp\n"       // Subtract 16 bytes from stack pointer (allocate
        //                             // space on stack for local variables)
        // "\tmovl\t%edi, -4(%rbp)\n"  // Move `printint` argument to stack
        // "\tmovl\t-4(%rbp), %eax\n"  // Move `printint` argument from stack to EAX
        //                             // register
        // "\tmovl\t%eax, %esi\n"      // Move `printint` argument into ESI (second
        //                             // argument for `printf`)
        // "\tleaq	.LC0(%rip), %rdi\n" // Load address of string literal into RDI
        //                             // (first argument for `printf`)
        // "\tmovl	$0, %eax\n"         // Clear EAX
        // "\tcall	printf@PLT\n"
        // "\tnop\n"
        // "\tleave\n" // Clean up stack frame before returning
        // "\tret\n"
        // "\n",
        Outfile);
}

// Nothing to do
void cgpostamble() {}

// Print out a function preamble
void cgfuncpreamble(int id)
{
  char *name = Gsym[id].name;

  fprintf(Outfile,
          "\t.text\n"
          "\t.globl\t%s\n"
          "\t.type\t%s, @function\n"
          "%s:\n"
          "\tpushq\t%%rbp\n"
          "\tmovq\t%%rsp, %%rbp\n",
          name, name, name);
}

// Print out a function postamble
void cgfuncpostamble(int id)
{
  cglabel(Gsym[id].endlabel);
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

int cgloadglob(int id)
{
  int r = alloc_register();

  // Print out the code to initialize it
  switch (Gsym[id].type)
  {
  case P_CHAR: // `movzbq` zeros 8-byte register and moves 1 byte into it (this widens the char)
    fprintf(Outfile, "\tmovzbq\t%s(\%%rip), %s\n", Gsym[id].name, reglist[r]);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovzbl\t%s(\%%rip), %s\n", Gsym[id].name, reglist[r]);
    break;
  case P_LONG:
    fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", Gsym[id].name, reglist[r]);
    break;
  default:
    fatald("Bad type in cgloadglob:", Gsym[id].type);
  }

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

// There's no x86-64 instruction to print a register as a decimal, so the
// preamble contains `printint()` that takes a register argument and calls
// `printf()`
void cgprintint(int r)
{
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\tprintint\n");
  free_register(r);
}

// Call a function with one argument from the given register. Return register with result.
// Argument values goes into `%rdi`; return value comes from `%rax`.
int cgcall(int r, int id)
{
  // Get a new register
  int outr = alloc_register();
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\t%s\n", Gsym[id].name);
  fprintf(Outfile, "\tmovq\t%%rax, %s\n", reglist[outr]);
  free_register(r);
  return outr;
}

// Store a register's value into a variable
int cgstorglob(int r, int id)
{
  switch (Gsym[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tmovb\t%s, %s(\%%rip)\n", breglist[r], Gsym[id].name);
    break;
  case P_INT:
    fprintf(Outfile, "\tmovl\t%s, %s(\%%rip)\n", dreglist[r], Gsym[id].name);
    break;
  case P_LONG:
    fprintf(Outfile, "\tmovq\t%s, %s(\%%rip)\n", reglist[r], Gsym[id].name);
    break;
  default:
    fatald("Bad type in cgstorglob:", Gsym[id].type);
  }

  return r;
}

// Array of type sizes in P_XXX order (0 means no size)
static int psize[] = {0,  // `P_NONE`
                      0,  // `P_VOID`
                      1,  // `P_CHAR`
                      4,  // `P_INT`
                      8}; // `P_LONG`

// Given a P_XXX type value, return the size of a primitive type in bytes
int cgprimsize(int type)
{
  if (type < P_NONE || type > P_LONG)
    fatal("Bad type in cgprimsize()");
  return psize[type];
}

// Generate a global symbol
void cgglobsym(int id)
{
  int typesize;

  typesize = cgprimsize(Gsym[id].type);

  fprintf(Outfile, "\t.comm\t%s,%d,%d\n", Gsym[id].name, typesize, typesize);
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

// Inverted jump instructions
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
  switch (Gsym[id].type)
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
    fatald("Bad function type in cgreturn:", Gsym[id].type);
  }

  cgjump(Gsym[id].endlabel);
}

// Generate code to load the address of a global identifier into
// a variable. Return a new register.
int cgaddress(int id)
{
  int r = alloc_register();
  fprintf(Outfile, "\tleaq\t%s(\%%rip), %s\n", Gsym[id].name, reglist[r]);
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
  case P_LONGPTR:
    fprintf(Outfile, "\tmovq\t(%s), %s\n", reglist[r], reglist[r]);
    break;
  }

  return r;
}
