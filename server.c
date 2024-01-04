#include "protocol.h"
#include "pthread.h"
#include <bits/pthreadtypes.h>
#include <dirent.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define REQUEST_SIZE_LIMIT 1024 * 1024

static pthread_mutex_t fs_mutex;

bool handle_list_worlds(char **buffer, size_t *len) {
    *len = 0;

    DIR *dir = opendir("worlds");
    if (dir == NULL) {
        return true;
    }

    size_t count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        *len += strlen(entry->d_name) + 4;
        count++;
    }

    closedir(dir);
    *buffer = realloc(*buffer, *len + 4);

    dir = opendir("worlds");
    if (dir == NULL) {
        return true;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        size_t len = strlen(entry->d_name);

        *((uint32_t *)(*buffer)) = htonl(len);
        *buffer += 4;
        memcpy(*buffer, entry->d_name, len);
        *buffer += strlen(entry->d_name) + 1;
    }

    return false;
}

bool handle_save_world(char **buffer, size_t *len) { return false; }

bool handle_load_world(char **buffer, size_t *len) { return false; }

bool handle_request(char **buffer, size_t *len) {
    if (*len < 1) {
        return true;
    }

    switch ((message_type_t)(*((*buffer)++))) {
    case PROTO_LIST_WORLDS:
        return handle_list_worlds(buffer, len);
    case PROTO_SAVE_WORLD:
        return handle_save_world(buffer, len);
    case PROTO_LOAD_WORLD:
        return handle_load_world(buffer, len);
    default:
        return true;
    }
}

void *handle_client(void *arg) {
    int socket = (int)(long)arg;
    char *buffer = malloc(0);

    for (;;) {
        char len_buffer[4];
        if (read(socket, len_buffer, 4) < 0) {
            printf("Error reading from socket.\n");
            break;
        }

        size_t len = ntohl(*(uint32_t *)len_buffer);
        if (len > REQUEST_SIZE_LIMIT) {
            printf("Request too large.\n");
            break;
        }
        buffer = realloc(buffer, len);

        if (read(socket, buffer, len) < 0) {
            printf("Error reading from socket.\n");
            break;
        }

        pthread_mutex_lock(&fs_mutex);
        bool res = handle_request(&buffer, &len);
        pthread_mutex_unlock(&fs_mutex);
        if (res) {
            printf("Error handling request.\n");
            break;
        }

        size_t send_len = htonl(len);
        if (write(socket, &send_len, 4) < 0) {
            printf("Error writing to socket.\n");
            break;
        }

        if (write(socket, buffer, len) < 0) {
            printf("Error writing to socket.\n");
            break;
        }
    }

    close(socket);

    return NULL;
}

int main(int n, char **args) {
    if (n != 2) {
        printf("Usage: %s <port>\n", args[0]);
        return 1;
    }

    int port = atoi(args[1]);

    if (port <= 0) {
        printf("Invalid port: %s\n", args[1]);
        return 1;
    }

    printf("Starting server on port %d\n", port);

    pthread_mutex_init(&fs_mutex, NULL);

    int server_fd;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int new_socket;
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                &addrlen)) >= 0) {
        pthread_t thread;
        pthread_create(&thread, NULL, &handle_client, (void *)(long)new_socket);
    }

    close(server_fd);
    return 0;
}
