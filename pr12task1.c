#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_QUEUE_SIZE 100
#define NUM_WORKER_THREADS 3

struct task {
    int id;
    char message[256];
    struct task* next;
};

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t task_available = PTHREAD_COND_INITIALIZER;

struct task* task_queue = NULL;
struct task* last_task = NULL;
int task_count = 0;
int task_id_counter = 0;

void add_task(const char* message) {
    pthread_mutex_lock(&queue_mutex);
    
    if (task_count >= MAX_QUEUE_SIZE) {
        printf("Queue is full! Cannot add more tasks.\n");
        pthread_mutex_unlock(&queue_mutex);
        return;
    }
    
    struct task* new_task = (struct task*)malloc(sizeof(struct task));
    if (!new_task) {
        printf("Memory allocation failed!\n");
        pthread_mutex_unlock(&queue_mutex);
        return;
    }
    
    new_task->id = task_id_counter++;
    strncpy(new_task->message, message, sizeof(new_task->message) - 1);
    new_task->message[sizeof(new_task->message) - 1] = '\0';
    new_task->next = NULL;
    
    if (task_queue == NULL) {
        task_queue = new_task;
        last_task = new_task;
    } else {
        last_task->next = new_task;
        last_task = new_task;
    }
    
    task_count++;
    
    printf("Producer: Added task %d - '%s' (Total tasks: %d)\n", 
           new_task->id, new_task->message, task_count);
    
    pthread_cond_signal(&task_available);
    pthread_mutex_unlock(&queue_mutex);
}

struct task* get_task() {
    struct task* task = NULL;
    
    if (task_count > 0) {
        task = task_queue;
        task_queue = task->next;
        
        if (task_queue == NULL) {
            last_task = NULL;
        }
        
        task_count--;
    }
    
    return task;
}

void process_task(struct task* task, int thread_id) {
    if (task) {
        printf("Worker %d: Processing task %d - '%s'\n", 
               thread_id, task->id, task->message);
        
        usleep(100000 + (rand() % 200000));
        
        printf("Worker %d: Completed task %d\n", thread_id, task->id);
        
        free(task);
    }
}

void* worker_thread(void* arg) {
    int thread_id = *((int*)arg);
    struct task* current_task = NULL;
    
    printf("Worker thread %d started\n", thread_id);
    
    while (1) {
        pthread_mutex_lock(&queue_mutex);
        
        while (task_count == 0) {
            printf("Worker %d: No tasks, waiting...\n", thread_id);
            pthread_cond_wait(&task_available, &queue_mutex);
        }
        
        current_task = get_task();
        
        pthread_mutex_unlock(&queue_mutex);
        
        if (current_task) {
            process_task(current_task, thread_id);
        }
    }
    
    return NULL;
}

void* producer_thread(void* arg) {
    int producer_id = *((int*)arg);
    const char* messages[] = {
        "Welcome email to new user",
        "Password reset notification",
        "Order confirmation",
        "Newsletter subscription",
        "Account verification",
        "Payment receipt",
        "System maintenance alert",
        "Special offer notification"
    };
    int message_count = sizeof(messages) / sizeof(messages[0]);
    
    printf("Producer thread %d started\n", producer_id);
    
    for (int i = 0; i < 10; i++) {
        usleep(500000 + (rand() % 1500000));
        
        const char* message = messages[rand() % message_count];
        char full_message[256];
        snprintf(full_message, sizeof(full_message), "%s (from producer %d)", 
                 message, producer_id);
        
        add_task(full_message);
    }
    
    printf("Producer thread %d finished\n", producer_id);
    return NULL;
}

int main() {
    pthread_t workers[NUM_WORKER_THREADS];
    pthread_t producers[2];
    int worker_ids[NUM_WORKER_THREADS];
    int producer_ids[2];
    
    srand(time(NULL));
    
    printf("=== Email Processing System Started ===\n");
    
    for (int i = 0; i < NUM_WORKER_THREADS; i++) {
        worker_ids[i] = i + 1;
        pthread_create(&workers[i], NULL, worker_thread, &worker_ids[i]);
    }
    
    for (int i = 0; i < 2; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer_thread, &producer_ids[i]);
    }
    
    for (int i = 0; i < 2; i++) {
        pthread_join(producers[i], NULL);
    }
    
    sleep(3);
    
    printf("=== All tasks processed ===\n");
    printf("Final queue size: %d\n", task_count);
    
    return 0;
}