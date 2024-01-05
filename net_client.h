#include "protocol.h"

typedef struct {
    int socket;
    char *buffer;
} client_t;

client_new_result_t client_new(client_t *out, char *host, uint16_t port);
client_send_result_t client_send(client_t *client, request_t message,
                                 response_t *out);
void client_free(client_t client);
