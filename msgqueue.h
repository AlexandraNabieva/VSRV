#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define MAX_MSG_LEN 81
#define MAX_QUEUE_SIZE 10 

typedef struct message {
    char data[MAX_MSG_LEN];
} message_t;

typedef struct queue {
    message_t buffer[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    
    pthread_mutex_t mutex;
    sem_t full;
    sem_t empty;
    
    bool dropped;
    pthread_cond_t drop_cond;

} queue_t;

void mymsginit(queue_t *q);
void mymsqdrop(queue_t *q);
void mymsgdestroy(queue_t *q);
int mymsgput(queue_t *q, char *msg);
int mymsgget(queue_t *q, char *buf, size_t bufsize);

#endif