#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define QUEUE_NAME "/coda_logger"
#define MAX_MSG_SIZE 256
#include "Syscalls.h"

int main(){
    mqd_t mq;
    SCALL(mq, mq_open(QUEUE_NAME, O_RDONLY, 0600, NULL), "Errore mq_open");

    char message[MAX_MSG_SIZE];
    int byte_letti;

    for(int i = 0; i<5; i++){
        SCALL_MQREAD(byte_letti, mq_receive(mq, message, MAX_MSG_SIZE, NULL), message, "Errore mq_recive");
    }

    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    return 0;
}