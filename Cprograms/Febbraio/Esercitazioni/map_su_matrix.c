#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>
#include <time.h>

#define N 5
#define M 10 //dimensioni matrice


mtx_t mutex;
cnd_t condition;
int iterazione = 0;

int input_data[M][N];


typedef struct {
	int elemento;
	int (*f)(int);
} main_data_t;
main_data_t thread_data[N];

void stampa_matrice(char * premessa, int data[M][N], int m, int n);

void cambio_dati(){
    for(int i = 0; i<N; i++){
        input_data[iterazione][i] = thread_data[i].elemento;
        if (iterazione + 1 < M) {
            thread_data[i].elemento = input_data[iterazione+1][i]; // Correzione qui
        }
    }
    iterazione++;
}

int count = 0;
void barrier(){
    mtx_lock(&mutex);
    count++;
    if(count == N){
        count = 0;
        cambio_dati();
		stampa_matrice("\nRisultati in output nella barrier", input_data, M, N);
        thrd_sleep(&(struct timespec){0, 500000000L}, NULL); // Pausa 500ms
        cnd_broadcast(&condition);
        mtx_unlock(&mutex);
    }else{
        cnd_wait(&condition, &mutex);
        mtx_unlock(&mutex);
    }
}

void stampa_matrice(char * premessa, int data[M][N], int m, int n) {
	printf("%s: \n", premessa);
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < n; j++) {
			printf("%d, ", data[i][j]);
		}
		printf("\n");
	}
	printf(" ----- \n");
}


void stampa(char * premessa, main_data_t * data, int n) {
	printf("%s: ", premessa);
	for (int i = 0; i < n; i++) {
		printf("%d, ", data[i].elemento);
	}
	printf("\n");
}


int quadrato(int i){return i * i;};

int map (void * arg){
    main_data_t *t = (main_data_t*)arg;
    
    do{
        int data = t->elemento;
        t->elemento = t->f(data);

		printf("Ho elaborato, mi metto in attesa: %d\n", t->elemento);
        barrier();

    }while(iterazione < M);

    return thrd_success;
}

int main(){

    srand(time(NULL));
    mtx_init(&mutex, mtx_plain);
    cnd_init(&condition);
    thrd_t threads[N];

    for(int i = 0; i<M ; i++){
        for(int j = 0; j<N; j++){
            input_data[i][j] = rand() % 100;
        }
    }

	stampa_matrice("Elementi in input", input_data, M, N);


    for(int i = 0; i<N; i++){
        thread_data[i].elemento = input_data[iterazione][i];
        thread_data[i].f = quadrato;
        thrd_create(&threads[i], map, &thread_data[i]);
    }

    for(int i = 0; i<N; i++){
        thrd_join(threads[i], NULL);
    }

    mtx_destroy(&mutex);
    cnd_destroy(&condition);

    return 0;

}
