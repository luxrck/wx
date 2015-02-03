include ../Makefile.header

CFLAGS += -I../include -I..
 
SRC := string.c	\
	syscall.c

OBJ := $(patsubst %.c, %.o, $(SRC))
OBJ := $(patsubst %.S, %.o, $(OBJ))

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

libc.a: $(OBJ)
	$(AR) r $@ $(OBJ)

entry.o: entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm *.o *.d *.a