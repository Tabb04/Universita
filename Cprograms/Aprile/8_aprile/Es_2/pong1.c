#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>


int main(){

    sem_t *sem1 = sem_open("/primo_sem", 0);
    sem_t *sem2 = sem_open("/secondo_sem", 0);

    for(int i = 0; i<5; i++){
        sleep(1);
        sem_wait(sem2);
        printf("Ciao?\n");
        fflush(stdout);
        sem_post(sem1);
        sleep(1);
    }

    sem_close(sem1);
    sem_close(sem2);

    sem_unlink("/primo_sem");
    sem_unlink("/secondo_sem");

    return 0;

}