include ../Makefile.header

CFLAGS		+= -I../include -I.. -mpreferred-stack-boundary=2
ULDFLAGS	:= $(LDFLAGS) -T user.ld

UBIN	:= ab init cat cp echo ln ls mkdir mv rm rmdir sh

%: %.c ../lib/entry.o user.ld
	$(CC) $(CFLAGS) -c $<
	$(LD) -o $@ $(ULDFLAGS) ../lib/entry.o $@.o -L../lib -lc
	$(OBJDUMP) -S $@ > $@.asm

all: $(UBIN)

clean:
	rm -f *.o *.d *.asm $(UBIN)
