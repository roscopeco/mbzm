CC=gcc
LD=gcc

CFLAGS=-std=c11 -Wall -Werror -Wpedantic -Iinclude -O3
OBJFILES=rz.o zheaders.o znumbers.o zserial.o crcccitt.o crc32.o

all: rz

%.o: %.c
	$(CC) $(CFLAGS) -DZDEBUG -c -o $@ $<

rz: $(OBJFILES)
	$(LD) $^ -o $@

test: tests.c zheaders.c znumbers.c zserial.c crcccitt.c crc32.c
	$(CC) $(CFLAGS) -DTEST -o $@ $^
	./$@

clean:
	rm -f *.o rz test

