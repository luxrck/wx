#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <lib/printf.h>

static void putch(int c, void *data)
{
	struct printbuf *b = (struct printbuf*)data;
	putchar(c);
	if (b) b->tolwrite++;
}


int vprintf(const char *fmt, va_list ap)
{
	struct printbuf b = {0};
	vprintfmt(putch, &b, fmt, ap);
	return b.tolwrite;
}

int printf(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);

	return ret;
}
