#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define CHAIRS_COUNT 5
#define CUSTOMERS_COUNT 10

pthread_mutex_t barber_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t customers_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t barber_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t customer_cond = PTHREAD_COND_INITIALIZER;

int waiting_customers = 0;
int barber_sleeping = 1;
int served_customers = 0;

void* barber(void* arg) {
    while (served_customers < CUSTOMERS_COUNT) {
        pthread_mutex_lock(&barber_mutex);
        
        if (waiting_customers == 0) {
            barber_sleeping = 1;
            printf("Парикмахер спит...\n");
            pthread_cond_wait(&customer_cond, &barber_mutex);
            barber_sleeping = 0;
            printf("Парикмахер проснулся и начинает работу\n");
        }
        
        waiting_customers--;
        served_customers++;
        printf("Парикмахер стрижет клиента. Ожидающих: %d\n", waiting_customers);
        
        pthread_cond_signal(&barber_cond);
        pthread_mutex_unlock(&barber_mutex);
        
        usleep(rand() % 800000 + 500000);
        printf("Парикмахер закончил стрижку\n");
    }
    
    printf("Парикмахер завершил работу дня\n");
    return NULL;
}

void* customer(void* arg) {
    int id = *(int*)arg;
    
    usleep(rand() % 2000000);
    
    pthread_mutex_lock(&customers_mutex);
    
    if (waiting_customers < CHAIRS_COUNT) {
        waiting_customers++;
        printf("Клиент %d пришел в парикмахерскую. Ожидающих: %d\n", id, waiting_customers);
        
        if (barber_sleeping) {
            pthread_cond_signal(&customer_cond);
        }
        
        pthread_mutex_unlock(&customers_mutex);
        
        pthread_mutex_lock(&barber_mutex);
        while (barber_sleeping || waiting_customers > 0) {
            pthread_cond_wait(&barber_cond, &barber_mutex);
        }
        pthread_mutex_unlock(&barber_mutex);
        
        printf("Клиент %d уходит подстриженным\n", id);
    } else {
        pthread_mutex_unlock(&customers_mutex);
        printf("Клиент %d ушел - нет свободных мест\n", id);
    }
    
    return NULL;
}

int main() {
    pthread_t barber_thread;
    pthread_t customer_threads[CUSTOMERS_COUNT];
    int customer_ids[CUSTOMERS_COUNT];
    
    srand(time(NULL));
    
    pthread_create(&barber_thread, NULL, barber, NULL);
    
    for (int i = 0; i < CUSTOMERS_COUNT; i++) {
        customer_ids[i] = i + 1;
        pthread_create(&customer_threads[i], NULL, customer, &customer_ids[i]);
    }
    
    for (int i = 0; i < CUSTOMERS_COUNT; i++) {
        pthread_join(customer_threads[i], NULL);
    }
    
    pthread_join(barber_thread, NULL);
    
    printf("Все клиенты обслужены. Работа завершена.\n");
    
    return 0;
}