// reusable_barrier.c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

typedef struct {
    pthread_mutex_t m;
    pthread_cond_t  cv;
    int count;      // threads currently waiting in this generation
    int trip;       // number of threads required to release barrier
    unsigned gen;   // generation counter to make barrier reusable
} barrier_t;

int barrier_init(barrier_t *b, int n) {
    if (n <= 0) return -1;
    b->count = 0;
    b->trip  = n;
    b->gen   = 0;
    pthread_mutex_init(&b->m, NULL);
    pthread_cond_init(&b->cv, NULL);
    return 0;
}

void barrier_destroy(barrier_t *b) {
    pthread_mutex_destroy(&b->m);
    pthread_cond_destroy(&b->cv);
}

// Returns the generation number a thread passed, useful for debugging
unsigned barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->m);
    unsigned mygen = b->gen;

    if (++b->count == b->trip) {
        // Last thread: advance generation, reset count, wake everyone
        b->gen++;
        b->count = 0;
        pthread_cond_broadcast(&b->cv);
        pthread_mutex_unlock(&b->m);
        return mygen;
    }

    // Wait until generation changes (handles spurious wakeups, re-use)
    while (mygen == b->gen) {
        pthread_cond_wait(&b->cv, &b->m);
    }
    pthread_mutex_unlock(&b->m);
    return mygen;
}

// ---------------- Demo ----------------

#define NUM_THREADS 4
#define ROUNDS 3

barrier_t barrier;

void* worker(void* arg) {
    int id = *(int*)arg;

    for (int r = 1; r <= ROUNDS; r++) {
        // Pretend to do some work before the barrier
        usleep((rand() % 500) * 1000); // 0–500ms
        printf("Thread %d: round %d — before barrier\n", id, r);

        unsigned g = barrier_wait(&barrier); // synchronize here

        // All threads print after barrier; same generation has completed
        printf("Thread %d: round %d — after barrier (gen %u)\n", id, r, g + 1);
    }
    return NULL;
}

int main(void) {
    srand((unsigned)time(NULL));

    if (barrier_init(&barrier, NUM_THREADS) != 0) {
        fprintf(stderr, "barrier_init failed\n");
        return 1;
    }

    pthread_t th[NUM_THREADS];
    int ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&th[i], NULL, worker, &ids[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) pthread_join(th[i], NULL);

    barrier_destroy(&barrier);
    return 0;
}
