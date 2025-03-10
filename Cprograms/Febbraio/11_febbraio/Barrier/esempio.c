/* 	
	Questo programma mostra come realizzare una barrier utilizzando
	le variabili di condizione di C11
*/

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdatomic.h>
#include <time.h>

#define N 5

int count = 0;
mtx_t mtx;
cnd_t cnd;

int barrier() {
	mtx_lock(&mtx);
	count++;
	if (count == N) {
		count = 0;
		cnd_broadcast(&cnd);
		mtx_unlock(&mtx);
		return 1;
	} else {
		cnd_wait(&cnd, &mtx);	//restituisco la lock, mi metto in attesa e voglio essere svegliato
								//quando ho la lock di nuovo per me 
		mtx_unlock(&mtx);		//devo per forza sbloccarla dopo sennò deadlock
		return 0;
	}
}

int thread(void *arg) {
	int id = *(int *)arg;
	int tempo_attesa = rand()%5; //attesa casuale

	printf("Thread %d: inizio, aspetterò per %d secondi\n", id, (tempo_attesa+1));
	thrd_sleep(&(struct timespec){.tv_sec=(tempo_attesa+1)}, NULL); //simulo calcolo random
	barrier(); //non libreria è funzione sopra
	printf("Thread %d: fine\n", id);
	return 0;
}

int main() {
	thrd_t threads[N];
	mtx_init(&mtx, mtx_plain); //per accedere alla variabile che dice quanti sono arrivati serve mutex
	cnd_init(&cnd); //ad ogni condition variable si accede in mutua-esclusione

	int ids[N];

	for (int i = 0; i < N; i++) {
		ids[i] = i;
		thrd_create(&threads[i], thread, (int *)&ids[i]);
	}
	for (int i = 0; i < N; i++) {
		thrd_join(threads[i], NULL);
	}
	cnd_destroy(&cnd);
	mtx_destroy(&mtx);
	return 0;
}