#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

// -------------------- Account Structure --------------------
typedef struct account_t {
    pthread_mutex_t lock;
    int balance;
    long uuid;
    int priority;   // current priority (may be boosted temporarily)
} account_t;

// -------------------- Transfer Function with Priority Donation --------------------
void transfer(account_t *donor, account_t *recipient, float amount, int thread_priority) {
    // Order locks to avoid deadlock
    account_t *first  = (donor->uuid < recipient->uuid) ? donor : recipient;
    account_t *second = (donor->uuid < recipient->uuid) ? recipient : donor;

    // Simulate priority donation: if the thread waiting has a higher priority
    // than the thread currently holding the lock, we "donate" the higher priority
    pthread_mutex_lock(&first->lock);
    if (first->priority < thread_priority) {
        printf("[Priority donation] Boosting account %ld from %d → %d\n",
               first->uuid, first->priority, thread_priority);
        first->priority = thread_priority;
    }

    pthread_mutex_lock(&second->lock);
    if (second->priority < thread_priority) {
        printf("[Priority donation] Boosting account %ld from %d → %d\n",
               second->uuid, second->priority, thread_priority);
        second->priority = thread_priority;
    }

    // Perform transaction
    if (donor->balance < amount) {
        printf("Insufficient funds in account %ld.\n", donor->uuid);
    } else {
        donor->balance -= amount;
        recipient->balance += amount;
        printf("Transferred %.2f from account %ld to %ld (Thread priority %d)\n",
               amount, donor->uuid, recipient->uuid, thread_priority);
    }

    // Reset priority after transfer
    second->priority = 0;
    first->priority  = 0;

    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

// -------------------- Thread Function --------------------
void* transfer_thread(void* arg) {
    struct {
        account_t *donor;
        account_t *recipient;
        float amount;
        int priority;
    } *params = arg;

    // Optional: set thread scheduling priority for realism
    struct sched_param sched;
    sched.sched_priority = params->priority;
    pthread_setschedparam(pthread_self(), SCHED_RR, &sched);

    transfer(params->donor, params->recipient, params->amount, params->priority);
    return NULL;
}

// -------------------- Main Function --------------------
int main() {
    account_t acc1 = {PTHREAD_MUTEX_INITIALIZER, 1000, 1, 0};
    account_t acc2 = {PTHREAD_MUTEX_INITIALIZER, 500, 2, 0};

    pthread_t t1, t2;

    // Thread 1 has higher priority, transferring from acc1 → acc2
    pthread_create(&t1, NULL, transfer_thread,
        &(struct {account_t* donor; account_t* recipient; float amount; int priority;})
        {&acc1, &acc2, 200, 10});

    // Thread 2 has lower priority, transferring from acc2 → acc1
    pthread_create(&t2, NULL, transfer_thread,
        &(struct {account_t* donor; account_t* recipient; float amount; int priority;})
        {&acc2, &acc1, 100, 2});

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("\nFinal Balances:\n");
    printf("Account %ld: %d\n", acc1.uuid, acc1.balance);
    printf("Account %ld: %d\n", acc2.uuid, acc2.balance);

    return 0;
}
