CC=gcc
CFLAGS=-O2

DNS.out: main.o lprintf.o log.o hlist.o
	gcc main.o lprintf.o log.o hlist.o -o DNS.out -lm

test.out:  lprintf.o log.o hlist.o test.o
	gcc  lprintf.o log.o hlist.o test.o -o test.out -lm

clean:
	${RM} *.o datalink *.log *.out