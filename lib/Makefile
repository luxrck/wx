include ../Makefile.header

CFLAGS += -I../include -I..
 
SRC := assert.c	\
	brk.c		\
	close.c		\
	dir.c		\
	dup.c		\
	errno.c		\
	exit.c		\
	fork.c		\
	libc.c		\
	link.c		\
	malloc.c	\
	open.c		\
	pipe.c		\
	fprintf.c	\
	printfmt.c	\
	read.c		\
	stat.c		\
	time.c		\
	wait.c		\
	write.c		\
	string.c

OBJ := $(patsubst %.c, %.o, $(SRC))
OBJ := $(patsubst %.S, %.o, $(OBJ))

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

libc.a: $(OBJ) entry.o
	$(AR) r $@ $(OBJ)

entry.o: entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.d *.a