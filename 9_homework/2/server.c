#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ncurses.h>
#include "server.h"

static void free_items(Panel *p) {
    for (int i = 0; i < p->count; i++) {
        free(p->items[i]);
    }

    free(p->items);
    p->items = NULL;
    p->count = 0;
}

void panel_init(Panel *p, const char *start_path) {
    strncpy(p->path, start_path, PATH_MAX);
    p->selected = 0;
    p->scroll = 0;
    p->items = NULL;
    p->count = 0;
}

void panel_load(Panel *p) {
    free_items(p);

    DIR *dir = opendir(p->path);
    if (!dir) {
        return;
    }

    int cap = 32;
    p->items = malloc(sizeof(char*) * cap);

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (p->count >= cap) {
            cap *= 2;
            p->items = realloc(p->items, sizeof(char*) * cap);
        }
        p->items[p->count++] = strdup(entry->d_name);
    }
    closedir(dir);

    p->selected = 0;
    p->scroll = 0;
}

void panel_free(Panel *p) {
    free_items(p);
}

void panel_move_up(Panel *p) {
    if (p->selected > 0) {
        p->selected--;
    }
    if (p->selected < p->scroll){
        p->scroll--;
    }
}

void panel_move_down(Panel *p, int visible_rows) {
    if (p->selected < p->count - 1) {
        p->selected++;
    }
    if (p->selected >= p->scroll + visible_rows) {
        p->scroll++;
    }
}

int is_directory(const char *base, const char *name) {
    char full[PATH_MAX];
    snprintf(full, sizeof(full), "%s/%s", base, name);

    struct stat st;
    return (stat(full, &st) == 0 && S_ISDIR(st.st_mode));
}

void panel_go_up(Panel *p) {
    if (strcmp(p->path, "/") == 0) return;

    char *slash = strrchr(p->path, '/');
    if (slash && slash != p->path)
        *slash = '\0';
    else
        strcpy(p->path, "/");

    panel_load(p);
}

void panel_enter(Panel *p) {
    if (p->count == 0) return;

    char *name = p->items[p->selected];

    if (strcmp(name, ".") == 0) return;

    if (strcmp(name, "..") == 0) {
        panel_go_up(p);
        return;
    }

    if (is_directory(p->path, name)) {
        if (strcmp(p->path, "/") == 0)
            snprintf(p->path, sizeof(p->path), "/%s", name);
        else {
            strcat(p->path, "/");
            strcat(p->path, name);
        }
        panel_load(p);
    }
}

void draw_panel(WINDOW *win, Panel *p, int active) {
    werase(win);
    box(win, 0, 0);

    if (active) wattron(win, A_BOLD);
    mvwprintw(win, 0, 2, " %s ", p->path);
    if (active) wattroff(win, A_BOLD);

    int h, w;
    getmaxyx(win, h, w);

    int visible_rows = h - 2;

    for (int i = 0; i < visible_rows && i + p->scroll < p->count; i++) {
        int idx = i + p->scroll;

        if (idx == p->selected && active)
            wattron(win, A_REVERSE);

        int dir = is_directory(p->path, p->items[idx]);
        mvwprintw(win, i + 1, 1, "%s%s", p->items[idx], dir ? "/" : "");

        if (idx == p->selected && active)
            wattroff(win, A_REVERSE);
    }

    wrefresh(win);
}