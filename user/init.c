#include <stdio.h>

int main(int argc, char **argv)
{
	int fd = -1;
	if (open("/ttyc", O_RDWR) < 0) {
		mknod("/ttyc", S_IFCHR, makedev(3,0));
		open("/ttyc", O_RDWR);
	}

	dup(0);
	dup(0);

	printf("init.\n");

	if (!fork())
		execv("/sh", argv);
	else
		while (1)
			waitpid(-1, WNOHANG);
}