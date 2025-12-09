#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int number = 0;
int number_ready = 0;
void* number_generator(void* arg) {
    srand(time(NULL));
    while (1) {
        pthread_mutex_lock(&mutex);
        number = rand() % 100 + 1;
        printf("Сгенерировано число: %d\n", number);
        number_ready = 1;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}
void* square_calculator(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (!number_ready || number % 2 != 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        if (number_ready && number % 2 == 0) {
            int square = number * number;
            printf("Квадрат четного числа %d: %d\n", number, square);
            number_ready = 0;
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
void* cube_calculator(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (!number_ready || number % 2 == 0) {
            pthread_cond_wait(&cond, &mutex);
        }
        if (number_ready && number % 2 != 0) {
            int cube = number * number * number;
            printf("Куб нечетного числа %d: %d\n", number, cube);
            number_ready = 0;
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}
int main() {
    pthread_t generator_thread, square_thread, cube_thread;
    pthread_create(&generator_thread, NULL, number_generator, NULL);
    pthread_create(&square_thread, NULL, square_calculator, NULL);
    pthread_create(&cube_thread, NULL, cube_calculator, NULL);
    pthread_join(generator_thread, NULL);
    pthread_join(square_thread, NULL);
    pthread_join(cube_thread, NULL);
    return 0;
}