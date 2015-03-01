#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <lib/printf.h>

void printint(printcallback pc, void *data,
		uint32_t num, int neg, int width, int prec, int base)
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

void vprintfmt(printcallback pc, void *data, const char *fmt, va_list ap)
{
	while (1) {
		while (*fmt != '%') {
			if (*fmt == '\0')
				return;
			pc(*fmt++, data);
		}
		const char *bfmt = fmt++, *ss = NULL;
		int width = 0, prec = 0, flag = 0, neg = 0, num = 0, le = 0;
		uint32_t base = 10;
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
			num = va_arg(ap, int);
			printint(pc, data, num, 0, width, prec, base);
			break;
		case '.':
			flag = 1;
			goto _switch;
		case 's':
			ss = va_arg(ap, const char*);
			le = strlen(ss);
			for (int i=0; i<le; i++) {
				pc(*ss++, data);
			}
			/*
			if (!prec || prec > ls) prec = ls;
			if (width > prec) width -= prec;
			for (int i=0; i<width; i++) {
				pc(' ', data);
			}
			*/
			break;
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