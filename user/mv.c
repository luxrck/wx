#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
	if (argc < 3) {
		printf("mv: missing operand\n");
		return -1;
	}
	int r = rename(argv[1], argv[2]);
	if (r < 0) {
		printf("mv error: %d\n", errno);
		return -errno;
	}
	return 0;
}