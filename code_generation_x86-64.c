#include "definitions.h"
#include "data.h"
#include "declarations.h"

// Code generator for x86-64

// q-suffix means 'quadword' (32 bits)

// List of available registers and their names
static int freereg[4];
static char *reglist[4] = {"%r8", "%r9", "%r10", "%r11"};

// Set all registers as available
void freeall_registers(void)
{
  freereg[0] = freereg[1] = freereg[2] = freereg[3] = 1;
}

// Allocate a free register. Return the number of the register. Terminate if none are free.
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

  fputs(
      "\t.text\n" // Text section
      ".LC0:\n"   // Label to reference a string literal ("%d\n")
      "\t.string\t\"%d\\n\"\n"
      "printint:\n" // Function label
      // SET UP STACK FRAME
      "\tpushq\t%rbp\n"      // Push base pointer onto stack (reference point for accessing local variables and parameters within a function)
      "\tmovq\t%rsp, %rbp\n" // Move stack pointer to be at base pointer

      "\tsubq\t$16, %rsp\n"       // Subtract 16 bytes from stack pointer (allocate space on stack for local variables)
      "\tmovl\t%edi, -4(%rbp)\n"  // Move `printint` argument to stack
      "\tmovl\t-4(%rbp), %eax\n"  // Move `printint` argument from stack to EAX register
      "\tmovl\t%eax, %esi\n"      // Move `printint` argument into ESI (second argument for `printf`)
      "\tleaq	.LC0(%rip), %rdi\n" // Load address of string literal into RDI (first argument for `printf`)
      "\tmovl	$0, %eax\n"         // Clear EAX
      "\tcall	printf@PLT\n"
      "\tnop\n"
      "\tleave\n" // Clean up stack frame before returning
      "\tret\n"
      "\n"
      "\t.globl\tmain\n" // Declares `main` function as global, making it accessible to other program parts
      "\t.type\tmain, @function\n"
      "main:\n"
      // SET UP STACK FRAME
      "\tpushq\t%rbp\n"
      "\tmovq	%rsp, %rbp\n",
      Outfile);
}

// Print out the assembly postamble
void cgpostamble(void)
{
  fputs(
      "\tmovl	$0, %eax\n"
      "\tpopq	%rbp\n"
      "\tret\n",
      Outfile);
}

// Load an integer literal value into a register and return the register number
int cgloadint(int value)
{
  int r = alloc_register();
  fprintf(Outfile, "\tmovq\t$%d, %s\n", value, reglist[r]);
  return r;
}

int cgloadglob(char *identifier)
{
  int r = alloc_register();
  fprintf(Outfile, "\tmovq\t%s(\%%rip), %s\n", identifier, reglist[r]);
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
  fprintf(Outfile, "\tsubq\t%s, %s\n", reglist[r2], reglist[r1]); // source, destination ... (d - s) -> d
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

// Divide the first register by the sceond and return the number of the register with the result
int cgdiv(int r1, int r2)
{
  fprintf(Outfile, "\tmovq\t%s,%%rax\n", reglist[r1]);
  fprintf(Outfile, "\tcqo\n");                    // Extend %rax to 8 bytes
  fprintf(Outfile, "\tidivq\t%s\n", reglist[r2]); // (%rax / r2) -> quotient in %rax, remainder in %rdx
  fprintf(Outfile, "\tmovq\t%%rax,%s\n", reglist[r1]);
  free_register(r2);
  return (r1);
}

// There's no x86-64 instruction to print a register as a decimal, so the
// preamble contains `printint()` that takes a register argument and calls `printf()`
void cgprintint(int r)
{
  fprintf(Outfile, "\tmovq\t%s, %%rdi\n", reglist[r]);
  fprintf(Outfile, "\tcall\tprintint\n");
  free_register(r);
}

// Store a register's value into a variable
int cgstorglob(int r, char *identifier)
{
  fprintf(Outfile, "\tmovq\t%s, %s(\%%rip)\n", reglist[r], identifier);
  return r;
}

// Generate a global symbol
void cgglobsym(char *sym)
{
  fprintf(Outfile, "\t.comm\t%s,8,8\n", sym);
}
