#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#define ITER 1000000

void *thread_increment(void *arg);
void *thread_decrement(void *arg);
int x;
sem_t m, empty, fill;

int main()
{
    sem_init(&m, 0, 1);
    sem_init(&empty, 0, 30);
    sem_init(&fill, 0, 0);
    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, thread_increment, NULL);
    pthread_create(&tid2, NULL, thread_decrement, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    if (x != 0)
        printf("BOOM! counter=%d\n", x);
    else
        printf("OK counter=%d\n", x);
    sem_destroy(&m);
    sem_destroy(&empty);
    sem_destroy(&fill);

    return 0;
}
/* thread routine */
void *thread_increment(void *arg)
{
    int i, val;
    for (i = 0; i < ITER; i++)
    {
        sem_wait(&empty);
        sem_wait(&m);
        val = x;
        printf("%u: %d\n", (unsigned int)pthread_self(), val);
        x = val + 1;
        sem_post(&m);
        sem_post(&fill);
    }
    return NULL;
}
void *thread_decrement(void *arg)
{
    int i, val;
    for (i = 0; i < ITER; i++)
    {
        sem_wait(&fill);
        sem_wait(&m);
        val = x;
        printf("%u: %d\n", (unsigned int)pthread_self(), val);
        x = val - 1;
        sem_post(&m);
        sem_post(&empty);
    }
    return NULL;
}
