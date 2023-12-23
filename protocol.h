#include "simulation.h"

#define DOWNLOAD_WORLD 0
#define UPLOAD_WORLD 1

typedef struct {
    char *data;
    int len;
} bytes_t;

typedef struct {
    bytes_t name;
} download_world_req_t;

typedef struct {
    bytes_t name;
    map_t map;
} upload_world_req_t;

typedef struct {
    char flag;
    union {
        download_world_req_t download_world;
        upload_world_req_t upload_world;
    } data;
} request_t;

