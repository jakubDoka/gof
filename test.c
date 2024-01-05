#include "net_client.h"
#include <assert.h>
#include <stdio.h>

response_t run_test(client_t *client, request_t request,
                    message_type_t expected) {
    response_t response;
    client_send_result_t result = client_send(client, request, &response);
    if (result) {
        printf("Error sending request: %s\n", CLIENT_SEND_RESULT_STR[result]);
        assert(false);
    }
    if (response.type != expected) {
        printf("Expected response type %d, got %d\n", expected, response.type);
        assert(false);
    }
    return response;
}

int main(int n, char **args) {
    client_t client;
    client_new_result_t result = client_new(&client, "127.0.0.1", 8080);
    if (result) {
        printf("Error creating client: %s\n", CLIENT_NEW_RESULT_STR[result]);
        return 0;
    }

    map_t m = map_new(10, 10);
    map_clear(m);

    for (size_t i = 0; i < 10; i++) {
        map_set(m, i, i, true);
    }

    request_t request = {
        .type = PROTO_SAVE_WORLD,
        .data.save_world =
            {
                .name = "test",
                .world = &m,
            },
    };
    response_t response = run_test(&client, request, PROTO_SAVE_WORLD);
    response_free(response);

    request = (request_t){
        .type = PROTO_LOAD_WORLD,
        .data.load_world =
            {
                .name = "test",
            },
    };
    response = run_test(&client, request, PROTO_LOAD_WORLD);

    assert(response.data.load_world.world.width == 10);
    assert(response.data.load_world.world.height == 10);
    for (size_t i = 0; i < 10; i++) {
        for (size_t j = 0; j < 10; j++) {
            assert(map_get(response.data.load_world.world, i, j) ==
                   map_get(m, i, j));
        }
    }
    response_free(response);
    map_free(response.data.load_world.world);

    request = (request_t){
        .type = PROTO_LIST_WORLDS,
    };
    response = run_test(&client, request, PROTO_LIST_WORLDS);

    assert(response.data.list_worlds.count == 1);
    assert(strcmp(response.data.list_worlds.names[0], "test") == 0);
    response_free(response);

    map_free(m);
    client_free(client);

    printf("Test passed\n");

    return 0;
}
