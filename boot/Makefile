include ../Makefile.header

CFLAGS += -I../include

boot: init.o boot.o
	ld $(LDFLAGS) -N -e start16 -Ttext 0x7c00 -o $@.out $^
	$(OBJDUMP) -S $@.out > $@.asm
	objcopy -S -O binary -j .text $@.out $@
	perl sign.pl $@

init.o: init.S
#	as --32 -o $@ init.S
	$(CC) $(CFLAGS) -c -o $@ $<

boot.o: boot.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.d boot boot.out *.asm
