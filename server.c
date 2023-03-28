#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "logger.h"

int create_socket(int port) {
    struct sockaddr_in server_addr, client_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        LOG_ERROR("Socket creation failed: %s\n", strerror(errno));
        return -1;
    }

    bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    listen(server_socket, 3);

    return server_socket;
}

char *check_file(char *filename, struct stat *last_checked) {
    int f = open(filename, O_RDONLY);
    char *file_buff = NULL;

    struct stat stat;

    if (fstat(f, &stat) != 0) {
        LOG_ERROR("Error getting filestats: %s\n", strerror(errno));
        close(f);
        return NULL;
    }
    if (stat.st_size == 0) {
        close(f);
        return NULL;
    }

    if (stat.st_mtime != last_checked->st_mtime) {
        file_buff = mmap(NULL, stat.st_size, PROT_READ, MAP_PRIVATE, f, 0);
        if (file_buff == MAP_FAILED) {
            LOG_ERROR("Mapping file %s failed: %s\n", filename, strerror(errno));
            file_buff = NULL;
        }
    }

    close(f);
    memcpy(last_checked, &stat, sizeof(struct stat));
    return file_buff;
}

int sendall(int s, char *buf, off_t *len) {
    off_t total = 0;
    off_t bytesleft = *len;
    int n;

    while (total < *len) {
        n = send(s, buf + total, bytesleft, 0);
        if (n == -1) {
            LOG_ERROR("Error while sending: %s", strerror(errno));
            break;
        }
        total += n;
        bytesleft -= n;
    }

    *len = total;

    return n;
}

int send_file(int socket, char *fbuf, struct stat fs) {
    off_t btw = fs.st_size;
    off_t btw_fs = sizeof(struct stat);
    if (sendall(socket, &fs, &btw_fs) != 0) {
        LOG_ERROR("Error while sending filestats, only %d bytes sent of %d", btw_fs, sizeof(struct stat));
        return -1;
    }
    if (sendall(socket, fbuf, &btw) != 0) {
        LOG_ERROR("Error while sending file, only %d bytes sent of %d", btw, fs.st_size);
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {

    logger_initConsoleLogger(stdout);
    logger_setLevel(LogLevel_INFO);

    if (argc != 3) {
        LOG_FATAL("The program takes 2 argument, but %d given\nUsage: %s [port] [file]\n", argc - 1, argv[0]);
        return -1;
    }

    int port;
    port = strtol(argv[1], NULL, 0);
    if (port > 65535 || port <= 0) {
        LOG_FATAL("Incorrect port value: %d", port);
        return -1;
    }

    int server_socket = create_socket(port);

    int client_socket;
    LOG_INFO("Waiting for connection on port %d\n", port);

    client_socket = accept(server_socket, NULL, NULL);
    if (client_socket < 0) {
        LOG_ERROR("Connection failed: %s\n", strerror(errno));
        return -1;
    }
    LOG_INFO("Connection received\n");

    struct stat lc;
    while (1) {
        char *filebuff = check_file(argv[2], &lc);
        if (filebuff) {
            LOG_DEBUG("File of size %d is mapped", lc.st_size);
            send_file(client_socket, filebuff, lc);
            if (munmap(filebuff, lc.st_size) != 0) {
                LOG_ERROR("Unmapping failed: %s\n", strerror(errno));
                return -1;
            }
        }
    }
}
