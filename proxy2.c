#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define PROXY_PORT 8080
#define MAX_CONNECTIONS 1024
#define BUFFER_SIZE 4096

typedef struct job {
    int client_fd;
    struct job *next;
} job_t;

typedef struct thread_pool {
    int num_threads;
    pthread_t *threads;
    
    job_t *head;
    job_t *tail;
    
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    
    int shutdown;
} thread_pool_t;

thread_pool_t *g_pool = NULL;

void handle_client_request(int client_fd) {
    printf("Worker thread %lu: Handling client FD %d\n", pthread_self(), client_fd);
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Worker thread %lu: Received %zd bytes. Request start: '%.50s...'\n", 
               pthread_self(), bytes_read, buffer);
            
        const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nProxy Ready\n";
        send(client_fd, response, strlen(response), 0);
    } else if (bytes_read == 0) {
        printf("Worker thread %lu: Client FD %d closed connection.\n", pthread_self(), client_fd);
    } else {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
             perror("recv error");
        }
    }
}

void *worker_thread_func(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;
    job_t *job;
    
    while (1) {
        pthread_mutex_lock(&pool->queue_mutex);
        
        while (pool->head == NULL && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }
        
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;
        }
        
        job = pool->head;
        pool->head = pool->head->next;
        if (pool->head == NULL) pool->tail = NULL;
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        if (job) {
            handle_client_request(job->client_fd);
            
            close(job->client_fd);
            free(job);
        }
    }
    printf("Worker thread %lu exiting.\n", pthread_self());
    return NULL;
}

void thread_pool_add_job(thread_pool_t *pool, int client_fd) {
    job_t *new_job = (job_t *)malloc(sizeof(job_t));
    if (!new_job) {
        perror("Failed to allocate job");
        close(client_fd);
        return;
    }
    new_job->client_fd = client_fd;
    new_job->next = NULL;

    pthread_mutex_lock(&pool->queue_mutex);

    if (pool->tail == NULL) {
        pool->head = new_job;
        pool->tail = new_job;
    } else {
        pool->tail->next = new_job;
        pool->tail = new_job;
    }

    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
}

int thread_pool_init(thread_pool_t *pool, int num_threads) {
    if (num_threads <= 0) {
        fprintf(stderr, "Pool size must be greater than 0.\n");
        return -1;
    }
    
    pool->num_threads = num_threads;
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    if (!pool->threads) {
        perror("Failed to allocate threads array");
        return -1;
    }
    
    pool->head = NULL;
    pool->tail = NULL;
    pool->shutdown = 0;

    pthread_mutex_init(&pool->queue_mutex, NULL);
    pthread_cond_init(&pool->queue_cond, NULL);
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread_func, pool) != 0) {
            perror("Failed to create thread");
            return -1;
        }
    }
    
    printf("Thread pool initialized with %d worker threads.\n", num_threads);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <thread_pool_size>\n", argv[0]);
        return 1;
    }

    int pool_size = atoi(argv[1]);
    if (pool_size <= 0) {
        fprintf(stderr, "Invalid thread_pool_size: must be a positive integer.\n");
        return 1;
    }

    thread_pool_t pool;
    if (thread_pool_init(&pool, pool_size) != 0) {
        return 1;
    }
    g_pool = &pool;
    
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket error");
        return 1;
    }
    
    int optval = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PROXY_PORT);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        close(listen_sock);
        return 1;
    }

    if (listen(listen_sock, MAX_CONNECTIONS) < 0) {
        perror("listen error");
        close(listen_sock);
        return 1;
    }

    printf("Proxy listening on port %d...\n", PROXY_PORT);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        
        FD_SET(listen_sock, &read_fds);
        int max_fd = listen_sock;

        int ready_count = select(max_fd + 1, &read_fds, NULL, NULL, NULL); 
        
        if (ready_count < 0) {
            if (errno == EINTR) continue;
            perror("select error");
            break;
        }
        
        if (FD_ISSET(listen_sock, &read_fds)) {
            int client_fd = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
            
            if (client_fd < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) continue;
                perror("accept error");
                continue;
            }
            
            printf("Main thread: Accepted new connection from %s:%d (FD %d). Adding to job queue.\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), client_fd);
            
            thread_pool_add_job(&pool, client_fd);
        }
    }

    pool.shutdown = 1;
    pthread_cond_broadcast(&pool.queue_cond);
    for (int i = 0; i < pool.num_threads; i++) {
        pthread_join(pool.threads[i], NULL);
    }
    
    pthread_mutex_destroy(&pool.queue_mutex);
    pthread_cond_destroy(&pool.queue_cond);
    free(pool.threads);
    
    close(listen_sock);
    
    return 0;
}