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
static __thread char const *reported_error = NULL;

void report_error(char const *msg) {
    printf("Error: %s\n", msg);
    reported_error = msg;
}
void report_ise(char const *msg) {
    printf("Internal server error: %s\n", msg);
    reported_error = "Internal server error";
}

bool handle_list_worlds(char **buffer, size_t *len) {
    *len = 0;

    DIR *dir = opendir(".");
    if (dir == NULL) {
        report_ise("Error opening directory");
        return true;
    }

    size_t count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char *ext = strchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".world") != 0) {
            continue;
        }

        *len += (ext - entry->d_name) + 4;
        count++;
    }

    *len += 4;

    closedir(dir);
    *buffer = realloc(*buffer, *len);

    dir = opendir(".");
    if (dir == NULL) {
        report_ise("Error opening directory");
        return true;
    }

    char *cursor = *buffer;
    *((uint32_t *)(cursor)) = htonl(count);
    cursor += 4;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char *ext = strchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".world") != 0) {
            continue;
        }
        size_t len = ext - entry->d_name;

        *((uint32_t *)(cursor)) = htonl(len);
        cursor += 4;
        memcpy(cursor, entry->d_name, len);
        cursor += len;
    }

    return false;
}

char *decode_world_name(char **cursor, size_t *len) {
    if (*len < 4) {
        report_error("Error reading name length");
        return NULL;
    }

    size_t name_len = ntohl(*(uint32_t *)(cursor));
    *cursor += 4;
    *len -= 4;

    if (*len < name_len) {
        report_error("Error reading name");
        return NULL;
    }

    for (size_t i = 0; i < name_len; i++) {
        if (!is_valid_world_name_char(cursor[0][i])) {
            free(cursor);
            report_error("Invalid character in name");
            return NULL;
        }
    }

    size_t world_len = strlen(".world");
    char *name = malloc(name_len + world_len + 1);
    memcpy(name, *cursor, name_len);
    memcpy(name + name_len, ".world", world_len + 1);
    *cursor += name_len;
    *len -= name_len;

    return name;
}

bool handle_save_world(char **buffer, size_t *len) {
    char *cursor = *buffer;
    char *name = decode_world_name(&cursor, len);

    FILE *file = fopen(name, "w");
    free(name);
    if (file == NULL) {
        report_ise("Error opening file");
        return true;
    }

    if (write_all(fileno(file), cursor, *len) < 0) {
        fclose(file);
        report_ise("Error writing to file");
        return true;
    }

    fclose(file);
    *len = 0;

    return false;
}

bool handle_load_world(char **buffer, size_t *len) {
    char *cursor = *buffer;
    char *name = decode_world_name(&cursor, len);

    FILE *file = fopen(name, "r");
    free(name);
    if (file == NULL) {
        report_error("World does not exist");
        return true;
    }

    if (fseek(file, 0, SEEK_END) < 0 || (*len = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) < 0) {
        fclose(file);
        report_ise("Error reading file");
        return true;
    }

    *buffer = realloc(*buffer, *len);
    if (read_all(fileno(file), *buffer, *len) < 0) {
        fclose(file);
        report_ise("Error reading file");
        return true;
    }

    fclose(file);

    return false;
}

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
        if (read_all(socket, len_buffer, 4) < 0) {
            printf("Error reading from socket.\n");
            break;
        }

        size_t len = ntohl(*(uint32_t *)len_buffer);
        if (len > REQUEST_SIZE_LIMIT) {
            printf("Request too large.\n");
            break;
        }
        buffer = realloc(buffer, len);

        if (read_all(socket, buffer, len) < 0) {
            printf("Error reading from socket.\n");
            break;
        }

        pthread_mutex_lock(&fs_mutex);
        bool res = handle_request(&buffer, &len);
        pthread_mutex_unlock(&fs_mutex);
        if (res) {
            printf("Error handling request.\n");
            len = strlen(reported_error) + 1;
            buffer = realloc(buffer, len);
            memcpy(buffer, reported_error, len);
            reported_error = NULL;
        }

        size_t send_len = htonl(len);
        if (write_all(socket, (char *)&send_len, 4) < 0) {
            printf("Error writing to socket.\n");
            break;
        }

        if (write_all(socket, buffer, len) < 0) {
            printf("Error writing to socket.\n");
            break;
        }
    }

    close(socket);
    free(buffer);

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
