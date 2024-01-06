#define _GNU_SOURCE
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
    printf("Internal server error: %s: %s\n", msg, strerror(errno));
    reported_error = "Internal server error";
}

size_t handle_list_worlds(char *out[]) {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        report_ise("Error opening directory");
        return -1;
    }

    size_t len = 4 + 1, count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char *ext = strchr(entry->d_name, '.');
        if (!ext || strcmp(ext, ".world") != 0) {
            continue;
        }

        len += (ext - entry->d_name) + 4;
        count++;
    }

    closedir(dir);
    *out = realloc(*out, len);

    dir = opendir(".");
    if (dir == NULL) {
        report_ise("Error opening directory");
        return -1;
    }

    char *cursor = *out;
    *(cursor++) = (char)(PROTO_LIST_WORLDS);
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

    closedir(dir);

    return len;
}

char *decode_world_name(size_t *len, const char *cursor[*len]) {
    if (*len < 4) {
        report_error("Error reading name length");
        return NULL;
    }

    size_t name_len = ntohl(*(uint32_t *)(*cursor));
    *cursor += 4;
    *len -= 4;

    if (*len < name_len) {
        report_error("Error reading name");
        return NULL;
    }

    for (size_t i = 0; i < name_len; i++) {
        printf("Invalid character in name: '%c'\n", cursor[0][i]);
        if (!is_valid_world_name_char(cursor[0][i])) {
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

size_t handle_save_world(size_t in_len, const char in[in_len], char *out[]) {
    char *name = decode_world_name(&in_len, &in);
    if (name == NULL) {
        return -1;
    }

    FILE *file = fopen(name, "w");
    free(name);
    if (file == NULL) {
        report_ise("Error opening file");
        return -1;
    }

    if (write_all(fileno(file), in_len, in) < 0) {
        fclose(file);
        report_ise("Error writing to file");
        return -1;
    }

    fclose(file);

    *out = realloc(*out, 1);
    **out = (char)(PROTO_SAVE_WORLD);

    return 1;
}

size_t handle_load_world(size_t in_len, const char in[in_len], char *out[]) {
    char *name = decode_world_name(&in_len, &in);
    if (name == NULL) {
        return -1;
    }

    FILE *file = fopen(name, "r");
    free(name);
    if (file == NULL) {
        report_error("World does not exist");
        return -1;
    }

    size_t len;
    if (fseek(file, 0, SEEK_END) < 0 || (len = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) < 0) {
        fclose(file);
        report_ise("Error reading file");
        return -1;
    }

    printf("Loading world of size %zu\n", len);

    *out = realloc(*out, len + 1);
    **out = (char)(PROTO_LOAD_WORLD);
    if (fread(*out + 1, sizeof(char), len, file) != len) {
        report_ise("Error reading file");
        fclose(file);
        return -1;
    }

    fclose(file);

    return len + 1;
}

size_t handle_request(size_t len_in, const char in[len_in], char *out[]) {
    if (len_in < 1) {
        return -1;
    }

    switch ((message_type_t)(*(in++))) {
    case PROTO_LIST_WORLDS:
        return handle_list_worlds(out);
    case PROTO_SAVE_WORLD:
        return handle_save_world(len_in - 1, in, out);
    case PROTO_LOAD_WORLD:
        return handle_load_world(len_in - 1, in, out);
    default:
        return -1;
    }
}

void *handle_client(void *arg) {
    int socket = (int)(long)arg;
    char *in_buffer = malloc(0), *out_buffer = malloc(0);

    for (;;) {
        char len_buffer[4];
        if (read_all(socket, 4, len_buffer) < 0) {
            printf("Error reading from socket.\n");
            break;
        }

        size_t len = ntohl(*(uint32_t *)len_buffer);

        printf("Received %zu bytes\n", len);

        if (len > REQUEST_SIZE_LIMIT) {
            printf("Request too large.\n");
            break;
        }
        in_buffer = realloc(in_buffer, len);

        if (read_all(socket, len, in_buffer) < 0) {
            printf("Error reading from socket.\n");
            break;
        }

        pthread_mutex_lock(&fs_mutex);
        len = handle_request(len, in_buffer, &out_buffer);
        pthread_mutex_unlock(&fs_mutex);
        if (len == -1) {
            len = 1 + 4 + strlen(reported_error);
            out_buffer = realloc(out_buffer, len);
            out_buffer[0] = (char)(PROTO_ERROR);
            *((uint32_t *)(out_buffer + 1)) = htonl(len - 5);
            memcpy(out_buffer + 5, reported_error, len - 5);
            reported_error = NULL;
        }

        size_t send_len = htonl(len);
        if (write_all(socket, 4, (char *)&send_len) < 0) {
            printf("Error writing packet length to socket.\n");
            break;
        }

        if (write_all(socket, len, out_buffer) < 0) {
            printf("Error writing to socket.\n");
            break;
        }
    }

    close(socket);
    free(in_buffer);
    free(out_buffer);

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
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket failed");
        return 1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        printf("setsockopt");
        return 1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("bind failed");
        return 1;
    }
    if (listen(server_fd, 3) < 0) {
        printf("listen failed");
        return 1;
    }

    int new_socket;
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                &addrlen)) >= 0) {
        pthread_t thread;
        pthread_create(&thread, NULL, &handle_client, (void *)(long)new_socket);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}
