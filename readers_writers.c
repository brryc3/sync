#include <pthread.h>
#include <stdio.h>

pthread_mutex_t lock;
pthread_cond_t reader_cv, writer_cv;
int reader_count = 0, writer_waiting = 0;
int shared_data = 0;

void* reader(void* arg) {
    pthread_mutex_lock(&lock);
    while (writer_waiting > 0) {
        pthread_cond_wait(&reader_cv, &lock);
    }
    reader_count++;
    pthread_mutex_unlock(&lock);

    printf("Reader reads: %d\n", shared_data);

    pthread_mutex_lock(&lock);
    reader_count--;
    if (reader_count == 0) {
        pthread_cond_signal(&writer_cv);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

void* writer(void* arg) {
    pthread_mutex_lock(&lock);
    writer_waiting++;
    while (reader_count > 0) {
        pthread_cond_wait(&writer_cv, &lock);
    }
    writer_waiting--;

    shared_data++;
    printf("Writer writes: %d\n", shared_data);

    pthread_cond_broadcast(&reader_cv);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main() {
    pthread_t readers[3], writers[2];
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&reader_cv, NULL);
    pthread_cond_init(&writer_cv, NULL);

    for (int i = 0; i < 3; i++)
        pthread_create(&readers[i], NULL, reader, NULL);
    for (int i = 0; i < 2; i++)
        pthread_create(&writers[i], NULL, writer, NULL);

    for (int i = 0; i < 3; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < 2; i++)
        pthread_join(writers[i], NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&reader_cv);
    pthread_cond_destroy(&writer_cv);
    return 0;
}
