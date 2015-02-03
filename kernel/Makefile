include ../Makefile.header

CFLAGS 		+= -I../include -I..
KLDFLAGS	:= $(LDFLAGS) -T kernel.ld -nostdlib

UBIN		:= $(addprefix ../user/_, $(UBIN))

QEMU		:= qemu
QEMUOPT		:= -kernel kernel -gdb tcp::23456

SRC			:= entry.S			\
			bootpgdir.c			\
			init.c				\
			kclock.c			\
			console.c			\
			pic.c				\
			sched.c				\
			mm.c				\
			proc.c				\
			syscall.c			\
			trapentry.S			\
			trap.c				\
			kshell.c			\
			../lib/string.c		\
			../lib/printf.c		\
			../lib/readline.c

OBJ			:= $(patsubst %.c, %.o, $(SRC))
OBJ			:= $(patsubst %.S, %.o, $(OBJ))

%.o: %.S
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
%.o: ../lib/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel: kernel.ld $(OBJ) $(UBIN)
	$(LD) -o $@ $(KLDFLAGS) $(OBJ) $(GCC_LIB) -b binary $(UBIN)
	$(OBJDUMP) -S $@ > $@.asm

qemu: kernel
	$(QEMU) $(QEMUOPT)

qemu-gdb:
	$(QEMU) $(QEMUOPT) -S

clean:
	rm -f *.o *.d kernel kernel.asm
