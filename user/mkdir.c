#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("mkdir: missing operand\n");
		return -1;
	}
	int r = mkdir(argv[1]);
	if (r < 0) {
		printf("mkdir error: %d\n", errno);
		return -errno;
	}
	return 0;
}