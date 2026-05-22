#include <pthread.h>
#include <stdio.h>
#define ITER 10000000

void *thread_increment(void *arg);
void *thread_decrement(void *arg);
int x;
pthread_mutex_t m;
pthread_cond_t fill, empty;

int main()
{
    pthread_t tid1, tid2;
    pthread_create(&tid1, NULL, thread_increment, NULL);
    pthread_create(&tid2, NULL, thread_decrement, NULL);
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    if (x != 0)
        printf("BOOM! counter=%d\n", x);
    else
        printf("OK counter=%d\n", x);

    return 0;
}
/* thread routine */
void *thread_increment(void *arg)
{
    int i, val;
    for (i = 0; i < ITER; i++)
    {
        pthread_mutex_lock(&m);
        while (x == 30)
            pthread_cond_wait(&empty, &m);
        val = x;
        x = val + 1;
        pthread_cond_signal(&fill);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}
void *thread_decrement(void *arg)
{
    int i, val;
    for (i = 0; i < ITER; i++)
    {
        pthread_mutex_lock(&m);
        while (x == 0)
            pthread_cond_wait(&fill, &m);
        val = x;
        x = val - 1;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}