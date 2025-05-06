#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <unistd.h>

#define NOME_CODA "/coda"
#define DIM_MSG 256


int main(){

    mqd_t mq = mq_open(NOME_CODA, O_RDONLY, 0600, NULL);
    char buffer[DIM_MSG];
    while (1) {
        ssize_t bytes_read = mq_receive(mq, buffer, DIM_MSG, NULL);
        if (bytes_read >= 0) {
            if (strcmp(buffer, "EXIT!") == 0) {
                printf("Terminazione ricevuta. Chiudo il receiver.\n");
                break;
            }
            printf("Ricevuto: %s", buffer);
        } else {
            perror("mq_receive");
            break;
        }
    }

    mq_close(mq);
    mq_unlink(NOME_CODA);
    return 0;

}