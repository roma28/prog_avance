cc = cc -g -Wall

all: server client

server: server.o logger.o
	cc server.o logger.o -o server

client: client.o logger.o
	cc client.o logger.o -o client

cleint.o: client.c
	cc -c client.c -o client.o

server.o: server.c
	cc -c server.c -o server.o

logger.o:
	cc -c logger.c -o logger.o

clean:
	rm -rf *.o