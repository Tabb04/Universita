//cuochi che vogliono cucinare ma hanno come risorsa condivisa il fornello
//fornello deve essere protetta da una lock
//mtx_lock(&fornello);
//un cuoco sta tot secondi su un fonello e poi lo libera

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>

#define NUM_CUOCHI 5

mtx_t fornello;

int cucinare(void * arg){
    int id = *(int*)arg;

    printf("👨‍🍳 Cuoco %d è pronto a cucinare!\n", id);
    mtx_lock(&fornello);
    printf("🔥 Lo chef %d sta usando il fornello!\n", id);
    
    //simulo lavoro
    thrd_sleep(&(struct timespec){.tv_sec=(1)}, NULL);
    printf("✅ Lo chef %d ha finito di cucinare!\n", id);
    mtx_unlock(&fornello);
    return 0;
}



int main(){
    srand(time(NULL));

    thrd_t threads[NUM_CUOCHI];
    int ids[NUM_CUOCHI];

    //mutex va sempre inizializzata
    mtx_init(&fornello, mtx_plain);

    for (int i = 0; i<NUM_CUOCHI; i++){
        ids[i] = i+1;
        thrd_create(&threads[i], cucinare, &ids[i]);
    }

    for (int i = 0; i<NUM_CUOCHI; i++){
        thrd_join(threads[i], NULL);
    }

    printf("🍽 Tutti i cuochi hanno finito di cucinare!\n");

    mtx_destroy(&fornello);
    return 0;
}