#ifndef SERVER_H
#define SERVER_H

#include <ncurses.h>

#define PATH_MAX 4096

typedef struct {
    char path[PATH_MAX];
    char **items;
    int count;
    int selected;
    int scroll;
} Panel;

void panel_init(Panel *p, const char *start_path);
void panel_load(Panel *p);
void panel_free(Panel *p);

void panel_move_up(Panel *p);
void panel_move_down(Panel *p, int visible_rows);

void panel_enter(Panel *p);
void panel_go_up(Panel *p);

int is_directory(const char *base, const char *name);

void draw_panel(WINDOW *win, Panel *p, int active);

#endif