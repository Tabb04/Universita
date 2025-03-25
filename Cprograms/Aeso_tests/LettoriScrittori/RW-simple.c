// NON EVITA ATTESA INFINITA PER GLI SCRITTORI (Writer Starving):
//
// - se ci sono lettori entrano tutti
// - appena uno scrittore finisce fa passare tutti i lettori accodati quindi
//   eventuali scrittori in attesa potrebbero non essere mai svegliati
//   perche' i lettori hanno sempre la precedenza.
// - se non ci sono lettori (quando l'ultimo dei lettori esce) puo'
//   entrare uno scrittore in attesa
//
// Questo programma e' costruito in modo tale che i lettori stiano sempre
// in sezione critica e gli scrittori non riescono ad entrare.
// 

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

static pthread_mutex_t mutex    = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  readerGo = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  writerGo = PTHREAD_COND_INITIALIZER;
static int activeReaders = 0, activeWriters = 0;
static int waitingReaders = 0, waitingWriters = 0;

void startRead() {
  pthread_mutex_lock(&mutex);
  waitingReaders++;
  while(activeWriters > 0) {
    pthread_cond_wait(&readerGo, &mutex);
  }
  waitingReaders--;
  activeReaders++;
  pthread_mutex_unlock(&mutex);
}
void doneRead() {
  pthread_mutex_lock(&mutex);
  activeReaders--;
  if (activeReaders == 0 && waitingWriters > 0) {  
    pthread_cond_signal(&writerGo);
  }
  pthread_mutex_unlock(&mutex);
}

void startWrite() {
  pthread_mutex_lock(&mutex);
  waitingWriters++;
  while(activeWriters > 0 || activeReaders > 0) {
    pthread_cond_wait(&writerGo, &mutex);
  }
  waitingWriters--;
  activeWriters++;
  pthread_mutex_unlock(&mutex);
}
void doneWriter() {
  pthread_mutex_lock(&mutex);
  activeWriters--;
  assert(activeWriters==0);

  if (waitingReaders > 0) {      // passano sempre prima i lettori
    pthread_cond_broadcast(&readerGo);
  } else {
    pthread_cond_signal(&writerGo);  // se c'e' uno scrittore lo sveglio
  }

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
