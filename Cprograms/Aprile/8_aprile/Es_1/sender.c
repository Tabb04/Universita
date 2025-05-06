#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>

#define NOME_CODA "/coda"
#define DIM_MSG 256

int main(){
 
    mqd_t mq;
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = DIM_MSG;
    attr.mq_curmsgs = 0;

    mq = mq_open(NOME_CODA, O_CREAT | O_RDWR, 0644, &attr);

    char buffer[DIM_MSG];
    printf("Scrivi messaggio da inviare (Ctrl + D per terminare): \n");
    while(fgets(buffer, DIM_MSG, stdin) != NULL){
        mq_send(mq, buffer, strlen(buffer) + 1, 0);
    }

    //invio messaggio di terminazione
    strcpy(buffer, "EXIT!");
    mq_send(mq, buffer, strlen(buffer) + 1, 0);
    
    mq_close(mq);
    mq_unlink(NOME_CODA);
    return 0;
}