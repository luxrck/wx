include ../Makefile.header

CFLAGS		+= -I../include -I..
ULDFLAGS	:= $(LDFLAGS) -T user.ld -nostdlib

%: %.c ../lib/entry.o user.ld
	@rm -f _$@
	$(CC) $(CFLAGS) -c $<
	$(LD) -o $@ $(ULDFLAGS) ../lib/entry.o $@.o -L../lib -lc
	$(OBJDUMP) -S $@ > $@.asm
	@mv $@ _$@

all: $(UBIN)

clean:
	rm -f *.o *.d *.asm _*
