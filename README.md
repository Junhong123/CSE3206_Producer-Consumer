# Producer-Consumer Problem
### Overview
Producer-Consumer Problem을 직접 해결해보며 느낀점과 평가를 내린다.
### 개발 환경
Ubuntu 24.04.1 LTS  
gcc 13.3.0  
WSL 2.6.3.0  
### Problem
---
Producer-Consumer Problem은 다수의 Thread가 하나의 공유 자원을 동시에 접근하려고 할 때 생기는 문제이다.  
```
#include <pthread.h>
#include <stdio.h>
#define ITER 1000000

void *thread_increment(void *arg);
void *thread_decrement(void *arg);
int x;

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
        val = x;
        printf("%u: %d\n", (unsigned int)pthread_self(), val);
        x = val + 1;
    }
    return NULL;
}
void *thread_decrement(void *arg)
{
    int i, val;
    for (i = 0; i < ITER; i++)
    {
        val = x;
        printf("%u: %d\n", (unsigned int)pthread_self(), val);
        x = val - 1;
    }
    return NULL;
}
```
이 예시 코드에서는 스레드 2개가 전역 변수인 x를 동시에 접근하여 문제가 발생한다.  
### Solution
---
**1. Condition Variables**  
첫번째 방법으로는 Condition Variables를 사용하는 방법이 있다.  
우선 공유 자원을 동시에 접근하는 것을 막기 위해 상호 배제를 시켜준 후  
Bounded Buffer Problem의 조건을 만족하기 위해 Condition Variables을 사용한다.
```
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
        printf("%u: %d\n", (unsigned int)pthread_self(), val);
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
        printf("%u: %d\n", (unsigned int)pthread_self(), val);
        x = val - 1;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&m);
    }
    return NULL;
}
```
우선 전역 변수 x에 대해 상호배제를 시켜주기 위해 pthread_mutex_lock으로 lock을 잡고 들어간다.  
조건이 x는 무조건 양수여야만 하기 때문에 x가 0이면 --하지 않고 cond_wait를 통해 go to sleep 한다.
또한 버퍼의 MAX 값이 30이므로 x가 30이면 ++하지 않고 cond_wait를 통해 go to sleep 한다.  
중요한 점은 decrement 스레드를 깨워주는 cond_signal은 increment 스레드에 있고  
increment 스레드를 깨워주는 cond_signal을 decrement 스레드에 있다는 점이다.  

