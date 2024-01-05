#include "ncurses.h"
#include "net_client.h"
#include "simulation.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_WIDTH 100
#define MAX_HEIGHT 50
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

void map_draw(map_t *m) {
    for (size_t y = 0; y < m->height; y++) {
        for (size_t x = 0; x < m->width; x++) {
            char c = map_get(m, x, y) ? '#' : '.';
            printw("%c", c);
        }
        printw("\n");
    }
}

void sim_draw(sim_t *s) {
    map_t m = s->maps[s->index];
    map_draw(&m);
}

void confirm_loop() {
    int ch = '\0';
    while (ch != '\n') {
        ch = getch();
    }
}

map_t map_creation(size_t width, size_t height) {
    char input[3];

    while (width == 0 || width >= MAX_WIDTH) {
        printw("Set map width: ");
        wgetnstr(stdscr, input, 2);
        width = atoi(input) > 0 ? atoi(input) : 0;
        clear();
    }

    while (height == 0 || height >= MAX_HEIGHT) {
        printw("Set map height: ");
        wgetnstr(stdscr, input, 2);
        height = atoi(input) > 0 ? atoi(input) : 0;
        clear();
    }

    return map_new(width, height);
}

void map_server_send(map_t *m) {
    clear();
    printw("Send to server as: ");
    refresh();
    char name_w[100];
    getstr(name_w);

    request_t r = {PROTO_SAVE_WORLD, {.save_world = {name_w, m}}};
    client_t c;
    client_new_result_t result_new = client_new(&c, SERVER_IP, SERVER_PORT);

    if (result_new != CLIENT_NEW_OK) {
        clear();
        printw("%s", CLIENT_NEW_RESULT_STR[result_new]);
        refresh();
    }

    response_t res;
    client_send_result_t result_send = client_send(&c, r, &res);

    clear();

    if (result_send != CLIENT_SEND_OK) {
        printw("%s\n", CLIENT_SEND_RESULT_STR[result_send]);
    }
    if (res.type == PROTO_ERROR) {
        printw("%s\n", res.data.error.message);
    }

    refresh();

    response_free(res);
    client_free(c);

    confirm_loop();
}

map_t map_server_get(map_t *m) {
    client_t c;
    client_new_result_t result_new = client_new(&c, SERVER_IP, SERVER_PORT);
    response_t res;

    clear();
    if (result_new != CLIENT_NEW_OK) {
        printw("%s", CLIENT_NEW_RESULT_STR[result_new]);
        refresh();
        confirm_loop();
        return *m;
    }

    request_t r = {PROTO_LIST_WORLDS, {.list_worlds = {}}};
    client_send_result_t result_send = client_send(&c, r, &res);
    if (result_send != CLIENT_SEND_OK) {
        printw("%s", CLIENT_SEND_RESULT_STR[result_send]);
        refresh();
        confirm_loop();
        return *m;
    }

    for (size_t i = 0; i < res.data.list_worlds.count; i++) {
        printw("%s\n", res.data.list_worlds.names[i]);
    }

    int ch = '\0';
    size_t y = 0;
    while (ch != '\n') {
        move(y, 0);
        refresh();
        ch = getch();

        switch (ch) {
        case 'w':
        case 'W':
        case KEY_UP:
            y -= y != 0;
            break;
        case 's':
        case 'S':
        case KEY_DOWN:
            y += y != res.data.list_worlds.count - 1;
            break;
        }
    }

    r = (request_t){PROTO_LOAD_WORLD,
                    {.load_world = {res.data.list_worlds.names[y]}}};
    result_send = client_send(&c, r, &res);
    response_free(res);

    clear();
    if (result_send != CLIENT_SEND_OK) {
        printw("%s", CLIENT_SEND_RESULT_STR[result_send]);
        refresh();
        confirm_loop();
        return *m;
    }

    map_free(*m);
    *m = res.data.load_world.world;

    response_free(res);
    client_free(c);
    return *m;
}

