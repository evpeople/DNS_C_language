CC=gcc
CFLAGS=-O2

DNS.out: main.o lprintf.o log.o hlist.o
	gcc main.o lprintf.o log.o hlist.o -o DNS.out -lm

test : test.o lprintf.o log.o hlist.o
	gcc test.o lprintf.o log.o hlist.o -o test.out -lm



.PHONY:clean
clean:
		@echo "=======clean project========="
		rm   *.o
		rm	 *.out
		rm   *.log
		@echo "=======clean completed========="t