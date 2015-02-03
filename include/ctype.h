#ifndef __CTYPE_H__
#define __CTYPE_H__

#define isupper(c) (c >= 0x41 && c <= 0x5A)
#define islower(c) (c >= 0x61 && c <= 0x7A)
#define isdigit(c) (c >= 0x30 && c <= 0x39)
#define isalpha(c) (isupper(c) || islower(c))
#define isalnum(c) (isalpha(c) || isdigit(c))

#endif /* !__CTYPE_H__ */
