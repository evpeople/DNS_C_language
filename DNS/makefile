CC=gcc
CFLAGS=-O2

DNS.out: main.o lprintf.o log.o hlist.o
	gcc main.o lprintf.o log.o hlist.o -o DNS.out -lm

test: test.o lprintf.o log.o 
	gcc test.o lprintf.o log.o -o test -lm

clean:
	${RM} *.o datalink *.log *.out