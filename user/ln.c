#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
	if (argc < 3) {
		printf("ln: missing operand\n");
		return -1;
	}
	int r = link(argv[1], argv[2]);
	if (r < 0) {
		printf("ln error: %d\n", errno);
		return -errno;
	}
	return 0;
}