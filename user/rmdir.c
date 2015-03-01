#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("rmdir: missing operand\n");
		return -1;
	}
	int r = rmdir(argv[1]);
	if (r < 0) {
		printf("rmdir error: %d\n", errno);
		return errno;
	}
}