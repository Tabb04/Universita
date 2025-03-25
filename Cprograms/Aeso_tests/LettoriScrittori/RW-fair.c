#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

static int  N;
static int  stop;
static unsigned long t0; // tempo iniziale

static inline unsigned long getusec() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (unsigned long)(tv.tv_sec*1e6+tv.tv_usec);
}

// variabile di mutua esclusione per la sezione critica
static pthread_mutex_t mutex    = PTHREAD_MUTEX_INITIALIZER;
// determina l'ordine sia per i lettori che per gli scrittori
// NOTA: l'ordine di ingresso non e' detto sia FIFO!
static pthread_mutex_t ordering = PTHREAD_MUTEX_INITIALIZER;
// variabile di condizione dove si sospendono lettori e scrittori
static pthread_cond_t  Go       = PTHREAD_COND_INITIALIZER;

static int activeReaders = 0;  // numero di lettori in sezione critica
static int activeWriter  = 0;  // indica se c'e' uno scrittore in sez. critica


void startRead() {
  pthread_mutex_lock(&ordering);  // coda di ingresso
  
  pthread_mutex_lock(&mutex);
  // se c'e' uno scrittore aspetto. Gli altri lettori/scrittori che arrivano
  // nel frattempo si bloccheranno su ordering che non viene rilasciata
  while(activeWriter > 0) {    
    pthread_cond_wait(&Go, &mutex);
  }
  activeReaders++;
  pthread_mutex_unlock(&mutex);
  pthread_mutex_unlock(&ordering); // faccio passare il prossimo se in coda
}
void doneRead() {
  pthread_mutex_lock(&mutex);
  activeReaders--;
  assert(activeReaders>=0);
  if (activeReaders==0) {      // sono l'ultimo lettore
    pthread_cond_signal(&Go);  // sveglio uno scrittore se in attesa
  } 
  pthread_mutex_unlock(&mutex);
}

void startWrite() {
  pthread_mutex_lock(&ordering);    // coda di ingresso
  pthread_mutex_lock(&mutex);
  // se ci sono lettori o scrittori aspetto. Gli altri lettori/scrittori che
  // arrivano nel frattempo si bloccheranno su ordering dato che non viene rilasciata
  while (activeReaders > 0 || activeWriter > 0) {  
    pthread_cond_wait(&Go, &mutex);
  }
  assert(activeWriter==0);
  activeWriter++;
  pthread_mutex_unlock(&mutex);
  pthread_mutex_unlock(&ordering); // faccio passare il prossimo se in attesa
}
void doneWriter() {
  pthread_mutex_lock(&mutex);
  activeWriter--;
  pthread_cond_signal(&Go); // sveglio il prossimo (lettore/scrittore) in attesa
  pthread_mutex_unlock(&mutex);
}

void work(long us) {
  struct timespec t={0, us*1000};
  nanosleep(&t, NULL);
}

void *Reader(void* arg) {
  long id = (long)arg;
  
  while(stop>0) {
    startRead();
    
    printf("READER%ld ENTRATO in sezione critica t=%.2f\n",id, (getusec()-t0)/1000.0);
    work(2000);
    printf("READER%ld USCITO dalla sezione critica\n",id);
    
    doneRead();
  }
  printf("READER%ld TERMINATO\n", id);  
  return NULL;
}

void *Writer(void* arg) {
  long id = (long)arg;
  
  for(int i=0;i<N;++i) {
    startWrite();

    printf("WRITER%ld ENTRATO in sezione critica t=%.2f\n",id, (getusec()-t0)/1000.0);
    work(6000);
    printf("WRITER%ld USCITO dalla sezione critica\n",id);

    if (i+1 == N) --stop;
    
    doneWriter();
  }

  printf("WRITER%ld TERMINATO\n", id);  
  return NULL;
}

int main(int argc, char* argv[]) {
  int R = 4;
  int W = 2;
  N = 100;
  if (argc > 1) {
    if (argc != 4) {
      fprintf(stderr, "usage: %s [#R #W N]\n", argv[0]);
      return -1;
    }
    R = atoi(argv[1]);
    W = atoi(argv[2]);
    N = atoi(argv[3]);
  }  
  // a reasonable limit 
  if (R>100) R = 100;
  if (W>100) W = 100;

  stop = W;
  t0   = getusec();
  
  pthread_t* readers     = malloc(R*sizeof(pthread_t));
  pthread_t* writers     = malloc(W*sizeof(pthread_t));
  if (!readers || !writers) {
    fprintf(stderr, "not enough memory\n");
    return -1;
  }
  
  for(long i=0; i<W; ++i)
    if (pthread_create(&writers[i], NULL, Writer, (void*)i) != 0) {
      fprintf(stderr, "pthread_create Writer failed\n");
      return -1;
    }
  for(long i=0; i<R; ++i)
    if (pthread_create(&readers[i], NULL, Reader, (void*)i) != 0) {
      fprintf(stderr, "pthread_create Reader failed\n");
      return -1;
    }		
  for(long i=0;i<R; ++i)
    if (pthread_join(readers[i], NULL) == -1) {
      fprintf(stderr, "pthread_join failed\n");
    }
  for(long i=0;i<W; ++i)
    if (pthread_join(writers[i], NULL) == -1) {
      fprintf(stderr, "pthread_join failed\n");
    }
  
  free(writers);
  free(readers);
  return 0;   
  
}
