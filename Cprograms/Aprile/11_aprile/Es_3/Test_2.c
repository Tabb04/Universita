#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>
#include "macro.h"

#define SHM_SIZE 1024
#define SHM_NAME "/shared_counter"
#define NUM_ITERATIONS 10

typedef struct {
    sem_t mutex;
    int counter;
} shared_data_t;

void cleanup(shared_data_t *data, int shm_fd, int is_parent, int first_time) {
    int ret;

    // Rimuovi la mappatura della memoria condivisa
    if (data != NULL) {
        SCALL(ret, munmap(data, sizeof(shared_data_t)), "Errore nel munmap");
    }

    // Chiudi il file descriptor
    if (shm_fd >= 0) {
        SCALL(ret, close(shm_fd), "Errore nel close");
    }

    // Distruggi il semaforo e rimuovi la memoria condivisa solo nel processo padre
    if (is_parent && first_time) {
        SCALL(ret, sem_destroy(&data->mutex), "Errore nel sem_destroy");
        SCALL(ret, shm_unlink(SHM_NAME), "Errore nello shm_unlink");
    }
}

int main() {
    int shm_fd = -1, ret;
    shared_data_t *data = NULL;
    int first_time = 0;

    // Tenta di aprire o creare la shared memory
    shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm_fd == -1) {
        if (errno == EEXIST) {
            shm_fd = shm_open(SHM_NAME, O_RDWR, 0600);
        } else {
            perror("shm_open");
            exit(1);
        }
    } else {
        // Se l'ha appena creata, bisogna inizializzarla
        ftruncate(shm_fd, sizeof(shared_data_t));
        first_time = 1;
    }

    // Mappatura della shared memory
    data = mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        cleanup(data, shm_fd, 1, first_time);
        exit(1);
    }

    // Inizializza il semaforo solo una volta (pshared=1)
    if (first_time) {
        SCALL(ret, sem_init(&data->mutex, 1, 1), "Errore nell'inizializzazione del semaforo");
        data->counter = 0;
    }

    // Creazione del processo figlio
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        cleanup(data, shm_fd, 1, first_time);
        exit(1);
    }

    // Entrambi i processi (padre e figlio) incrementano il contatore
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Attendi il semaforo
        SCALL(ret, sem_wait(&data->mutex), "Errore nel sem_wait");

        // Sezione critica
        int val = data->counter;
        printf("PID %d legge %d, lo incrementa a %d\n", getpid(), val, val + 1);
        data->counter = val + 1;

        // Rilascia il semaforo
        SCALL(ret, sem_post(&data->mutex), "Errore nel sem_post");

        sleep(1); // pausa per rendere visibile la concorrenza
    }

    // Pulizia delle risorse
    cleanup(data, shm_fd, pid > 0, first_time);

    return 0;
}   