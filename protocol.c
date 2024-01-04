#include "protocol.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PACKET_SIZE_LIMIT 1024 * 1024
#define MAP_LIST_LIMIT 1000

void response_free(response_t response) {
    switch (response.type) {
    case PROTO_LIST_WORLDS:
        if (response.data.list_worlds.names == NULL) {
            return;
        }

        for (size_t i = 0; i < response.data.list_worlds.count; i++) {
            free(response.data.list_worlds.names[i]);
        }
        free(response.data.list_worlds.names);
        response.data.list_worlds.names = NULL;
        response.data.list_worlds.count = 0;

        break;
    case PROTO_SAVE_WORLD:
        // no-op
        break;
    case PROTO_LOAD_WORLD:
        // supposed to be moved
        break;
    case PROTO_ERROR:
        if (response.data.error.message == NULL) {
            return;
        }

        free(response.data.error.message);
    }
}

client_new_result_t client_new(client_t *out, char *host, uint16_t post) {
    out->buffer = malloc(0);

    if ((out->socket = socket(AF_INET, SOCK_STREAM, 0) < 0)) {
        return CLIENT_NEW_SOCKET_CREATION;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(post);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        return CLIENT_NEW_INVALID_IP;
    }

    if (connect(out->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return CLIENT_NEW_CONNECTION;
    }

    return CLIENT_NEW_OK;
}

size_t encode_request_len(request_t in) {
    switch (in.type) {
    case PROTO_LIST_WORLDS:
        return 1;
    case PROTO_SAVE_WORLD:
        return 1 + 4 + strlen(in.data.save_world.name) + 4 + 4 +
               map_alloc_size(in.data.save_world.world->width,
                              in.data.save_world.world->height);
    case PROTO_LOAD_WORLD:
        return 1 + 4 + strlen(in.data.load_world.name);
    default:
        return 0;
    }
}

void encode_len(char **buffer, size_t len) {
    *(uint32_t *)*buffer = htonl(len);
    *buffer += 4;
}
void encode_string(char **buffer, char *in) {
    size_t len = strlen(in);
    encode_len(buffer, len);
    memcpy(*buffer, in, len);
    *buffer += len;
}

size_t encode_request(char **buffer, request_t in) {
    size_t buffer_size = encode_request_len(in) + 4;
    *buffer = realloc(*buffer, buffer_size);

    char *cursor = *buffer;
    encode_len(&cursor, buffer_size - 4);

    *((cursor)++) = (char)(in.type);
    switch (in.type) {
    case PROTO_LIST_WORLDS:
        break;
    case PROTO_SAVE_WORLD:
        encode_string(&cursor, in.data.save_world.name);
        encode_len(&cursor, in.data.save_world.world->width);
        encode_len(&cursor, in.data.save_world.world->height);
        memcpy(cursor, in.data.save_world.world->data,
               map_alloc_size(in.data.save_world.world->width,
                              in.data.save_world.world->height));
        break;
    case PROTO_LOAD_WORLD:
        encode_string(buffer, in.data.load_world.name);
        break;
    default:
        printf("Invalid request type: %d\n", in.type);
        exit(1);
    }

    return buffer_size;
}

size_t decode_len(char **buffer, size_t *len) {
    if (*len < 4) {
        return -1;
    }

    size_t decoded_len = ntohl(*(uint32_t *)*buffer);
    *buffer += 4;
    *len -= 4;

    return decoded_len;
}

char *decode_string(char **buffer, size_t *len) {
    size_t decoded_len = decode_len(buffer, len);

    if (*len < decoded_len) {
        return NULL;
    }

    char *decoded_string = malloc(decoded_len + 1);
    memcpy(decoded_string, *buffer, decoded_len);
    decoded_string[decoded_len] = '\0';
    *buffer += decoded_len;
    *len -= decoded_len;

    return decoded_string;
}

bool decode_list_worlds(char *buffer, size_t len, response_t *out) {
    out->data.list_worlds.count = decode_len(&buffer, &len);
    if (out->data.list_worlds.count > MAP_LIST_LIMIT) {
        return true;
    }

    out->data.list_worlds.names =
        malloc(sizeof(char *) * out->data.list_worlds.count);

    for (size_t i = 0; i < out->data.list_worlds.count; i++) {
        out->data.list_worlds.names[i] = decode_string(&buffer, &len);
        if (out->data.list_worlds.names[i] == NULL) {
            return true;
        }
    }

    return false;
}

bool decode_world_load(char *buffer, size_t len, map_t *out) {
    out->width = decode_len(&buffer, &len);
    out->height = decode_len(&buffer, &len);

    size_t map_size = map_alloc_size(out->width, out->height);
    if (len < map_size) {
        return true;
    }

    out->data = malloc(map_size);
    memcpy(out->data, buffer, map_size);

    return false;
}

bool decode_response(char *buffer, size_t len, response_t *out) {
    if (len < 1) {
        return true;
    }

    out->type = *(buffer++);
    len--;

    switch (out->type) {
    case PROTO_LIST_WORLDS:
        return decode_list_worlds(buffer, len, out);
    case PROTO_SAVE_WORLD:
        return false;
    case PROTO_LOAD_WORLD:
        return decode_world_load(buffer, len, &out->data.load_world.world);
    case PROTO_ERROR:
        out->data.error.message = decode_string(&buffer, &len);
        if (out->data.error.message == NULL) {
            return true;
        }
        return false;
    default:
        return true;
    }
}

client_send_result_t client_send(client_t *client, request_t message,
                                 response_t *out) {
    size_t len = encode_request(&client->buffer, message);

    if (write_all(client->socket, client->buffer, len) < 0) {
        return CLIENT_SEND_SEND;
    }

    char len_buffer[4];
    if (read_all(client->socket, len_buffer, 4) < 0) {
        return CLIENT_SEND_READ;
    }

    len = ntohl(*(uint32_t *)len_buffer);
    if (len > PACKET_SIZE_LIMIT) {
        return CLIENT_SEND_TOO_BIG;
    }

    client->buffer = realloc(client->buffer, len);
    if (read_all(client->socket, client->buffer, len) < 0) {
        return CLIENT_SEND_READ;
    }

    if (decode_response(client->buffer, len, out)) {
        return CLIENT_SEND_DECODE;
    }

    return CLIENT_SEND_OK;
}

void client_free(client_t client) {
    if (client.socket == -1) {
        return;
    }

    close(client.socket);
    client.socket = -1;
    free(client.buffer);
    client.buffer = NULL;
}
