#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define PRODUCTS_TO_MAKE 3

sem_t sem_part_A;
sem_t sem_part_B;
sem_t sem_part_C;
sem_t sem_module;

void* worker_A(void* arg) {
    for (int i = 0; i < PRODUCTS_TO_MAKE; i++) {
        printf("[Цех A] Начало производства детали A (1 сек)...\n");
        sleep(1);
        printf("[Цех A] Деталь A готова.\n");
        sem_post(&sem_part_A);
    }
    return NULL;
}

void* worker_B(void* arg) {
    for (int i = 0; i < PRODUCTS_TO_MAKE; i++) {
        printf("[Цех B] Начало производства детали B (2 сек)...\n");
        sleep(2);
        printf("[Цех B] Деталь B готова.\n");
        sem_post(&sem_part_B);
    }
    return NULL;
}

void* worker_C(void* arg) {
    for (int i = 0; i < PRODUCTS_TO_MAKE; i++) {
        printf("[Цех C] Начало производства детали C (3 сек)...\n");
        sleep(3);
        printf("[Цех C] Деталь C готова.\n");
        sem_post(&sem_part_C);
    }
    return NULL;
}

void* worker_Module(void* arg) {
    for (int i = 0; i < PRODUCTS_TO_MAKE; i++) {
        sem_wait(&sem_part_A);
        sem_wait(&sem_part_B);
        
        printf(" -> [Сборка Модуля] Детали A и B получены. Сборка модуля...\n");
        printf(" -> [Сборка Модуля] Модуль готов.\n");
        
        sem_post(&sem_module);
    }
    return NULL;
}

void* worker_Final(void* arg) {
    for (int i = 0; i < PRODUCTS_TO_MAKE; i++) {
        sem_wait(&sem_module);
        sem_wait(&sem_part_C);
        
        printf(" === [ФИНАЛ] Изделие %d собрано (Модуль + C)! ===\n\n", i + 1);
    }
    return NULL;
}

int main() {
    pthread_t threadA, threadB, threadC, threadMod, threadFin;

    sem_init(&sem_part_A, 0, 0);
    sem_init(&sem_part_B, 0, 0);
    sem_init(&sem_part_C, 0, 0);
    sem_init(&sem_module, 0, 0);

    printf("Запуск производственной линии...\n");

    pthread_create(&threadA, NULL, worker_A, NULL);
    pthread_create(&threadB, NULL, worker_B, NULL);
    pthread_create(&threadC, NULL, worker_C, NULL);
    pthread_create(&threadMod, NULL, worker_Module, NULL);
    pthread_create(&threadFin, NULL, worker_Final, NULL);

    pthread_join(threadA, NULL);
    pthread_join(threadB, NULL);
    pthread_join(threadC, NULL);
    pthread_join(threadMod, NULL);
    pthread_join(threadFin, NULL);

    sem_destroy(&sem_part_A);
    sem_destroy(&sem_part_B);
    sem_destroy(&sem_part_C);
    sem_destroy(&sem_module);

    printf("План выполнен.\n");
    return 0;
}