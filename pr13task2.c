#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#define MAX_FOOD 1000
#define MIN_FOOD 100
#define CATS_COUNT 5
#define CAT_EAT_AMOUNT 100
pthread_mutex_t bowl_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t food_available = PTHREAD_COND_INITIALIZER;
pthread_cond_t need_more_food = PTHREAD_COND_INITIALIZER;
int food_amount = 500;
void* owner(void* arg) {
    while (1) {
        pthread_mutex_lock(&bowl_mutex);
        while (food_amount > MIN_FOOD) {
            pthread_cond_wait(&need_more_food, &bowl_mutex);
        }
        int add_amount = MAX_FOOD - food_amount;
        food_amount = MAX_FOOD;
        printf("Владелец добавил %d грамм корма. Теперь в миске: %d грамм\n", 
               add_amount, food_amount);
        pthread_cond_broadcast(&food_available);
        pthread_mutex_unlock(&bowl_mutex);
        sleep(2);
    }
    return NULL;
}
void* cat(void* arg) {
    int cat_id = *(int*)arg;
    while (1) {
        pthread_mutex_lock(&bowl_mutex);
        while (food_amount < CAT_EAT_AMOUNT) {
            printf("Кот %d ждет корма... (в миске: %dг)\n", cat_id, food_amount);
            if (food_amount <= MIN_FOOD) {
                pthread_cond_signal(&need_more_food);
            }
            pthread_cond_wait(&food_available, &bowl_mutex);
        }
        if (food_amount >= CAT_EAT_AMOUNT) {
            food_amount -= CAT_EAT_AMOUNT;
            printf("Кот %d съел %d грамм корма. Осталось: %d грамм\n", 
                   cat_id, CAT_EAT_AMOUNT, food_amount);
            if (food_amount <= MIN_FOOD) {
                printf("Внимание! Корма мало (%dг), нужно пополнить!\n", food_amount);
                pthread_cond_signal(&need_more_food);
            }
        }
        pthread_mutex_unlock(&bowl_mutex);
        sleep(rand() % 3 + 1);
    }
    return NULL;
}
int main() {
    pthread_t owner_thread;
    pthread_t cat_threads[CATS_COUNT];
    int cat_ids[CATS_COUNT];
    srand(time(NULL));
    printf("=== Программа 'Покорми кота' ===\n");
    printf("Максимальный корм: %dг, Минимальный корм: %dг\n", MAX_FOOD, MIN_FOOD);
    printf("Количество котов: %d, Порция на кота: %dг\n", CATS_COUNT, CAT_EAT_AMOUNT);
    printf("Начальное количество корма: %dг\n\n", food_amount);
    
    pthread_create(&owner_thread, NULL, owner, NULL);
    
    for (int i = 0; i < CATS_COUNT; i++) {
        cat_ids[i] = i + 1;
        pthread_create(&cat_threads[i], NULL, cat, &cat_ids[i]);
    }
    
    pthread_join(owner_thread, NULL);
    for (int i = 0; i < CATS_COUNT; i++) {
        pthread_join(cat_threads[i], NULL);
    }
    
    return 0;
}