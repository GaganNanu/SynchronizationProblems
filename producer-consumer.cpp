#include <iostream>
#include <pthread.h>
#include <semaphore.h>
int p = 20;
int BUFFER_SIZE = 10;
sem_t full;
sem_t empty;
sem_t s;
int global = 0;
int buffer[10], in = 0, out = 0;

void *Producer(void *args1)
{
    while (global< 20 )
    {
        int itemp = global;
        sem_wait(&empty);
        sem_wait(&s);

        buffer[in] = itemp;

        in = (in + 1) % BUFFER_SIZE;

        std::cout << "In: " << in << std::endl;

        std::cout << "Produced: " << itemp << std::endl;
        global++;
        sem_post(&s);
        sem_post(&full);
    }
}

void *Consumer(void *args1)
{
    while (true)
    {
        int* consumerId = (int *)args1;
        sem_wait(&full);

        sem_wait(&s);

        int c = buffer[out];

        std::cout << "Consumer " << *consumerId << " consumed: " << c << std::endl;

        out = (out + 1) % BUFFER_SIZE;
        std::cout << "Out: " << out << std::endl;

        sem_post(&s);

        sem_post(&empty);
    }
}

int main()
{

    sem_init(&full, 0, 0);

    sem_init(&empty, 0, BUFFER_SIZE);

    sem_init(&s, 0, 1);

    pthread_t producer;
    pthread_t consumer1;
    pthread_t consumer2; 
    pthread_create(&producer, NULL, &Producer, NULL);
    int one = 1;
    int two = 2;
    pthread_create(&consumer1, NULL, &Consumer, (void *) &one);
    pthread_create(&consumer2, NULL, &Consumer, (void *) &two);

    pthread_join(producer, NULL);
    pthread_join(consumer1, NULL);
    pthread_join(consumer2, NULL);

}