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
#include <errno.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "logger.h"


int recvall(int s, char *buf, off_t *len) {
    int total = 0;
    int bytesleft = *len;
    int n;

    while (total < *len) {
        n = recv(s, buf + total, bytesleft, 0);
        if (n == -1) {
            LOG_ERROR("Error while receiving: %s", strerror(errno));
            break;
        }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}


int main(int argc, char *argv[]) {

    logger_initConsoleLogger(stdout);
    logger_setLevel(LogLevel_DEBUG);

    struct sockaddr_in server;

//    char *ip = malloc(20 * sizeof(char));
    if (argc != 4) {
        LOG_FATAL("The program takes 2 argument, but %d given\nUsage: %s [ip] [port] [filename]\n", argc - 1, argv[0]);
        return -1;
    }

    int port;
    sscanf(argv[2], "%d", &port);

    LOG_INFO("Connecting to %s on port %d", argv[1], port);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(client_socket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        LOG_ERROR("Connection failed: %s", strerror(errno));
        return -1;
    }
    LOG_INFO("Connected");

    struct stat s;
    recv(client_socket, &s, sizeof(struct stat), 0);
    LOG_INFO("File of size %d is to be received", s.st_size);


    int fd = open(argv[3], O_WRONLY);
    char *fp = mmap(NULL, s.st_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (fp != MAP_FAILED) {
        close(fd);
        off_t to_receive = s.st_size;
        recvall(client_socket, fp, &to_receive);
        munmap(fp, s.st_size);
    } else {
        LOG_ERROR("Mapping failed: %s", strerror(errno));
    }

}
