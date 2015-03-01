#include <kernel/kernel.h>

// Entrypoint - this is where the kernel (or our parent environment)
// starts us running when we are initially loaded into a new environment.
.text
.globl _start, main
_start:
	// See if we were started with arguments on the stack
	cmpl $USTACKTOP, %esp
	jne args_exist

	// If not, push dummy argc/argv arguments.
	// This happens when we are loaded by the kernel,
	// because the kernel does not know about passing arguments.
	pushl $0
	pushl $0

args_exist:
	call main
	call exit
1:	jmp 1b