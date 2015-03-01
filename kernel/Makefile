include ../Makefile.header

CFLAGS 		+= -I../include -I..
KLDFLAGS	:= $(LDFLAGS) -T kernel.ld

SRC			:= entry.S			\
			bootpgdir.c			\
			init.c				\
			kclock.c			\
			pic.c				\
			sched.c				\
			mm.c				\
			ide.c				\
			pipe.c				\
			console.c			\
			device.c			\
			fs.c				\
			proc.c				\
			syscall.c			\
			trapentry.S			\
			trap.c				\
			kshell.c			\
			printf.c			\
			../lib/assert.c		\
			../lib/string.c		\
			../lib/printfmt.c	\
			../lib/readline.c

OBJ			:= $(patsubst %.c, %.o, $(SRC))
OBJ			:= $(patsubst %.S, %.o, $(OBJ))

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: ../lib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel: kernel.ld $(OBJ)
	$(LD) -o $@ $(KLDFLAGS) $(OBJ) $(GCC_LIB)
	$(OBJDUMP) -S $@ > $@.asm

clean:
	rm -f *.o *.d kernel kernel.asm
