all:server client


server:server.o
	gcc server.o -o server
server.o:server.c
	gcc -c server.c

client:client.o
	gcc client.o -o client
client.o:client.c
	gcc -c client.c


.PHONY:clean
clean:
	@echo "=======clean project========="
	rm   *.o 
	@echo "=======clean completed========="

	