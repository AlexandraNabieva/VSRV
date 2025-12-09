#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define PHILOSOPHERS_COUNT 5

pthread_mutex_t forks[PHILOSOPHERS_COUNT];
pthread_mutex_t forks_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t forks_cond = PTHREAD_COND_INITIALIZER;
pthread_t philosophers[PHILOSOPHERS_COUNT];
int philosopher_ids[PHILOSOPHERS_COUNT];

void* philosopher(void* arg) {
    int id = *(int*)arg;
    int left_fork = id;
    int right_fork = (id + 1) % PHILOSOPHERS_COUNT;
    
    while (1) {
        printf("Философ %d размышляет...\n", id);
        usleep(rand() % 1000000 + 500000);
        
        printf("Философ %d хочет есть\n", id);
        
        pthread_mutex_lock(&forks_mutex);
        
        int success = 0;
        while (!success) {
            if (pthread_mutex_trylock(&forks[left_fork]) == 0) {
                if (pthread_mutex_trylock(&forks[right_fork]) == 0) {
                    success = 1;
                    printf("Философ %d взял обе вилки и начинает есть\n", id);
                } else {
                    pthread_mutex_unlock(&forks[left_fork]);
                }
            }
            
            if (!success) {
                pthread_cond_wait(&forks_cond, &forks_mutex);
            }
        }
        
        pthread_mutex_unlock(&forks_mutex);
        
        usleep(rand() % 1000000 + 500000);
        printf("Философ %d поел и кладет вилки\n", id);
        
        pthread_mutex_lock(&forks_mutex);
        pthread_mutex_unlock(&forks[left_fork]);
        pthread_mutex_unlock(&forks[right_fork]);
        pthread_cond_broadcast(&forks_cond);
        pthread_mutex_unlock(&forks_mutex);
    }
    return NULL;
}

int main() {
    srand(time(NULL));
    
    for (int i = 0; i < PHILOSOPHERS_COUNT; i++) {
        pthread_mutex_init(&forks[i], NULL);
    }
    
    for (int i = 0; i < PHILOSOPHERS_COUNT; i++) {
        philosopher_ids[i] = i;
        pthread_create(&philosophers[i], NULL, philosopher, &philosopher_ids[i]);
    }
    
    for (int i = 0; i < PHILOSOPHERS_COUNT; i++) {
        pthread_join(philosophers[i], NULL);
    }
    for (int i = 0; i < PHILOSOPHERS_COUNT; i++) {
        pthread_mutex_destroy(&forks[i]);
    }
    pthread_mutex_destroy(&forks_mutex);
    pthread_cond_destroy(&forks_cond);
    return 0;
}