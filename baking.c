#include <pthread.h>
#include <stdbool.h>
#include <unistd.h> 

int numBatterInBowl = 0;
int numEggInBowl = 0;
bool readyToEat = false;
pthread_mutex_t lock;
pthread_cond_t needIngredients, readyToBake, startEating;

void addBatter() { numBatterInBowl += 1; }
void addEgg() { numEggInBowl += 1; }
void heatBowl() { readyToEat = true; numBatterInBowl = 0; numEggInBowl = 0; }
void eatCake() { readyToEat = false; }

void* batterAdder(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        while (numBatterInBowl >= 1 || readyToEat) {
            pthread_cond_wait(&needIngredients, &lock);
        }
        addBatter();
        /* wake heater (use broadcast to avoid lost wakeups if multiple signals needed) */
        pthread_cond_broadcast(&readyToBake);
        pthread_mutex_unlock(&lock);
        /* yield so other threads can run */
        usleep(1000);
    }
}

void* eggBreaker(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        while (numEggInBowl >= 2 || readyToEat) {
            pthread_cond_wait(&needIngredients, &lock);
        }
        addEgg();
        pthread_cond_broadcast(&readyToBake);
        pthread_mutex_unlock(&lock);
        usleep(1000);
    }
}

void* bowlHeater(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        while (numBatterInBowl < 1 || numEggInBowl < 2 || readyToEat) {
            pthread_cond_wait(&readyToBake, &lock);
        }
        heatBowl();
        pthread_cond_signal(&startEating);
        pthread_mutex_unlock(&lock);
        usleep(1000);
    }
}

void* cakeEater(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        while (!readyToEat) {
            pthread_cond_wait(&startEating, &lock);
        }
        eatCake();
        /* wake all ingredient threads */
        pthread_cond_broadcast(&needIngredients);
        pthread_mutex_unlock(&lock);
        usleep(1000);
    }
}

int main(int argc, char* argv[]) {
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&needIngredients, NULL);
    pthread_cond_init(&readyToBake, NULL);
    pthread_cond_init(&startEating, NULL);

    pthread_t batter, egg1, egg2, heater, eater;
    pthread_create(&batter, NULL, batterAdder, NULL);
    pthread_create(&egg1, NULL, eggBreaker, NULL);
    pthread_create(&egg2, NULL, eggBreaker, NULL);
    pthread_create(&heater, NULL, bowlHeater, NULL);
    pthread_create(&eater, NULL, cakeEater, NULL);

    while (1) sleep(1); // Sleep forever

    return 0;
}

