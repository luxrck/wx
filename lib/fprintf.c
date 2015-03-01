#include <stdio.h>

#include <lib/printf.h>

static void __flushbuf(struct printbuf *b)
{
	if (b->error < 0) return;
	int r = write(b->fd, b->data, b->cpos);
	if (r > 0) b->tolwrite += r;
	if (r < 0) b->error = r;
}

static void putch(int c, void *data)
{
	struct printbuf *b = (struct printbuf *)data;
	b->data[b->cpos++] = c;
	if (b->cpos == PRINTBUFLEN) {
		__flushbuf(b);
		b->cpos = 0;
	}
}

int vfprintf(int fd, const char *fmt, va_list ap)
{
	struct printbuf b = {0};
	b.fd = fd;
	vprintfmt(putch, &b, fmt, ap);
	if (b.cpos > 0) __flushbuf(&b);
	return (b.tolwrite ? b.tolwrite : b.error);
}

int fprintf(int fd, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vfprintf(fd, fmt, ap);
	va_end(ap);

	return ret;
}

int vprintf(const char *fmt, va_list ap)
{
	return vfprintf(1, fmt, ap);
}

int printf(const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vfprintf(1, fmt, ap);
	va_end(ap);

	return ret;
}