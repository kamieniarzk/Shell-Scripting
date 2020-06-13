#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

// EOPSY Lab 7 - The Dining Philosophers Problem using threads
// Kacper Kamieniarz 293065

#define N 5
#define LEFT (i + N - 1) % N
#define RIGHT (i + 1) % N

// global variables
pthread_mutex_t m;            // mutex for critical region
pthread_mutex_t s[N];         // philosophers mutex array (for synchronization)
pthread_t philosopher_tid[N]; // philosophers threads ids
int state[N];                 // philosophers state array

// functions
void grab_forks(int i);
void put_away_forks(int i);
void test(int i);
void *philosopher(void *arg);
void think(int i);
void eat(int i);
int clean_up();

// state enum
enum state
{
    THINKING,
    EATING,
    HUNGRY
};

int main()
{
    int i, philosopher_id[N];
    for (int i = 0; i < N; i++)
    {
        // initializing id's and states of each philosopher
        philosopher_id[i] = i;
        state[i] = THINKING;

        // initializing philosopher's mutexes
        if (pthread_mutex_init(&s[i], NULL) != 0)
        {
            perror("error while initializing mutex");
            exit(1);
        }
        // setting initial state of the mutexes to locked
        pthread_mutex_lock(&s[i]);
    }

    for (int i = 0; i < N; i++)
    {
        // creating philosophers thread and storing the thread it in an array
        if (pthread_create(&philosopher_tid[i], NULL, philosopher, (void *)&philosopher_id[i]) != 0)
        {
            perror("error while creating thread");
            exit(1);
        }
    }

    sleep(40); // program runtime
    return clean_up();
}

void grab_forks(int i)
{
    pthread_mutex_lock(&m);
    state[i] = HUNGRY;
    test(i);
    pthread_mutex_unlock(&m);
    pthread_mutex_lock(&s[i]);
}

void put_away_forks(int i)
{
    pthread_mutex_lock(&m);
    state[i] = THINKING;
    test(LEFT);
    test(RIGHT);
    pthread_mutex_unlock(&m);
}

void test(int i)
{
    if (state[i] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING)
    {
        state[i] = EATING;
        pthread_mutex_unlock(&s[i]);
    }
}

void think(int i)
{
    printf("PHILOSOPHER[%d]: THINKING\n", i);
    sleep(3); // time before a philosopher tries to grab forks
}

void eat(int i)
{
    printf("PHILOSOPHER[%d]: EATING\n", i);
    sleep(3); // meal time
}

void *philosopher(void *arg)
{
    // asynchronous type of cancel has to be set to each thread so that
    // at the end of the program we can cancel them instantly
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0)
    {
        perror("error whyle setting cancel type to asynchronous");
        exit(EXIT_FAILURE);
    }

    int i = *((int *)arg); // the argument type in the function pthread_create is void* so it needs to be casted to int

    while (true)
    {
        think(i);
        grab_forks(i);
        eat(i);
        put_away_forks(i);
    }

    return NULL;
}

int clean_up()
{
    for (int i = 0; i < N; i++)
    {
        // sending a cancellation request to a thread and waiting for it to terminate
        pthread_cancel(philosopher_tid[i]);
        if (pthread_join(philosopher_tid[i], NULL) != 0)
        {
            perror("error in clean_up()");
            return 1;
        }
    }
    // destroying mutexes
    pthread_mutex_destroy(&m);
    for (int i = 0; i < N; i++)
    {
        // unlocking the mutexes before destroying them
        pthread_mutex_unlock(&s[i]);
        if (pthread_mutex_destroy(&s[i]) != 0)
        {
            perror("error in clean_up()");
            return 1;
        }
    }

    return 0;
}
