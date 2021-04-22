CC=gcc
CFLAGS=-O2

datalink.out: datalink.o protocol.o lprintf.o crc32.o
	gcc datalink.o protocol.o lprintf.o crc32.o -o datalink.out -lm

clean:
	${RM} *.o datalink *.log

