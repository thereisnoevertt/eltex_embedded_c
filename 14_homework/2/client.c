#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <ncurses.h>

#define HISTORY_SIZE 10
#define MAX_CLIENTS 10
#define NAME_LEN 32
#define TEXT_LEN 256
#define SHM_NAME "/chat_shm"

typedef enum {
    MSG_JOIN_REQ = 1,
    MSG_JOIN_OK = 2,
    MSG_TEXT = 3,
    MSG_JOIN_NOTIFY = 4,
    MSG_LEAVE = 5,
    MSG_LEAVE_NOTIFY = 6,
} MsgType;

typedef struct {
    pid_t client_pid;
    char name[NAME_LEN];
} ClientEntry;

typedef struct {
    ClientEntry owner;
    MsgType type;
    pid_t sender_pid;
    char to[NAME_LEN];
    char text[TEXT_LEN];
} Message;

typedef struct {
    Message msgs[HISTORY_SIZE];
    int msg_count;
    int write_index;
} shared_chat;

/* ncurses */
#define CP_TITLE    1
#define CP_SYSTEM   2
#define CP_SELF     3
#define CP_HISTORY  4
#define CP_OTHER    5

static WINDOW *chat_win = NULL;
static WINDOW *input_win = NULL;
static pthread_mutex_t ui_mtx = PTHREAD_MUTEX_INITIALIZER;

static volatile sig_atomic_t running = 1;

static shared_chat *chat = NULL;
static sem_t *mutex_sem = NULL;
static sem_t *event_sem = NULL;

static char my_name[NAME_LEN];
static pid_t my_pid;
static int last_processed = 0;   /* сколько сообщений уже показано в чате */


static void _chat_puts(int colour, int extra_attr, const char *text){
    wattron(chat_win, COLOR_PAIR(colour) | extra_attr);
    waddstr(chat_win, text);
    wattroff(chat_win, COLOR_PAIR(colour) | extra_attr);
    waddch(chat_win, '\n');
    wrefresh(chat_win);
    wrefresh(input_win);
}

static void ui_append(int colour, int extra_attr, const char *fmt, ...)
{
    char buf[600];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    pthread_mutex_lock(&ui_mtx);
    _chat_puts(colour, extra_attr, buf);
    pthread_mutex_unlock(&ui_mtx);
}

static void display_message(const Message *msg)
{
    switch (msg->type) {
        case MSG_JOIN_OK:
            if (strcmp(msg->to, my_name) == 0)
                ui_append(CP_SYSTEM, A_BOLD, "  *** %s ***", msg->text);
            break;
        case MSG_TEXT:
            if (msg->sender_pid == my_pid)
                ui_append(CP_SELF, A_BOLD, "  %s: %s",
                          msg->owner.name, msg->text);
            else
                ui_append(CP_OTHER, 0, "  %s: %s",
                          msg->owner.name, msg->text);
            break;
        case MSG_JOIN_NOTIFY:
            ui_append(CP_SYSTEM, A_BOLD | A_UNDERLINE,
                      "  *** %s joined the chat ***", msg->text);
            break;
        case MSG_LEAVE_NOTIFY:
            ui_append(CP_SYSTEM, A_BOLD | A_UNDERLINE,
                      "  *** %s left the chat ***", msg->text);
            break;
        default:
            break;
    }
}

