#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFF_SIZE 1024


int main(){
    mqd_t mq;
    char buffer[BUFF_SIZE];
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1024;
    attr.mq_curmsgs = 0;


    mq = mq_open("/queue_test", O_CREAT | O_RDWR, 0644, &attr);
    //controllo -1

    pid_t pid = fork();

    if (pid > 0){
        //padre
        mq_send(mq, "Messaggio iniziale", strlen("Messaggio iniziale")+1, 0);
        //controllo -1

        mq_close(mq);
        //controllo -1

        waitpid(pid, NULL, 0);

        mq_unlink("/queue_test");
        //controllo -1

        printf("Coda eliminata, processo padre termina\n");
    
    }else{
        //processo figlio
        ssize_t byte_letti = mq_receive(mq, buffer, 1024, NULL);
        //controllo -1

        printf("Ricevuto messaggio: %s\n", buffer);

        mq_close(mq);
    }

    return 0;

}