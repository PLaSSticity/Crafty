#include "utils.h"
#include "nh_globals.h"

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

// #define BUFFER_SIZE 1024

FILE *DBG_LOG_log_fp; // extern
ts_s DBG_LOG_log_init_ts;

int make_named_socket(const char *filename) {
    struct sockaddr_un name;
    int sock;
    size_t size;

    /* Create the socket. */
    sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Bind a name to the socket. */
    name.sun_family = AF_LOCAL;
    strncpy(name.sun_path, filename, sizeof (name.sun_path));
    name.sun_path[sizeof (name.sun_path) - 1] = '\0';

    /* The size of the address is
     the offset of the start of the filename,
     plus its length (not including the terminating null byte).
     Alternatively you can just do:
     size = SUN_LEN (&name);
     */
    size = (offsetof(struct sockaddr_un, sun_path)
            + strlen(name.sun_path));

    int bind_res;

    bind_res = bind(sock, (struct sockaddr *) &name, size);

    if (bind_res < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    return sock;
}

int send_to_named_socket(int sockfd, const char *filename,
        void *buf, size_t size_buf) {
    struct sockaddr_un name;
    socklen_t size;
    int res;

    name.sun_family = AF_LOCAL;
    strncpy(name.sun_path, filename, sizeof (name.sun_path));
    name.sun_path[sizeof (name.sun_path) - 1] = '\0';
    size = (offsetof(struct sockaddr_un, sun_path)
            + strlen(name.sun_path));

    res = sendto(sockfd, buf, size_buf, 0, (struct sockaddr*) &name, size);

    return res;
}

int recv_from_named_socket(int sockfd, char *filename,
        size_t filename_size, void *buf, size_t size_buf) {
    struct sockaddr_un name;
    socklen_t size;
    int res;

    res = recvfrom(sockfd, buf, size_buf, 0, (struct sockaddr*) &name, &size);

    if (filename != NULL) {
        size_t name_size = offsetof(struct sockaddr_un, sun_path)
                + strlen(name.sun_path); // TODO: MIN(name_size, filename_size);
        strncpy(filename, name.sun_path, name_size);
    }

    return res;
}

int run_command(char *command, char *buffer_stdout, size_t size_buf) {
    FILE *fp;

    /* Open the command for reading. */
    fp = popen(command, "r");
    if (fp == NULL) {
        return -1;
    }

    /* Read the output a line at a time - output it. */
    fread(buffer_stdout, sizeof (char), size_buf, fp);

    /* close */
    pclose(fp);

    return 0;
}

void launch_thread_at(int core, void*(*callback)(void*)) {
    pthread_t pthr;
    pthread_attr_t attr;
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    pthread_attr_init(&attr);
    pthread_attr_setaffinity_np(&attr, sizeof (cpu_set_t), &cpuset);

    pthread_create(&(pthr), &attr, callback, NULL);
}

void set_affinity_at(int core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    sched_setaffinity(0, sizeof (cpu_set_t), &cpuset);
}