**2. Semaphore**  
두번째 방법은 Semaphore를 사용하는 방법이다.  
Semaphore는 Mutual Exclusion과 Condition Variables 둘 다 커버 가능 하다.
```
int main()
{
    sem_init(&m, 0, 1);
    sem_init(&empty, 0, 30);
    sem_init(&fill, 0, 0);

    ```
    sem_destroy(&m);
    sem_destroy(&empty);
    sem_destroy(&fill);
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
```
우선 상호배제를 위해 Binary Semaphore를 만들어 상호배제를 해준다.  
x가 0일 때 wait 해야되기 때문에 decrement 스레드에 들어갈 fill은 semaphore 값을 0으로 설정한다.  
x가 30일 때 wait 해야되기 때문에 increment 스레드에 들어갈 empty는 semaphore 값을 30으로 설정한다.  
Condition Variables와 같이 sem_post에서 다른 스레드를 풀어줘야 한다.  
또한 상호 배제 semaphore를 안쪽으로 넣어야 된다.
### Implementation Result
두 코드 모두 실행시켜보니 출력되는 x값이 양수만 나오면서 최대 30을 넘지 않는 것을 확인할 수 있다.  



### Evaluation
---
위에서 정말 Bounded Buffer Problem을 만족 하는지는 확인 했다. 하지만 어떤 방법이 더 좋은 방법일까?  
우선 프로그램 실행에 걸리는 시간을 측정 해 보았다.  
측정 방법은 linux에 있는 time 명령어를 사용하여 측정 했다.  
예상으로는 문제를 해결 안한 코드가 가장 빠르고, 그 다음이 CV, Semaphore로 해결한 코드가 가장 느릴 것 같다.  
Semaphore가 가장 느릴 것이라 생각한 이유는 계속해서 sem_wait과 sem_post를 호출하기 때문이다.  

> 이제부터 아무 처리도 안한 코드를 PC, CV로 구현한 코드를 PC_CV, Semaphore로 구현한 코드를 PC_S라 하겠다.

|  | T1 | T2 | T3 | T4 | T5 | AVG |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| PC | real : 6.285s <br> user : 0.447s <br> sys : 2.238s | real : 6.103s <br> user : 0.449s <br> sys : 2.282s | real : 5.533s <br> user : 0.410s <br> sys : 2.226s | real : 5.844s <br> user : 0.461s <br> sys : 2.278s | real : 6.425s <br> user : 0.370s <br> sys : 2.382s | real : 6.038s <br> user : 0.427s <br> sys : 2.281s |
| PC_CV | real : 4.850s <br> user : 0.603s <br> sys : 2.298s | real : 4.813s <br> user : 0.593s <br> sys : 2.289s | real : 4.522s <br> user : 0.482s <br> sys : 2.440s | real : 4.804s <br> user : 0.596s <br> sys : 2.318s | real : 4.823s <br> user : 0.543s <br> sys : 2.408s | real : 4.762s <br> user : 0.563s <br> sys : 2.351s |
| PC_S | real : 4.949s <br> user : 0.609s <br> sys : 2.744s | real : 4.900s <br> user : 0.738s <br> sys : 2.527s | real : 4.642s <br> user :0.743s <br> sys : 2.605s | real : 4.879s <br> user : 0.679s <br> sys : 2.604s | real : 4.947s <br> user : 0.746s <br> sys : 2.596s | real : 4.863s <br> user : 0.703s <br> sys : 2.615s |

결과는 real 기준으로 PC가 가장 느리고 그 다음 PC_S, 가장 빠른 것은 CV로 구현 한 PC_CV이다.  
CV와 semaphore 모두 원래의 코드에 제약을 걸어 Bounded Buffer Problem을 만족시키는 것이여서   
아무런 제약을 걸지 않은 PC가 가장 빠를 줄 알았지만 아니었다.  
그런데 real이 아닌 user + sys 기준으로는 PC -> PC_CV -> PC_S 순으로 빠른 것을 볼 수 있다.  
또한 프로그램을 실행 할 때 화면에 출력되는 숫자의 값이 CV와 S는 제한 했기 때문에 0~30 이지만  
PC는 제한 하지 않아 비교적 큰 숫자가 화면에 표시되느라 오래 걸리는 것 같은 느낌을 받았다.  
그래서 printf를 하지 않고 마지막에 결과만 출력하는 코드로 바꿔서 시간 측정을 해 보았다.  
printf를 하지 않아 더 빨리 끝날 것을 생각해 Iteration도 백만에서 천만으로 증가시켰다.  
| | T1 | T2 | T3 | T4 | T5 | AVG |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| PC | real : 0.036s <br> user : 0.058s <br> sys : 0.001s | real : 0.034s <br> user : 0.060s <br> sys : 0.001s | real : 0.040s <br> user : 0.071s <br> sys : 0.002s | real : 0.041s <br> user : 0.068s <br> sys : 0.004s | real : 0.037s <br> user : 0.054s <br> sys : 0.002s | real : 0.0376s <br> user : 0.0622s <br> sys : 0.002s |
| PC_CV | real : 9.151s <br> user : 0.864s <br> sys : 2.739s | real : 8.964s <br> user : 0.947s <br> sys : 2.661s | real : 8.864s <br> user : 0.986s <br> sys : 2.584s | real : 8.986s <br> user : 1.060s <br> sys : 2.565s | real : 9.628s <br> user : 0.968s <br> sys : 2.653s | real : 9.119s <br> user : 0.965s <br> sys : 2.640s |
| PC_S | real : 8.667s <br> user : 1.590s <br> sys : 3.290s | real : 8.605s <br> user : 1.512s <br> sys : 3.339s | real : 8.880s <br> user : 1.552s <br> sys : 3.357s | real : 8.596s <br> user : 1.528s <br> sys : 3.338s | real : 8.643s <br> user : 1.604s <br> sys : 3.299s | real : 8.678s <br> user : 1.557s <br> sys : 3.325s |

결과는 real 기준으로 PC가 압도적으로 빠르고 그 다음이 PC_S, PC_CV가 가장 느리다.  
printf로 실행 과정을 출력하는 코드를 빼니 PC가 압도적으로 빠르게 끝났다.  
lock하고 unlock 하는 오버헤드가 없어서 그런 것 같다.  
재밌는 점은 PC에서 user + sys가 real보다 크게 나왔다는 점인데 찾아보니 스레드 개수에 따라 곱해서 출력 된다고 한다.  
CV로 구현한 것이 Semaphore로 구현한 것 보다 느린 것을 확인할 수 있는데 내 예측과는 다른 결과가 나왔다.  
user + sys만 봤을 때는 PC_CV가 PC_S보다 빠르지만 어디서 뭘 하길래 늦게 끝나는지 모르겠다.  
### 결론 및 느낀점
---
real 기준으로 printf가 있을 때 PC_CV -> PC_S -> PC 순으로 빠르다.  
real 기준으로 printf가 없을 때 PC -> PC_S -> PC_CV 순으로 빠르다.  
직접 여러가지 방법을 적용해보고 또 실행시켜 보면서 컴퓨터를 예측하는 것은 어렵다고 느꼈다.