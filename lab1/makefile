CC=gcc
CFLAGS=-O2

datalink.out: SR.o protocol.o lprintf.o crc32.o
	gcc SR.o protocol.o lprintf.o crc32.o -o datalink.out -lm

clean:
	${RM} *.o datalink *.log

