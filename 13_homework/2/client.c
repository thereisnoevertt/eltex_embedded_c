#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <ncurses.h>

#include "message.h"

#define CP_TITLE    1
#define CP_SYSTEM   2 
#define CP_SELF     3 
#define CP_HISTORY  4
#define CP_OTHER    5 


static WINDOW *chat_win  = NULL;
static WINDOW *input_win = NULL;
static pthread_mutex_t ui_mtx = PTHREAD_MUTEX_INITIALIZER;

static volatile sig_atomic_t running = 1;

static mqd_t server_queue    = (mqd_t)-1;
static mqd_t my_queue        = (mqd_t)-1;
static char  my_queue_name[64];
static char  my_name[NAME_LEN];
static pid_t my_pid;

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

static void _chat_puts(int colour, int extra_attr, const char *text)
{
    wattron(chat_win,  COLOR_PAIR(colour) | extra_attr);
    waddstr(chat_win,  text);
    wattroff(chat_win, COLOR_PAIR(colour) | extra_attr);
    waddch(chat_win, '\n');
    wrefresh(chat_win);
    wrefresh(input_win); 
}

static void ui_append(int colour, int extra_attr,
                       const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

static void ui_append(int colour, int extra_attr,
                       const char *fmt, ...)
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
        ui_append(CP_SYSTEM, A_BOLD,
                  "  *** %s ***", msg->text);
        break;

    case MSG_HIST:
        ui_append(CP_HISTORY, 0,
                  "  [~] %s: %s", msg->name, msg->text);
        break;

    case MSG_TEXT:
        if (msg->sender_pid == my_pid)
            ui_append(CP_SELF,  A_BOLD, "  %s: %s",
                      msg->name, msg->text);
        else
            ui_append(CP_OTHER, 0,      "  %s: %s",
                      msg->name, msg->text);
        break;

    case MSG_JOIN_NOTIFY:
        ui_append(CP_SYSTEM, A_BOLD | A_UNDERLINE,
                  "  *** %s joined the chat ***", msg->name);
        break;

    case MSG_LEAVE_NOTIFY:
        ui_append(CP_SYSTEM, A_BOLD | A_UNDERLINE,
                  "  *** %s left the chat ***", msg->name);
        break;

    default:
        ui_append(CP_OTHER, 0, "  [?] type=%d", msg->type);
    }
}

static void *recv_thread_fn(void *arg)
{
    (void)arg;
    Message msg;
while (running) {
        ssize_t n = mq_receive(my_queue,
                               (char *)&msg, sizeof(msg), NULL);
        if (n == -1) {
            if (errno == EAGAIN) { usleep(5000); continue; }
            if (errno == EINTR)  continue;
            break;
        }
        display_message(&msg);
    }
    return NULL;
}


static void draw_title(void)
{
    int cols = getmaxx(stdscr);

    attron(COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    mvhline(0, 0, ' ', cols);
    char buf[128];
    snprintf(buf, sizeof(buf),
             " Chat Room — %s   (Ctrl+Q to quit)", my_name);
    mvprintw(0, 0, "%s", buf);
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    refresh();
}

static void draw_separator(void)
{
    int rows = getmaxy(stdscr);
    int cols = getmaxx(stdscr);
    attron(A_DIM);
    mvhline(rows - 2, 0, ACS_HLINE, cols);
    attroff(A_DIM);
    refresh();
}

static void ui_init(void)
{
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

static void ui_teardown(void)
{
    if (input_win) { 
        delwin(input_win); 
        input_win = NULL; 
    }
    if (chat_win)  { 
        delwin(chat_win);  
        chat_win  = NULL; 
    }
    endwin();
}


static void mq_cleanup(void)
{
    if (server_queue != (mqd_t)-1) {
        Message lv;
        memset(&lv, 0, sizeof(lv));
        lv.type = MSG_LEAVE;
        lv.sender_pid = my_pid;
        strncpy(lv.name, my_name, NAME_LEN - 1);
        mq_send(server_queue, (const char *)&lv, sizeof(lv), 0);
        mq_close(server_queue);
        server_queue = (mqd_t)-1;
    }
    if (my_queue != (mqd_t)-1) {
        mq_close(my_queue);
        my_queue = (mqd_t)-1;
    }
    mq_unlink(my_queue_name);

    pthread_mutex_destroy(&ui_mtx);
}


int main(void)
{
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    my_pid = getpid();

    printf("Enter your name: ");
    fflush(stdout);
    if (!fgets(my_name, NAME_LEN, stdin)) return 1;
    my_name[strcspn(my_name, "\n")] = '\0';
    if (!my_name[0]){
        strncpy(my_name, "Anonymous", NAME_LEN - 1);
    }

    snprintf(my_queue_name, sizeof(my_queue_name),
             "/client_%d", (int)my_pid);

    struct mq_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = sizeof(Message);

    mq_unlink(my_queue_name);
    my_queue = mq_open(my_queue_name,
                       O_CREAT | O_RDONLY | O_NONBLOCK, 0666, &attr);
    if (my_queue == (mqd_t)-1) {
        perror("mq_open own queue");
        return 1;
    }

    server_queue = mq_open(MAIN_QUEUE_NAME, O_WRONLY);
    if (server_queue == (mqd_t)-1) {
        perror("mq_open server queue (is the server running?)");
        mq_unlink(my_queue_name);
        return 1;
    }

    {
        Message jreq;
        memset(&jreq, 0, sizeof(jreq));
        jreq.type = MSG_JOIN_REQ;
        jreq.sender_pid = my_pid;
        strncpy(jreq.queue_name, my_queue_name, 63);
        strncpy(jreq.name, my_name, NAME_LEN - 1);

        if (mq_send(server_queue,
                    (const char *)&jreq, sizeof(jreq), 0) == -1) {
            perror("mq_send JOIN_REQ");
            mq_cleanup();
            return 1;
        }
    }

    ui_init();

    pthread_t recv_tid;
    pthread_create(&recv_tid, NULL, recv_thread_fn, NULL);

    char ibuf[TEXT_LEN];
    int  ilen = 0;
    memset(ibuf, 0, sizeof(ibuf));

    while (running) {
        pthread_mutex_lock(&ui_mtx);
        int ch = wgetch(input_win);
        pthread_mutex_unlock(&ui_mtx);

        if (ch == ERR  || ch == 0)
            continue;

        if (ch == ('q' & 0x1f) || ch == ('c' & 0x1f)) {
            running = 0;
            break;
        }

        if (ch == '\n' || ch == KEY_ENTER) {
            if (ilen == 0) continue;

            ibuf[ilen] = '\0';

            Message msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = MSG_TEXT;
            msg.sender_pid = my_pid;
            strncpy(msg.name,       my_name,        NAME_LEN - 1);
            strncpy(msg.queue_name, my_queue_name,  63);
            strncpy(msg.text,       ibuf,           TEXT_LEN - 1);

            if (mq_send(server_queue,
                        (const char *)&msg, sizeof(msg), 0) == -1)
                perror("mq_send TEXT");

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
                (void)y;
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

    running = 0;
    pthread_join(recv_tid, NULL);

    ui_teardown();
    mq_cleanup();
    return 0;
}
