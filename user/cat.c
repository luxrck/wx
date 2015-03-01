#include <stdio.h>
#include <errno.h>

char buf[129];
int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("cat: missing operand\n");
		return -1;
	}
	int fd = 0, r = 0;
	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		printf("cat error: %d\n", errno);
		return errno;
	}
	while ((r = read(fd, buf, 128))) {
		if (r > 0)
			printf("%s", buf);
		else {
			printf("cat error: %d\n", errno);
			return -errno;
		}
	}
	return 0;
}