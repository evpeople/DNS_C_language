all:testHear testSend


testHear:testHear.o
	gcc testHear.o -o testHear
testHear.o:testHear.c
	gcc -c testHear.c

testSend:testSend.o
	gcc testSend.o -o testSend
testSend.o:testSend.c
	gcc -c testSend.c


.PHONY:clean
clean:
	@echo "=======clean project========="
	rm   *.o 
	@echo "=======clean completed========="

	