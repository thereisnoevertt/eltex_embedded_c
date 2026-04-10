#pragma once
 
#include <sys/types.h>
 
#define MAIN_QUEUE_NAME "/main_chat"
#define HISTORY_SIZE    10
#define MAX_CLIENTS     10
#define NAME_LEN        32
#define TEXT_LEN        256
 
typedef enum {
    MSG_JOIN_REQ     = 1,  /* client → server: join request          */
    MSG_JOIN_OK      = 2,  /* server → client: welcome + history done*/
    MSG_TEXT         = 3,  /* client ↔️ server ↔️ all clients          */
    MSG_JOIN_NOTIFY  = 4,  /* server → clients: someone joined       */
    MSG_HIST         = 5,  /* server → new client: history replay    */
    MSG_LEAVE        = 6,  /* client → server: graceful disconnect   */
    MSG_LEAVE_NOTIFY = 7,  /* server → clients: someone left         */
} MsgType;
 
typedef struct {
    MsgType type;
    pid_t   sender_pid;
    char    queue_name[64];
    char    name[NAME_LEN];
    char    text[TEXT_LEN];
} Message;