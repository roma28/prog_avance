#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>


int create_socket(int port)
{
  socklen_t c = (socklen_t) sizeof(struct sockaddr_in);

  struct sockaddr_in server_addr, client_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket<0)
  {
    printf("Socket creation failed\n");
    return -1;
  }

  bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr));
  listen(server_socket, 3);

  return server_socket;
}

char* check_file(char* filename, struct stat* last_checked)
{
  int f = open(filename, O_RDONLY);
  char* file_buff = NULL;

  struct stat stat;

  if(fstat(f, &stat) != 0)
  {
    printf("Error getting filestats: %s\n", strerror(errno));
    return NULL;
  }

  if(stat.st_mtime != last_checked->st_mtime)
  {
    file_buff = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, f, 0);
    if(file_buff == MAP_FAILED) file_buff = NULL;
  }

  close(f);
  memcpy(last_checked, &stat, sizeof(struct stat));
  return file_buff;
}

int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) {
          printf("Error while sending: %s", strerror(errno));
          break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

int main(int argc, char* argv[])
{

  if(argc != 3)
  {
    printf("The program takes 2 argument, but %d given\nUsage: %s [port] [file]\n", argc-1, argv[0]);
    return -1;
  }

  int port;
  sscanf(argv[1],"%d", &port);

  // wait_for_connection:
  int server_socket = create_socket(port);

  int client_socket;
  printf("Waiting for connection on port %d\n", port);



  client_socket = accept(server_socket, NULL, NULL);
  if(client_socket<0)
  {
    printf("Connection failed: %s\n", strerror(errno));
    return -1;
  }
  printf("Connection succes\n");

  struct stat lc;
  while(1)
  {

    char* filebuff = check_file(argv[2], &lc);
    if(filebuff)
    {
      int btw = lc.st_size-1;

      int n = sendall(client_socket, filebuff, &btw);
      if(n == -1)
      {
        printf("Error while sending, only %d bytes are sent\n%s", btw, strerror(errno));
        // break;
      }

      if(munmap(filebuff, lc.st_size)!=0)
      {
        printf("Filemapping failed: %s\n", strerror(errno));
      }
    }

  }





}
