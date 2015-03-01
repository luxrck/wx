#ifndef __PRINTF_H__
#define __PRINTF_H__

#define PRINTBUFLEN	128

// used by printf sprintf fprintf
struct printbuf {
	int fd;
	int cpos;
	int tolwrite;
	int error;
	char data[PRINTBUFLEN];
};

typedef void(*printcallback)(int c, void *data);

void vprintfmt(printcallback pc, void *data, const char *fmt, va_list ap);

#endif /* !__PRINTF_H__ */