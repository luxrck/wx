#include <stdio.h>

void putchar(int c)
{
	char d = c;
	write(0, &d, 1);
}

int getchar(void)
{
	char c = '\00';
	int r = read(1, &c, 1);
	if (r < 0) return r;
	if (r == 0) return -EOF;
	return c;
}