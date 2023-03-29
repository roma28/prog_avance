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
#include <sys/stat.h>
#include <pthread.h>

#include "logger.h"


struct listener_thread {
    pthread_t thread_id;
    int thread_num;
    int port;
    char *ip;
    char *filename;
};

struct listener_thread *read_config(int *n) {
    char str_buff[256];
    FILE *cf = fopen("client_config", "r");

    if (!cf) {
        LOG_FATAL("No valid config file found");
        exit(-1);
    }

    fgets(str_buff, 255, cf);
    long n_servers = strtol(str_buff, NULL, 0);
    if (n_servers <= 0) {
        fclose(cf);
        LOG_FATAL("Error in config line: %s", str_buff);
        exit(-1);
    }
    LOG_INFO("%d listener threads will be employed", n_servers);
    *n = n_servers;

    struct listener_thread *config = malloc(n_servers * sizeof(struct listener_thread));
    for (int i = 0; i < n_servers; ++i) {
        config[i].thread_num = i;
        fgets(str_buff, 255, cf);
        LOG_TRACE("Parsing string %s", str_buff);
        config[i].ip = malloc(16 * sizeof(char));
        config[i].filename = malloc(255 * sizeof(char));
        int n_args = sscanf(str_buff, "%s %d %s", config[i].ip, &(config[i].port), config[i].filename);
        if (n_args != 3) {
            LOG_FATAL("Error in config line: %s only %d parameters of 3 found", str_buff, n);
            fclose(cf);
            exit(-1);
        }
        LOG_DEBUG("Thread %d will connect to %s:%d and will write to file %s", i, config[i].ip, config[i].port,
                  config[i].filename);
    }
    fclose(cf);
    return config;
}

int create_socket_thread(struct listener_thread param) {
    struct sockaddr_in server;

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_addr.s_addr = inet_addr(param.ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(param.port);

    if (connect(client_socket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        return -1;
    }

    return client_socket;
}

void *thread_function(void *param) {
    struct listener_thread *thread_param = (struct listener_thread *) param;
    LOG_INFO("Thread %d connecting to %s:%d", thread_param->thread_num, thread_param->ip, thread_param->port);

    int client_socket = create_socket_thread(*thread_param);
    if (client_socket < 0) {
        LOG_ERROR("Thread %d: connection failed: %s", thread_param->thread_num, strerror(errno));
        pthread_exit(NULL);
    }
    LOG_INFO("Thread %d: connected", thread_param->thread_num);

    struct stat filestats;

    while (1) {
        ssize_t received = recv(client_socket, &filestats, sizeof(struct stat), 0);
        if (received != sizeof(struct stat)) continue;
        if (filestats.st_size > 100000000) continue;
        LOG_INFO("Thread %d: file of size %d is to be received", thread_param->thread_num, filestats.st_size);

        char *fb = malloc(filestats.st_size);
        int fd = open(thread_param->filename, O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE | S_IEXEC);
        if (fd < 0) {
            LOG_FATAL("Thread %d: error opening file %s: %s", thread_param->thread_num, thread_param->filename,
                      strerror(errno));
            break;
        }
        off_t rcvd = recv(client_socket, fb, filestats.st_size, 0);

        write(fd, fb, filestats.st_size);
        LOG_INFO("Thread %d: %d bytes received out of %d", thread_param->thread_num, rcvd, filestats.st_size);
        close(fd);
    }
    return NULL;
}

void create_html(int n_images, struct listener_thread *cfg, int update_delay) {
    FILE *html = fopen("main.html", "w");

    if (!html) {
        LOG_FATAL("Error creating HTML file: %s", strerror(errno));
    }

    fprintf(html, "<!DOCTYPE html>\n"
                  "<html lang=\"en\">\n"
                  "<head>\n"
                  "    <meta charset=\"UTF-8\">\n"
                  "    <title>Title</title>\n"
                  "</head>\n"
                  "<body>\n");

    for (int i = 0; i < n_images; ++i) {
        fprintf(html, "<img src='%s' id='img%d'>\n", cfg[i].filename, i);
        fprintf(html, "<br>\n");
    }

    fprintf(html, "<script>\n"
                  "    setInterval(function () {\n"
                  "        document.querySelectorAll('img').forEach(function (i) {\n"
                  "            i.src = i.src + '';\n"
                  "        });\n"
                  "    }, %d);\n"
                  "    </script>\n", update_delay);

    fprintf(html, "</body>\n"
                  "</html>");

    fflush(html);
    fclose(html);
}


int main(int argc, char *argv[]) {
    logger_initConsoleLogger(stdout);
    logger_setLevel(LogLevel_INFO);

    int n_threads = 0;
    struct listener_thread *cfg = read_config(&n_threads);

    create_html(n_threads, cfg, 100);

    for (size_t i = 0; i < n_threads; ++i) {
        LOG_DEBUG("Creating thread %d", cfg[i].thread_num);
        if (pthread_create(&(cfg[i].thread_id), NULL, thread_function, &(cfg[i])) != 0) {
            LOG_FATAL("Creation thread %d failed: %s", i, strerror(errno));
        }
    }
    while (1) {
        // Idling
    }
}
