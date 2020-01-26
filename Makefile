CC=gcc
LD=gcc

CFLAGS=-std=c11 -Wall -Werror -Wpedantic -Iinclude -O3
OBJFILES=rz.o zheaders.o znumbers.o zserial.o xmodemcrc.o

all: rz

%.o: %.c
	$(CC) $(CFLAGS) -DDEBUG -c -o $@ $<

rz: $(OBJFILES)
	$(LD) $^ -o $@

test: rz.c zheaders.c znumbers.c zserial.c xmodemcrc.c
	$(CC) $(CFLAGS) -DTEST -o $@ $^
	./$@

clean:
	rm -f *.o rz

