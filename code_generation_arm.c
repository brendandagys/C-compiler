#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Code generator for ARMv6 on Raspberry Pi

// List of available registers and their names
static int freereg[4];
static char *reglist[4] = {"r4", "r5", "r6", "r7"};

// Set all registers as available
void freeall_registers(void)
{
  freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

// Allocate a free register and return its number
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

static void free_register(int reg)
{
  if (freereg[reg] != 0)
    fatald("Error trying to free register", reg);
  freereg[reg] = 1;
}

// We have to store large integer literal values in memory.
// Keep a list of them that will be output in the postamble.
#define MAXINTS 1024

int Intlist[MAXINTS];
static int Intslot = 0;

// Determine the offset of a large integer literal from the `.L3` label.
// If integer isn't in the list, add it.
static void set_int_offset(int val)
{
  int offset = -1;

  for (int i = 0; i < Intslot; i++)
  {
    if (Intlist[i] == val)
    {
      offset = 4 * i;
      break;
    }
  }

  if (offset == -1)
  {
    if (Intslot == MAXINTS)
      fatal("Out of int slots in `set_int_offset()`");

    offset = 4 * Intslot;

    Intlist[Intslot++] = val;
  }

  // Load `r3` with this offset (address of integer)
  fprintf(Outfile, "\tldr\tr3, .L3+%d\n", offset);
}

// Print out the assembly preamble
void cgpreamble(void)
{
  freeall_registers();
  fputs("\t.text\n", Outfile);
}

// Print out the assembly postamble
void cgpostamble()
{
  // Print out the global variables
  fprintf(Outfile, ".L2:\n");
  for (int i = 0; i < Globs; i++)
  {
    if (Gsym[i].stype == S_VARIABLE)
      fprintf(Outfile, "\t.word %s\n", Gsym[i].name);
  }

  // Print out the integer literals
  fprintf(Outfile, ".L3:\n");
  for (int i = 0; i < Intslot; i++)
  {
    fprintf(Outfile, "\t.word %d\n", Intlist[i]);
  }
}

// Print out a function preamble
void cgfuncpreamble(int id)
{
  char *name = Gsym[id].name;
  fprintf(Outfile,
          "\t.text\n"
          "\t.globl\t%s\n"
          "\t.type\t%s, \%%function\n"
          "%s:\n"
          "\tpush\t{fp, lr}\n"
          "\tadd\tfp, sp, #4\n"
          "\tsub\tsp, sp, #8\n"
          "\tstr\tr0, [fp, #-8]\n",
          name, name, name);
}

// Print out a function postamble
void cgfuncpostamble(int id)
{
  cglabel(Gsym[id].endlabel);
  fputs("\tsub\tsp, fp, #4\n"
        "\tpop\t{fp, pc}\n"
        "\t.align\t2\n",
        Outfile);
}

// Load an integer literal value into a register, returning its number
int cgloadint(int value, int type)
{
  int r = alloc_register();

  // If the literal value is small, do it with 1 instruction
  if (value <= 1000)
    fprintf(Outfile, "\tmov\t%s, #%d\n", reglist[r], value);
  else
  {
    set_int_offset(value);
    fprintf(Outfile, "\tldr\t%s, [r3]\n", reglist[r]);
  }

  return r;
}

// Determine the offset of a variable from the `.L2` label (inefficient)
static void set_var_offset(int id)
{
  int offset = 0;

  for (int i = 0; i < id; i++)
  {
    if (Gsym[i].stype == S_VARIABLE)
      offset += 4;
  }

  fprintf(Outfile, "\tldr\tr3, .L2+%d\n", offset); // Load `r3` with the offset
}

// Load a value from a variable into a register and return its number
int cgloadglob(int id)
{
  int r = alloc_register();
  set_var_offset(id);
  fprintf(Outfile, "\tldr\t%s, [r3]\n", reglist[r]);
  return r;
}

// Add 2 registers and return the register with the result
int cgadd(int r1, int r2)
{
  fprintf(Outfile, "\tadd\t%s, %s, %s\n", reglist[r2], reglist[r1],
          reglist[r2]);
  free_register(r1);
  return r2;
}

// Subtract the 2nd register from the 1st and return the register with the result
int cgsub(int r1, int r2)
{
  fprintf(Outfile, "\tsub\t%s, %s, %s\n", reglist[r1], reglist[r1],
          reglist[r2]);
  free_register(r2);
  return r1;
}

// Multiply 2 registers and return the register with the result
int cgmul(int r1, int r2)
{
  fprintf(Outfile, "\tmul\t%s, %s, %s\n", reglist[r2], reglist[r1],
          reglist[r2]);
  free_register(r1);
  return r2;
}

// Divide the 1st register by the 2nd and return the register with the result
int cgdiv(int r1, int r2)
{
  // `r0` holds the dividend, `r1` holds the divisor. The quotient will be in `r0`.
  fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r1]);
  fprintf(Outfile, "\tmov\tr1, %s\n", reglist[r2]);
  fprintf(Outfile, "\tbl\t__aeabi_idiv\n");
  fprintf(Outfile, "\tmov\t%s, r0\n", reglist[r1]);
  free_register(r2);
  return r1;
}

