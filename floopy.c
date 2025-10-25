#include <pthread.h>
#include <stdio.h>

typedef struct {
    pthread_mutex_t lock;
    int balance;
    long uuid;
} account_t;

typedef struct {
    account_t *donor;
    account_t *recipient;
    float amount;
} transfer_args;

static void transfer(account_t *donor, account_t *recipient, float amount) {
    // Order locks deterministically by uuid to avoid deadlock
    account_t *first  = (donor->uuid < recipient->uuid) ? donor     : recipient;
    account_t *second = (donor->uuid < recipient->uuid) ? recipient : donor;

    pthread_mutex_lock(&first->lock);
    pthread_mutex_lock(&second->lock);

    if (donor->balance < amount) {
        printf("Insufficient funds.\n");
    } else {
        donor->balance     -= (int)amount;
        recipient->balance += (int)amount;
        printf("Transferred %.2f from %ld to %ld (balances: %d, %d)\n",
               amount, donor->uuid, recipient->uuid, donor->balance, recipient->balance);
    }

    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

static void* transfer_thread(void *arg) {
    transfer_args *a = (transfer_args*)arg;
    transfer(a->donor, a->recipient, a->amount);
    return NULL;
}

int main(void) {
    account_t acc1 = { .lock = PTHREAD_MUTEX_INITIALIZER, .balance = 1000, .uuid = 1 };
    account_t acc2 = { .lock = PTHREAD_MUTEX_INITIALIZER, .balance = 500,  .uuid = 2 };

    transfer_args a1 = { .donor = &acc1, .recipient = &acc2, .amount = 200.0f };
    transfer_args a2 = { .donor = &acc2, .recipient = &acc1, .amount = 100.0f };

    pthread_t t1, t2;
    pthread_create(&t1, NULL, transfer_thread, &a1);
    pthread_create(&t2, NULL, transfer_thread, &a2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // If you didn’t use PTHREAD_MUTEX_INITIALIZER, you’d call pthread_mutex_destroy here.
    return 0;
}