static void send_message(MsgType type, const char *text, const char *to)
{
    Message msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = type;
    msg.sender_pid = my_pid;
    snprintf(msg.owner.name, NAME_LEN, "%s", my_name);
    msg.owner.client_pid = my_pid;
    if (to) snprintf(msg.to, NAME_LEN, "%s", to);
    if (text) snprintf(msg.text, TEXT_LEN, "%s", text);

    sem_wait(mutex_sem);
    chat->msgs[chat->write_index] = msg;
    chat->msg_count++;
    chat->write_index = (chat->write_index + 1) % HISTORY_SIZE;
    sem_post(mutex_sem);
    sem_post(event_sem);   // разбудить сервер
}
/*поток фонового чтения (активный опрос)*/
static void *recv_thread_fn(void *arg) {
    (void)arg;
    int my_last = last_processed;
    while (running) {
        usleep(100000);
        sem_wait(mutex_sem);
        if (chat->msg_count - my_last > HISTORY_SIZE) {
            ui_append(CP_SYSTEM, A_BOLD, "  *** History lost ***");
            my_last = chat->msg_count - HISTORY_SIZE;
        }
        while (my_last < chat->msg_count) {
            int idx = my_last % HISTORY_SIZE;
            display_message(&chat->msgs[idx]);
            my_last++;
        }
        last_processed = my_last;
        sem_post(mutex_sem);
    }
    return NULL;
}

static void draw_title(void) {
    int cols = getmaxx(stdscr);
    attron(COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    mvhline(0, 0, ' ', cols);
    char buf[128];
    snprintf(buf, sizeof(buf), " Shared memory chat — %s   (Ctrl+Q to quit)", my_name);
    mvprintw(0, 0, "%s", buf);
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    refresh();
}

static void draw_separator(void) {
    int rows = getmaxy(stdscr);
    int cols = getmaxx(stdscr);
    attron(A_DIM);
    mvhline(rows - 2, 0, ACS_HLINE, cols);
    attroff(A_DIM);
    refresh();
}

static void ui_init(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_TITLE,   COLOR_BLACK,  COLOR_CYAN);
        init_pair(CP_SYSTEM,  COLOR_YELLOW, -1);
        init_pair(CP_SELF,    COLOR_GREEN,  -1);
        init_pair(CP_HISTORY, COLOR_WHITE,  -1);
        init_pair(CP_OTHER,   COLOR_CYAN,   -1);
    }

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    draw_title();
    draw_separator();

    chat_win = newwin(rows - 3, cols, 1, 0);
    scrollok(chat_win, TRUE);
    idlok(chat_win, TRUE);
    wrefresh(chat_win);

    input_win = newwin(1, cols, rows - 1, 0);
    keypad(input_win, TRUE);
    wtimeout(input_win, 50);
    waddstr(input_win, "> ");
    wrefresh(input_win);
}

static void ui_teardown(void) {
    if (input_win) { delwin(input_win); input_win = NULL; }
    if (chat_win)  { delwin(chat_win);  chat_win  = NULL; }
    endwin();
}

static void sig_handler(int sig) {
    (void)sig;
    running = 0;
}


