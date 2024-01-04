#include "map.h"
#include <stdlib.h>

typedef enum {
    CLIENT_NEW_OK,
    CLIENT_NEW_SOCKET_CREATION,
    CLIENT_NEW_INVALID_IP,
    CLIENT_NEW_CONNECTION,
} client_new_result_t;

static const char *CLIENT_NEW_RESULT_STR[] = {
    "OK",
    "Socket creation failed.",
    "Invalid IP address.",
    "Connection to server failed.",
};

typedef enum {
    CLIENT_SEND_OK,
    CLIENT_SEND_SEND,
    CLIENT_SEND_READ,
    CLIENT_SEND_TOO_BIG,
    CLIENT_SEND_DECODE,
} client_send_result_t;

static const char *CLIENT_SEND_RESULT_STR[] = {
    "OK",
    "Sending to server failed.",
    "Reading from server failed.",
    "Message from server is too big.",
    "Decoding message from server failed.",
};

typedef enum {
    PROTO_LIST_WORLDS,
    PROTO_SAVE_WORLD,
    PROTO_LOAD_WORLD
} message_type_t;

// request does not own any fields, its responsibility of the creator to free
// them
typedef struct {
    message_type_t type;
    union {
        struct {
        } list_worlds;
        struct {
            char *name;
            map_t *world;
        } save_world;
        struct {
            char *name;
        } load_world;
    } data;
} request_t;

// all fields are owned, call response_free for cleanup
typedef struct {
    message_type_t type;
    union {
        struct {
            size_t count;
            char **names;
        } list_worlds;
        struct {
        } save_world;
        struct {
            map_t world;
        } load_world;
    } data;
} response_t;

void response_free(response_t response);

typedef struct {
    int socket;
    char *buffer;
} client_t;

client_new_result_t client_new(client_t *out, char *host, uint16_t port);
client_send_result_t client_send(client_t *client, request_t message,
                                 response_t *out);
void client_free(client_t client);
