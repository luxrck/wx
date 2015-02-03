#include <stdio.h>

void kshell(void)
{
	while (1) {
		char *cmd = readline("W> ");
		if (cmd)
			printf("Command %s is not implemented.\n", cmd);
	}
}
