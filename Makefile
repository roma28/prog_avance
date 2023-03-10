all: server client

server: server.o
	cc server.o -o server

client: client.o
	cc client.o -o client

cleint.o: client.c
	cc -c client.c -o client.o

server.o: server.c
	cc -c server.c -o server.o
