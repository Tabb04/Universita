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
    struct mq_attr attr;
    int ris = 0;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    SCALL(mq, mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr), "Errore mq_open");

    char message[MAX_MSG_SIZE];
    for(int i = 0; i<5; i++){
        snprintf(message, MAX_MSG_SIZE, "Messaggio numero %d", i);
        mq_send(mq, message, strlen(message)+ 1, 0);
        printf("Produttore %d ha inviato messaggio '%s'\n", i, message);
        sleep(1);
    }

    SCALL(ris, mq_close(mq), "Errore close");
    return 0;
}