#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>



int main(){
    mqd_t mq;
    char buffer[1024];
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1024;
    attr.mq_curmsgs = 0;

    mq = mq_open("/test_queue", O_CREAT | O_RDWR, &attr);
    //controllo -1

    pid_t pid = fork();
    //controllo -1

    if (pid > 0){
        int num_bytes;

        while(1){
            printf("Inserisci un messaggio (o exit per uscire): \n");
            fflush(stdout);

            num_bytes = read(STDIN_FILENO, buffer, 1024);
            //controllo se -1

            if(num_bytes > 0 && buffer[num_bytes -1] == '\n'){
                buffer[num_bytes - 1 ] = '\0';
            }

            if (strcmp(buffer, "exit") == 0){
                mq_send(mq, buffer, strlen(buffer)+ 1, 0);
                break;
            }

            mq_send(mq, buffer, strlen(buffer)+ 1, 0);
            //controllo -1 prima 
            
        }

        mq_close(mq);
        //controllo -1
        
        wait(NULL);
        //equivalente di int statom; waitpid(pid, &stato, 0);

        mq_unlink("/test_queue");
        //controllo -1

        
    }

}