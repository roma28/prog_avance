#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>


int main(int argc, char *argv[])
{

  struct sockaddr_in server;

  char* ip = malloc(20*sizeof(char));

  if(argc != 3)
  {
    printf("The program takes 2 argument, but %d given\nUsage: %s [ip] [port]\n", argc-1, argv[0]);
    return -1;
  }

  int port = 8080;
  // sscanf(argv[1],"%d", &port);


  int client_socket = socket(AF_INET, SOCK_STREAM, 0);

  server.sin_addr.s_addr  = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(8080);

  if(connect(client_socket, (struct sockaddr*) &server, sizeof(server)) < 0)
  {
    printf("Connection failed\n");
    return -1;
  }
  printf("Connected\n");

  char c;
  while(recv(client_socket, &c, 1, 0))
  {
    printf("%c", c);
  };
}
