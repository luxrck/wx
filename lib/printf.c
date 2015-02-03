#include <stdio.h>
#include <string.h>
#include <ctype.h>

// used by printf sprintf fprintf
struct printbuf {
	int index;
	int len;
	int tolwrite;
	void *data;
};

#define PRINTBUFLEN	1024
typedef void(*printcallback)(int c, struct printbuf *data);

static void putch(int c, struct printbuf *data)
{
	putchar(c);
	if (data) data->tolwrite++;
}
static void printint(printcallback pc, struct printbuf *data,
		uint32_t num, int neg, int width, int prec, uint8_t base)
{
	static char *number = "0123456789ABCDEF";
	if (num < base) {
		if (width < 0) width = 0;
		if (prec < 0) prec = 0;
		width -= prec;
		if (neg) width--;
		while (--width > 0) pc(' ', data);
		if (neg) pc('-', data);
		while (--prec > 0) pc('0', data);
		pc(number[num], data);
	} else {
		printint(pc, data, num / base, neg, width - 1, prec - 1, base);
		pc(number[num % base], data);
	}
}

void vprintfmt(printcallback pc, struct printbuf *data, const char *fmt, va_list ap)
{
	while (1) {
		while (*fmt != '%') {
			if (*fmt == '\0')
				return;
			pc(*fmt++, data);
		}
		const char *bfmt = fmt++;
		int width = 0, prec = 0, flag = 0, neg = 0;
		int32_t num = 0;
		uint8_t base = 10;
_switch:
		switch (*fmt++) {
		case 'd':
		case 'i':
			num = va_arg(ap, int);
			if (num < 0) {
				num = -num;
				neg = 1;
			}
_printint:
			printint(pc, data, num, neg, width, prec, base);
			break;
		case 'o':
			base = 8;
			goto _printuint;
		case 'x':
			base = 16;
			goto _printuint;
		case 'u':
			base = 10;
_printuint:
			num = va_arg(ap, uint32_t);
			goto _printint;
		case '.':
			flag = 1;
			goto _switch;
		case 's': {
			char *ss = va_arg(ap, char *);
			int ls = strlen(ss);
			if (prec > ls) prec = ls;
			if (width > prec) width = prec;
			while (width-- > 0) pc(' ', data);
			while (prec-- > 0) pc(*ss++, data);
			break;
		}
		case 'c':
			pc(va_arg(ap, int), data);
			break;
		default:
			fmt--;
			if (!isdigit(*fmt))
				while (bfmt != fmt) pc(*bfmt++, data);
			else {
				int dig = 0;
				while (isdigit(*fmt)) {
					dig = dig * 10 + *fmt - '0';
					fmt++;
				}
				flag ? (prec = dig) : (width = dig);
				goto _switch;
			}
		}
	}
}

int vprintf(const char *fmt, va_list ap)
{
	char *cbuf[PRINTBUFLEN];
	struct printbuf buf;
	buf.len = PRINTBUFLEN;
	buf.tolwrite = 0;
	buf.data = cbuf;
	vprintfmt(putch, &buf, fmt, ap);
	return buf.tolwrite;
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