map_t map_manipulation(map_t *m) {
    int ch = '\0';
    size_t x = 0;
    size_t y = 0;

    while (ch != '\n') {
        clear();
        map_draw(m);
        move(y, x);
        refresh();
        ch = getch();

        switch (ch) {
        case 'q':
        case 'Q':
            map_free(*m);
            endwin();
            exit(0);
        case 'r':
        case 'R':
            map_gen_rnd(m);
            break;
        case 'c':
        case 'C':
            map_clear(m);
            break;
        case 'f':
        case 'F':
            map_server_send(m);
            break;
        case 'g':
        case 'G':
            *m = map_server_get(m);
            break;
        case 'u':
        case 'U':
            clear();
            printw("Save as: ");
            refresh();
            char name_u[100];
            getstr(name_u);
            map_save(m, name_u);
            break;
        case 'l':
        case 'L':
            clear();
            printw("Load from: ");
            refresh();
            char name_r[100];
            getstr(name_r);
            map_t new_m = map_load(name_r);
            if (new_m.width >= MAX_WIDTH || new_m.height >= MAX_HEIGHT) {
                map_free(new_m);
            } else if (memcmp(&new_m, &MAP_NULL, sizeof(map_t)) == 0) {
            } else {
                map_free(*m);
                *m = new_m;
                x = 0;
                y = 0;
            }
            break;
        case 'w':
        case 'W':
        case KEY_UP:
            y -= y == 0 ? 0 : 1;
            break;
        case 'a':
        case 'A':
        case KEY_LEFT:
            x -= x == 0 ? 0 : 1;
            break;
        case 's':
        case 'S':
        case KEY_DOWN:
            y += y == m->height - 1 ? 0 : 1;
            break;
        case 'd':
        case 'D':
        case KEY_RIGHT:
            x += x == m->width - 1 ? 0 : 1;
            break;
        case ' ':
            map_set(m, x, y, !map_get(m, x, y));
            break;
        }
    }
    clear();
    return *m;
}

sim_t sim_creation(map_t m, size_t ticks) {
    char input[3];

    while (ticks == 0) {
        printw("Set number of ticks: ");
        wgetnstr(stdscr, input, 2);
        ticks = atoi(input) > 0 ? atoi(input) : 0;
        clear();
    }

    return sim_new(m, ticks);
}

void sim_run(sim_t *s) {
    int ch = '\0';
    bool is_running = true;
    bool is_reverse = false;

    timeout(0);
    while (ch != 'q' && ch != 'Q') {
        ch = getch();

        switch (ch) {
        case 'p':
        case 'P':
            is_running = !is_running;
            break;
        case 'r':
        case 'R':
            is_reverse = !is_reverse;
            break;
        case 'n':
        case 'N':
            clear();
            sim_free(*s);
            timeout(-1);
            map_t m = map_creation(0, 0);
            map_clear(&m);
            m = map_manipulation(&m);
            *s = sim_creation(m, 0);
            timeout(0);
            break;
        }

        if (is_running) {
            if (s->index != s->ticks && !is_reverse) {
                sim_next(s);
            } else if (s->index != 0 && is_reverse) {
                s->index--;
            }
        }

        clear();
        printw("%zu / %zu\n", s->index, s->ticks);
        sim_draw(s);
        refresh();
        usleep(200 * 1000);
    }
}

int main(int n, char **args) {
    srand(time(NULL));
    size_t width = 0;
    size_t height = 0;
    size_t ticks = 0;

    initscr();
    keypad(stdscr, TRUE);
    resize_term(MAX_HEIGHT, MAX_WIDTH);
    refresh();

    if (n >= 3) {
        width = atoi(args[1]) > 0 ? atoi(args[1]) : 0;
        height = atoi(args[2]) > 0 ? atoi(args[2]) : 0;
    }

    map_t m = map_creation(width, height);
    map_clear(&m);
    m = map_manipulation(&m);

    if (n >= 4) {
        ticks = atoi(args[3]) > 0 ? atoi(args[3]) : 0;
    }

    sim_t s = sim_creation(m, ticks);
    sim_run(&s);
    sim_free(s);

    endwin();
    return 0;
}
