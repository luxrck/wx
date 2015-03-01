#include <stdio.h>
#include <errno.h>
#include <string.h>

struct dir {
	uint16_t inum;
	char name[14];
};

char *dargv[] = {"ls", ".", NULL};

static const char* basename(const char *path)
{
	const char *name = path + strlen(path);
	while (name > path && *name != '/') name--;
	return *name == '/' ? name + 1 : name;
}

int main(int argc, char **argv)
{
	if (argc < 2)
		argv = dargv;
	struct dir dirent = {0};
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		printf("ls error: %d\n", errno);
		return -errno;
	}
	struct stat st = {0};
	fstat(fd, &st);
	int r = 0;
	if (st.st_mode == 1) {
		while ((r = readdir(fd, &dirent))) {
			if (r > 0)
				printf("%d %s\n", dirent.inum, dirent.name);
			else {
				printf("ls error: %d\n", errno);
				return -errno;
			}
		}
	} else {
		printf("%d %s\n", st.st_ino, basename(argv[1]));
	}
	return 0;
}