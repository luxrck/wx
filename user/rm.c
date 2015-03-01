#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("rm: missing operand\n");
		return -1;
	}
	int r = 0;
	for (int i=1; i<argc; i++) {
		if ((r = unlink(argv[i])) < 0) {
			printf("rm error: %d\n", errno);
			return -errno;
		}
	}
	return 0;
}