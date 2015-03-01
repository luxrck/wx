// AB
#include <stdio.h>

int main(int argc, char **argv)
{
	if (fork() == 0) {
		for (int i=0; i<52; i++)
			putchar('A');
	} else {
		for (int i=0; i<50; i++)
			putchar('B');
		waitpid(-1, 0);
	}
}
