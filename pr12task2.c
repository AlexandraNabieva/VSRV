#include <stdio.h>
#include <unistd.h>
#define __USE_GNU
#include <pthread.h>
#include <stdlib.h>

#define NUM_HANDLER_THREADS 3

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t got_request = PTHREAD_COND_INITIALIZER;

int num_requests = 0;

struct request {
    int number;
    struct request* next;
};

struct request* requests = NULL;
struct request* last_request = NULL;

void add_request(int request_num)
{
    struct request* a_request;

    a_request = (struct request*)malloc(sizeof(struct request));
    if (!a_request) {
        fprintf(stderr, "add_request: out of memory\n");
        exit(1);
    }
    a_request->number = request_num;
    a_request->next = NULL;

    pthread_mutex_lock(&queue_mutex);

    if (num_requests == 0) {
        requests = a_request;
        last_request = a_request;
    }
    else {
        last_request->next = a_request;
        last_request = a_request;
    }

    num_requests++;

#ifdef DEBUG
    printf("add_request: added request with id '%d'\n", a_request->number);
    fflush(stdout);
#endif

    pthread_mutex_unlock(&queue_mutex);

    pthread_mutex_lock(&cond_mutex);
    pthread_cond_signal(&got_request);
    pthread_mutex_unlock(&cond_mutex);
}

struct request* get_request()
{
    struct request* a_request = NULL;

    pthread_mutex_lock(&queue_mutex);

    if (num_requests > 0) {
        a_request = requests;
        requests = a_request->next;
        if (requests == NULL) {
            last_request = NULL;
        }
        num_requests--;
    }

    pthread_mutex_unlock(&queue_mutex);

    return a_request;
}

void handle_request(struct request* a_request, int thread_id)
{
    if (a_request) {
        printf("Thread '%d' handled request '%d'\n", thread_id, a_request->number);
        fflush(stdout);
    }
}

void* handle_requests_loop(void* data)
{
    struct request* a_request;
    int thread_id = *((int*)data);

#ifdef DEBUG
    printf("Starting thread '%d'\n", thread_id);
    fflush(stdout);
#endif

    while (1) {
        a_request = get_request();
        
        if (a_request) {
            handle_request(a_request, thread_id);
            free(a_request);
        }
        else {
            pthread_mutex_lock(&cond_mutex);
            
            a_request = get_request();
            if (a_request == NULL) {
#ifdef DEBUG
                printf("thread '%d' before pthread_cond_wait\n", thread_id);
                fflush(stdout);
#endif
                
                pthread_cond_wait(&got_request, &cond_mutex);
                
#ifdef DEBUG
                printf("thread '%d' after pthread_cond_wait\n", thread_id);
                fflush(stdout);
#endif
            }
            
            pthread_mutex_unlock(&cond_mutex);
            
            if (a_request) {
                handle_request(a_request, thread_id);
                free(a_request);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    int i;
    int thr_id[NUM_HANDLER_THREADS];
    pthread_t p_threads[NUM_HANDLER_THREADS];
    struct timespec delay;

    printf("Server started with %d handler threads\n", NUM_HANDLER_THREADS);
    printf("Using separate mutexes for queue and condition variable\n");

    for (i = 0; i < NUM_HANDLER_THREADS; i++) {
        thr_id[i] = i;
        pthread_create(&p_threads[i], NULL, handle_requests_loop, (void*)&thr_id[i]);
    }
    
    sleep(3);
    
    printf("Generating requests...\n");
    for (i = 0; i < 20; i++) {
        add_request(i);
        
        if (rand() > 3 * (RAND_MAX / 4)) {
            delay.tv_sec = 0;
            delay.tv_nsec = 10000000;
            nanosleep(&delay, NULL);
        }
    }
    
    sleep(3);

    printf("All requests processed successfully\n");
    printf("Final queue size: %d\n", num_requests);
    printf("Server shutdown complete\n");

    return 0;
}