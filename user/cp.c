#include <stdio.h>
#include <errno.h>

char buf[512];
int main(int argc, char **argv)
{
	if (argc < 3) {
		printf("cp: missing operand\n");
		return -1;
	}
	int f1 = open(argv[1], O_RDONLY);
	int f2 = open(argv[2], O_WRONLY|O_CREAT);
	int r = 0;
	while ((r = read(f1, buf, 512))) {
		if (r > 0)
			write(f2, buf, r);
		else if (r < 0) {
			printf("cp error: %d\n", errno);
			return -errno;
		}
	}
	close(f1);
	close(f2);
	return 0;
}