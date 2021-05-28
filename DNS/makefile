CC=gcc
CFLAGS=-O2

DNS.out: main.o lprintf.o log.o
	gcc main.o lprintf.o log.o -o DNS.out -lm

clean:
	${RM} *.o datalink *.log *.out