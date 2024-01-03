#include "simulation.h"
#include <stdlib.h>
#include "curses.h"
#include <time.h>

#define MAX_WIDTH 100
#define MAX_HEIGHT 50

int main(int n, char **args) {
    srand(time(NULL));
    size_t width = 0;
    size_t height = 0;
    size_t ticks = 0;
    char input[3];

    initscr();
    resize_term(MAX_HEIGHT, MAX_WIDTH);
    refresh();

    if (n >= 3) {
        width = atoi(args[1]) > 0 ? atoi(args[1]) : 0;
        height = atoi(args[2]) > 0 ? atoi(args[2]) : 0;
    }

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

    if (n >= 4) {
        ticks = atoi(args[3]) > 0 ? atoi(args[3]) : 0;
    }

    while (ticks == 0) {
        printw("Set number of ticks: ");
        wgetnstr(stdscr, input, 2);
        ticks = atoi(input) > 0 ? atoi(input) : 0;
        clear();
    }

    map_t m = map_new(width, height);
    map_clear(m);

    int ch = '\0';
    size_t x = 0;
    size_t y = 0;
    keypad(stdscr, TRUE);

    while(ch != '\n') {
        clear();
        map_draw(m);
        move(y, x);
        refresh();
        ch = getch();

        switch (ch) {
            case 'q':
            case 'Q':
                map_free(m);
                endwin();
                return 0;
            case 'r':
            case 'R':
                map_gen_rnd(m);
                break;
            case 'c':
            case 'C':
                map_clear(m);
                break;
            case 'u':
            case 'U':
                clear();
                printw("Save as: ");
                refresh();
                char* name_u = malloc(100);
                getstr(name_u);
                map_save(m, name_u);
                free(name_u);
                break;
            case 'l':
            case 'L':
                clear();
                printw("Load from: ");
                refresh();
                char* name_r = malloc(100);
                getstr(name_r);
                map_t new_m = map_load(name_r);
                free(name_r);
                if (new_m.width >= MAX_WIDTH || new_m.height >= MAX_HEIGHT) {
                    map_free(new_m);
                } else {
                    map_free(m);
                    m = new_m;
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
                y += y == m.height - 1 ? 0 : 1;
                break;
            case 'd':
            case 'D':
            case KEY_RIGHT:
                x += x == m.width - 1 ? 0 : 1;
                break;
            case ' ':
                map_set(m, x, y, !map_get(m, x, y));
                break;
        }
    }

    sim_t s = sim_new(width, height, ticks, m);
    sim_free(s);
    endwin();
    return 0;
}
