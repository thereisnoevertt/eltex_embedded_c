#include <ncurses.h>
#include <unistd.h>
#include "server.h"

int main() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);

    int h, w;
    getmaxyx(stdscr, h, w);

    WINDOW *left_win  = newwin(h - 1, w / 2, 0, 0);
    WINDOW *right_win = newwin(h - 1, w - w/2, 0, w / 2);
    WINDOW *status    = newwin(1, w, h - 1, 0);

    Panel left, right;
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    panel_init(&left, cwd);
    panel_init(&right, cwd);
    panel_load(&left);
    panel_load(&right);

    int active = 0, ch;

    while ((ch = getch()) != 'q') {
        int ph, pw;
        getmaxyx(left_win, ph, pw);
        int visible_rows = ph - 2;

        Panel *p = active ? &right : &left;

        switch (ch) {
            case KEY_UP: panel_move_up(p); break;
            case KEY_DOWN: panel_move_down(p, visible_rows); break;
            case 10: panel_enter(p); break;
            case KEY_BACKSPACE:
            case 127: panel_go_up(p); break;
            case 9: active = !active; break;
        }

        draw_panel(left_win, &left, !active);
        draw_panel(right_win, &right, active);

        werase(status);
        mvwprintw(status, 0, 0, "TAB switch | ENTER open | BACKSPACE up | q quit");
        wrefresh(status);
    }

    endwin();
    panel_free(&left);
    panel_free(&right);
    return 0;
}