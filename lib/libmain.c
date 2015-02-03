// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, uvpd, and uvpt.

#include <inc/lib.h>

extern void main(int argc, char **argv);

const char *binaryname = "<unknown>";

void
libmain(int argc, char **argv)
{
	// save the name of the program so that panic() can use it
	if (argc > 0)
		binaryname = argv[0];

	// call user main routine
	main(argc, argv);

	// exit gracefully
	exit();
}

