#include "map.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PACKET_SIZE_LIMIT 1024 * 1024
#define MAP_LIST_LIMIT 1000

typedef enum {
    CLIENT_NEW_OK,
    CLIENT_NEW_SOCKET_CREATION,
    CLIENT_NEW_INVALID_IP,
    CLIENT_NEW_CONNECTION,
} client_new_result_t;

static const char *CLIENT_NEW_RESULT_STR[] = {
    NULL,
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
    NULL,
    "Sending to server failed.",
    "Reading from server failed.",
    "Message from server is too big.",
    "Decoding message from server failed.",
};

typedef enum {
    PROTO_LIST_WORLDS,
    PROTO_SAVE_WORLD,
    PROTO_LOAD_WORLD,
    PROTO_ERROR,
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

size_t request_encode(char **buffer, request_t in);

// all fields except world are owned, call response_free for cleanup
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
        struct {
            char *message;
        } error;
    } data;
} response_t;

bool response_decode(char *buffer, size_t len, response_t *out);
void response_free(response_t response);

static inline int write_all(int fd, size_t count, const char buf[count]) {
    size_t written = 0;
    while (written < count) {
        ssize_t n = write(fd, buf + written, count - written);
        if (n <= 0) {
            return -1;
        }
        written += n;
    }
    return 0;
}

static inline int read_all(int fd, size_t count, char buf[count]) {
    size_t red = 0;
    while (red < count) {
        ssize_t n = read(fd, buf + red, count - red);
        if (n <= 0) {
            return -1;
        }
        red += n;
    }
    return 0;
}

static inline bool is_valid_world_name_char(char a) {
    return (a >= 'a' && a <= 'z') || (a >= 'A' && a <= 'Z') ||
           (a >= '0' && a <= '9') || a == '_' || a == '-';
}
