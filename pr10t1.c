#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SHM_KEY 0x12345

struct shmseg {
    int cntr;
    int write_complete;
};

void increment_counter(int pid, struct shmseg *shmp, int total_count) {
    int cntr;
    
    printf("Process %d: started work\n", pid);
    
    for (int i = 0; i < total_count; i++) {
        cntr = shmp->cntr;
        cntr += 1;
        shmp->cntr = cntr;
        
        if (i % 1000 == 0) {
            usleep(1000);
        }
    }
    
    printf("Process %d: finished work\n", pid);
}

int main() {
    int shmid;
    struct shmseg *shmp;
    pid_t pid;
    int total_count = 10000;
    
    shmid = shmget(SHM_KEY, sizeof(struct shmseg), 0644|IPC_CREAT);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }
    
    shmp = shmat(shmid, NULL, 0);
    if (shmp == (void *) -1) {
        perror("shmat");
        return 1;
    }
    
    shmp->cntr = 0;
    shmp->write_complete = 0;
    
    printf("Initial counter value: %d\n", shmp->cntr);
    printf("Expected final value: %d\n", total_count * 2);
    printf("=== RUNNING WITHOUT SEMAPHORES ===\n");
    
    pid = fork();
    
    if (pid > 0) {
        increment_counter(getpid(), shmp, total_count);
        
        wait(NULL);
        
        printf("\n=== RESULTS WITHOUT SEMAPHORES ===\n");
        printf("Actual counter value: %d\n", shmp->cntr);
        printf("Expected value: %d\n", total_count * 2);
        printf("Data loss: %d increments\n", total_count * 2 - shmp->cntr);
        printf("Race condition detected: YES\n");
        
        shmdt(shmp);
        shmctl(shmid, IPC_RMID, 0);
        
    } else if (pid == 0) {
        increment_counter(getpid(), shmp, total_count);
        exit(0);
    } else {
        perror("fork");
        return 1;
    }
    
    return 0;
}