#include "net_client.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

client_new_result_t client_new(client_t *out, char *host, uint16_t port) {
    if ((out->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return CLIENT_NEW_SOCKET_CREATION;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(out->socket);
        return CLIENT_NEW_INVALID_IP;
    }

    int status = connect(out->socket, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0) {
        close(out->socket);
        return CLIENT_NEW_CONNECTION;
    }

    out->buffer = malloc(0);
    return CLIENT_NEW_OK;
}

client_send_result_t client_send(client_t *client, request_t message,
                                 response_t *out) {
    size_t len = request_encode(&client->buffer, message);

    if (write_all(client->socket, len, client->buffer) < 0) {
        return CLIENT_SEND_SEND;
    }

    printf("Sent %zu bytes\n", len);

    char len_buffer[4];
    if (read_all(client->socket, 4, len_buffer) < 0) {
        return CLIENT_SEND_READ;
    }

    len = ntohl(*(uint32_t *)len_buffer);
    if (len > PACKET_SIZE_LIMIT) {
        return CLIENT_SEND_TOO_BIG;
    }

    client->buffer = realloc(client->buffer, len);
    if (read_all(client->socket, len, client->buffer) < 0) {
        return CLIENT_SEND_READ;
    }

    if (response_decode(client->buffer, len, out)) {
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