int main(void) {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    my_pid = getpid();

    printf("Enter your name: ");
    fflush(stdout);
    if (!fgets(my_name, sizeof(my_name), stdin)) {
        return EXIT_FAILURE;
    }
    my_name[strcspn(my_name, "\n")] = '\0';
    if (my_name[0] == '\0')
        snprintf(my_name, NAME_LEN, "Anonymous");

    /* Открыть разделяемую память */
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open (is the server running?)");
        return EXIT_FAILURE;
    }
    chat = mmap(NULL, sizeof(shared_chat), PROT_READ | PROT_WRITE,
                MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    if (chat == MAP_FAILED) {
        perror("mmap");
        return EXIT_FAILURE;
    }
/* Открыть семафоры */
    mutex_sem = sem_open("/mutex_sem", 0);
    event_sem = sem_open("/event_sem", 0);
    if (mutex_sem == SEM_FAILED || event_sem == SEM_FAILED) {
        perror("sem_open");
        munmap(chat, sizeof(shared_chat));
        return EXIT_FAILURE;
    }

    Message history[1000];
    int history_cnt = 0;
    int temp_last = 0;

    sem_wait(mutex_sem);
    int initial_count = chat->msg_count;
    sem_post(mutex_sem);

    /* Отправить запрос на регистрацию */
    send_message(MSG_JOIN_REQ, "", "");

    /* Дождаться MSG_JOIN_OK и одновременно собрать всю историю (все сообщения от начала до JOIN_OK) */
    int registered = 0;
    int timeout = 500;
    while (running && !registered && timeout-- > 0) {
        sem_wait(mutex_sem);
        while (temp_last < chat->msg_count) {
            int idx = temp_last % HISTORY_SIZE;
            const Message *msg = &chat->msgs[idx];
            if (msg->type == MSG_JOIN_OK && strcmp(msg->to, my_name) == 0) {
                registered = 1;
                temp_last++;   // пропустить JOIN_OK
                break;
            } 
            else {
                /* сохраняем значимые сообщения в историю */
                if (msg->type == MSG_TEXT || msg->type == MSG_JOIN_NOTIFY || msg->type == MSG_LEAVE_NOTIFY) {
                    if (history_cnt < 1000)
                        history[history_cnt++] = *msg;
                }
                temp_last++;
            }
        }
        sem_post(mutex_sem);
        if (!registered) usleep(100000);
    }

    if (!registered) {
        fprintf(stderr, "Registration failed (name taken or server error)\n");
        sem_close(mutex_sem);
        sem_close(event_sem);
        munmap(chat, sizeof(shared_chat));
        return EXIT_FAILURE;
    }

    sem_wait(mutex_sem);
    while (temp_last < chat->msg_count) {
        int idx = temp_last % HISTORY_SIZE;
        const Message *msg = &chat->msgs[idx];
        if (msg->type == MSG_TEXT || msg->type == MSG_JOIN_NOTIFY || msg->type == MSG_LEAVE_NOTIFY) {
            if (history_cnt < 1000)
                history[history_cnt++] = *msg;
        }
        temp_last++;
    }
    last_processed = chat->msg_count;
    sem_post(mutex_sem);

    ui_init();

    for (int i = 0; i < history_cnt; i++)
        display_message(&history[i]);

    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, recv_thread_fn, NULL);

    char ibuf[TEXT_LEN];
    int ilen = 0;
    memset(ibuf, 0, sizeof(ibuf));

    while (running) {
        pthread_mutex_lock(&ui_mtx);
        int ch = wgetch(input_win);
        pthread_mutex_unlock(&ui_mtx);

        if (ch == ERR || ch == 0)
            continue;

        if (ch == ('q' & 0x1f) || ch == ('c' & 0x1f)) {
            running = 0;
            break;
        }

        if (ch == '\n' || ch == KEY_ENTER) {
            if (ilen == 0) continue;
            ibuf[ilen] = '\0';
            send_message(MSG_TEXT, ibuf, "*");
            ilen = 0;
            memset(ibuf, 0, sizeof(ibuf));

            pthread_mutex_lock(&ui_mtx);
            werase(input_win);
            waddstr(input_win, "> ");
            wrefresh(input_win);
            pthread_mutex_unlock(&ui_mtx);
            continue;
        }
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
            if (ilen > 0) {
                ilen--;
                ibuf[ilen] = '\0';
                pthread_mutex_lock(&ui_mtx);
                int y, x;
                getyx(input_win, y, x);
                if (x > 2) {
                    wmove(input_win, 0, x - 1);
                    wdelch(input_win);
                    wrefresh(input_win);
                }
                pthread_mutex_unlock(&ui_mtx);
            }
            continue;
        }

        if (ch >= 32 && ch < 127 && ilen < TEXT_LEN - 1) {
            ibuf[ilen++] = (char)ch;
            pthread_mutex_lock(&ui_mtx);
            waddch(input_win, (chtype)ch);
            wrefresh(input_win);
            pthread_mutex_unlock(&ui_mtx);
        }
    }

    /* Завершение */
    running = 0;
    send_message(MSG_LEAVE, "", "");
    usleep(200000);

    pthread_join(recv_tid, NULL);
    ui_teardown();

    sem_close(mutex_sem);
    sem_close(event_sem);
    munmap(chat, sizeof(shared_chat));

    return 0;
}