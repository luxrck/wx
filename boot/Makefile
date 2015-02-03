include ../Makefile.header

CFLAGS += -I../include

boot: init.o boot.o
	ld $(LDFLAGS) -N -e start16 -Ttext 0x7c00 -o $@.out $^
	objcopy -S -O binary -j .text $@.out $@

init.o: init.S
#	as --32 -o $@ init.S
	$(CC) $(CFLAGS) -c -o $@ $<

boot.o: boot.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm *.o *.d boot boot.out