// Call `printint()` with the given register
void cgprintint(int r)
{
  fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r]);
  fprintf(Outfile, "\tbl\tprintint\n");
  fprintf(Outfile, "\tnop\n");
  free_register(r);
}

// Call a function with 1 argument from the given register; return register with result
int cgcall(int r, int id)
{
  fprintf(Outfile, "\tmov\tr0, %s\n", reglist[r]);
  fprintf(Outfile, "\tbl\t%s\n", Gsym[id].name);
  fprintf(Outfile, "\tmov\t%s, r0\n", reglist[r]);
  return r;
}

// Store a register's value into a variable
int cgstorglob(int r, int id)
{
  set_var_offset(id);

  switch (Gsym[id].type)
  {
  case P_CHAR:
    fprintf(Outfile, "\tstrb\t%s, [r3]\n", reglist[r]);
    break;
  case P_INT:
  case P_LONG:
    fprintf(Outfile, "\tstr\t%s, [r3]\n", reglist[r]);
    break;
  default:
    fatald("Bad type in `cgstorglob()`", Gsym[id].type);
  }

  return r;
}

// Array of type sizes in P_XXX order (0 means no size)
static int psize[] = {0, 0, 1, 4, 4};

// Given a P_XXX type value, return the size of a primitive type in bytes
int cgprimsize(int type)
{
  if (type < P_NONE || type > P_LONG)
    fatal("Bad type in `cgprimsize()`");

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
static char *cmplist[] = {"moveq", "movne", "movlt", "movgt", "movle", "movge"};

// Inverted jump instructions in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *invcmplist[] = {"movne", "moveq", "movge", "movle", "movgt", "movlt"};

// Compare 2 registers and set if true
int cgcompare_and_set(int ASTop, int r1, int r2)
{
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in `cgcompare_and_set()`");

  fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
  fprintf(Outfile, "\t%s\t%s, #1\n", cmplist[ASTop - A_EQ], reglist[r2]);
  fprintf(Outfile, "\t%s\t%s, #0\n", invcmplist[ASTop - A_EQ], reglist[r2]);
  // Unsigned extend byte: zero-extend a byte to a 32-bit word (1: destination, 2: source)
  fprintf(Outfile, "\tuxtb\t%s, %s\n", reglist[r2], reglist[r2]);
  free_register(r1);
  return r2;
}

// Generate a label
void cglabel(int l)
{
  fprintf(Outfile, "L%d:\n", l);
}

// Generate a jump to a label
void cgjump(int l)
{
  fprintf(Outfile, "\tb\tL%d\n", l);
}

// List of inverted branch instructions in AST order: A_EQ, A_NE, A_LT, A_GT, A_LE, A_GE
static char *brlist[] = {"bne", "beq", "bge", "ble", "bgt", "blt"};

// Compare 2 registers and jump if false
int cgcompare_and_jump(int ASTop, int r1, int r2, int label)
{
  if (ASTop < A_EQ || ASTop > A_GE)
    fatal("Bad ASTop in `cgcompare_and_set()`");

  fprintf(Outfile, "\tcmp\t%s, %s\n", reglist[r1], reglist[r2]);
  fprintf(Outfile, "\t%s\tL%d\n", brlist[ASTop - A_EQ], label);
  freeall_registers();
  return NOREG;
}

// Widen value in register from old to new type, and return a register with the result
int cgwiden(int r, int oldtype, int newtype)
{
  return r; // Nothing to do
}

// Generate code to return a value from a function
void cgreturn(int reg, int id)
{
  fprintf(Outfile, "\tmov\tr0, %s\n", reglist[reg]);
  cgjump(Gsym[id].endlabel);
}