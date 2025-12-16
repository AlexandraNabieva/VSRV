#include "msgqueue.h"

void mymsginit(queue_t *q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    q->dropped = false;

    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        perror("pthread_mutex_init failed");
        exit(EXIT_FAILURE);
    }
    
    if (sem_init(&q->full, 0, 0) != 0) {
        perror("sem_init full failed");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&q->empty, 0, MAX_QUEUE_SIZE) != 0) {
        perror("sem_init empty failed");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_cond_init(&q->drop_cond, NULL) != 0) {
        perror("pthread_cond_init failed");
        exit(EXIT_FAILURE);
    }
}

int mymsgput(queue_t *q, char *msg) {
    int transferred_chars = 0;
    
    if (sem_wait(&q->empty) != 0) {
        perror("sem_wait empty failed");
        return 0;
    }

    pthread_mutex_lock(&q->mutex);
    
    if (q->dropped) {
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->empty); 
        return 0;
    }
    
    size_t len = strnlen(msg, MAX_MSG_LEN - 1);
    
    strncpy(q->buffer[q->tail].data, msg, len);
    q->buffer[q->tail].data[len] = '\0';
    transferred_chars = len;

    q->tail = (q->tail + 1) % MAX_QUEUE_SIZE;
    q->count++;
    
    pthread_mutex_unlock(&q->mutex);
    
    sem_post(&q->full);
    
    return transferred_chars;
}

int mymsgget(queue_t *q, char *buf, size_t bufsize) {
    int read_chars = 0;
    
    if (sem_wait(&q->full) != 0) {
        perror("sem_wait full failed");
        return 0;
    }

    pthread_mutex_lock(&q->mutex);
    
    if (q->dropped) {
        pthread_mutex_unlock(&q->mutex);
        sem_post(&q->full);
        return 0;
    }
    
    size_t len = strnlen(q->buffer[q->head].data, MAX_MSG_LEN - 1);
    size_t copy_len = len < (bufsize - 1) ? len : (bufsize - 1);
    
    strncpy(buf, q->buffer[q->head].data, copy_len);
    buf[copy_len] = '\0';
    read_chars = copy_len;
    
    q->head = (q->head + 1) % MAX_QUEUE_SIZE;
    q->count--;
    
    pthread_mutex_unlock(&q->mutex);
    
    sem_post(&q->empty);
    
    return read_chars;
}

void mymsqdrop(queue_t *q) {
    pthread_mutex_lock(&q->mutex);
    
    if (q->dropped) {
        pthread_mutex_unlock(&q->mutex);
        return;
    }
    
    q->dropped = true;
    
    int i;
    int sem_val;
    sem_getvalue(&q->empty, &sem_val);
    int num_to_post_put = MAX_QUEUE_SIZE - sem_val;
    for (i = 0; i < num_to_post_put; i++) {
        sem_post(&q->empty);
    }
    
    sem_getvalue(&q->full, &sem_val);
    int num_to_post_get = sem_val;
    for (i = 0; i < num_to_post_get; i++) {
        sem_post(&q->full);
    }
    
    for (i = 0; i < MAX_QUEUE_SIZE; i++) {
         sem_post(&q->full);
         sem_post(&q->empty);
    }

    pthread_mutex_unlock(&q->mutex);
    
    printf("\n>>> ОЧЕРЕДЬ СБРОШЕНА (DROP). Get/Put возвращают 0. <<<\n\n");
}

void mymsgdestroy(queue_t *q) {
    if (pthread_mutex_destroy(&q->mutex) != 0) {
        perror("pthread_mutex_destroy failed");
    }
    
    if (sem_destroy(&q->full) != 0) {
        perror("sem_destroy full failed");
    }
    if (sem_destroy(&q->empty) != 0) {
        perror("sem_destroy empty failed");
    }
    
    if (pthread_cond_destroy(&q->drop_cond) != 0) {
        perror("pthread_cond_destroy failed");
    }
    
    printf("Очередь уничтожена.\n");
}