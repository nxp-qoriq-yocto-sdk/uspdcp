#ifdef __cplusplus

extern "C"{

#endif


#include "messaging.h"
#include <sys/msg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
 

static MessageQueue *queues = NULL;
static int queue_count = 0;

void clean_up_messaging();

int start_messaging(int tag) {
    if (queue_count == 0) {
        atexit(&clean_up_messaging);
    }
    queues = (MessageQueue *)realloc(queues, sizeof(MessageQueue) * ++queue_count);
    queues[queue_count - 1].queue = msgget((long)getpid(), 0666 | IPC_CREAT);
    queues[queue_count - 1].owner = getpid();
    queues[queue_count - 1].tag = tag;
    return queue_count - 1;
}

void send_message(int messaging, int result) {
    Message *message = (Message *)malloc(sizeof(Message));
    message->type = queues[messaging].tag;
    message->result = result;
    msgsnd(queues[messaging].queue, message, sizeof(Message), 0);
    free(message);
}

int receive_message(int messaging) {
    Message *message = (Message *)malloc(sizeof(Message));
    ssize_t received = msgrcv(queues[messaging].queue, message, sizeof(Message), queues[messaging].tag, IPC_NOWAIT);
    int result = (received > 0 ? message->result : 0);
    free(message);
    return result;
}
 void clean_up_messaging() {
    int i;
    for (i = 0; i < queue_count; i++) {
        if (queues[i].owner == getpid()) {
            msgctl(queues[i].queue, IPC_RMID, NULL);
        }
    }
    free(queues);
    queues = NULL;
    queue_count = 0;

    
}

#ifdef __cplusplus

}
 
#endif
