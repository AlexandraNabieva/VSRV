#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define PHILOSOPHERS_COUNT 5

pthread_mutex_t forks[PHILOSOPHERS_COUNT];
pthread_t philosophers[PHILOSOPHERS_COUNT];
int philosopher_ids[PHILOSOPHERS_COUNT];

void* philosopher(void* arg) {
    int id = *(int*)arg;
    int left_fork = id;
    int right_fork = (id + 1) % PHILOSOPHERS_COUNT;
    
    int first_fork, second_fork;
    if (id % 2 == 0) {
        first_fork = left_fork;
        second_fork = right_fork;
    } else {
        first_fork = right_fork;
        second_fork = left_fork;
    }
    
    while (1) {
        printf("Философ %d размышляет...\n", id);
        usleep(rand() % 1000000 + 500000);
        
        printf("Философ %d хочет есть\n", id);
        
        pthread_mutex_lock(&forks[first_fork]);
        printf("Философ %d взял вилку %d\n", id, first_fork);
        
        pthread_mutex_lock(&forks[second_fork]);
        printf("Философ %d взял вилку %d и начинает есть\n", id, second_fork);
        
        usleep(rand() % 1000000 + 500000);
        printf("Философ %d поел и кладет вилки\n", id);
        
        pthread_mutex_unlock(&forks[second_fork]);
        pthread_mutex_unlock(&forks[first_fork]);
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
    
    return 0;
}