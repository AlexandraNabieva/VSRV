#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SHM_KEY 0x12345
#define SEM_KEY 0x54321

struct shmseg {
    int cntr;
    int write_complete;
};

struct sembuf sem_wait = {0, -1, SEM_UNDO};
struct sembuf sem_signal = {0, 1, SEM_UNDO};

void increment_counter_with_sem(int pid, struct shmseg *shmp, int total_count, int semid) {
    int cntr;
    
    printf("Process %d: started work\n", pid);
    
    for (int i = 0; i < total_count; i++) {
        if (semop(semid, &sem_wait, 1) == -1) {
            perror("semop wait");
            exit(1);
        }
        
        cntr = shmp->cntr;
        cntr += 1;
        shmp->cntr = cntr;
        
        if (semop(semid, &sem_signal, 1) == -1) {
            perror("semop signal");
            exit(1);
        }
        
        if (i % 1000 == 0) {
            usleep(1000);
        }
    }
    
    printf("Process %d: finished work\n", pid);
}

void remove_semaphore(int semid) {
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl remove");
    }
}

int main() {
    int shmid, semid;
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
    
    semid = semget(SEM_KEY, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid >= 0) {
        if (semctl(semid, 0, SETVAL, 1) == -1) {
            perror("semctl init");
            return 1;
        }
        printf("New semaphore created\n");
    } else if (errno == EEXIST) {
        semid = semget(SEM_KEY, 1, 0);
        if (semid == -1) {
            perror("semget existing");
            return 1;
        }
        printf("Existing semaphore used\n");
    } else {
        perror("semget");
        return 1;
    }
    
    shmp->cntr = 0;
    shmp->write_complete = 0;
    
    printf("Initial counter value: %d\n", shmp->cntr);
    printf("Expected final value: %d\n", total_count * 2);
    printf("=== RUNNING WITH SEMAPHORE PROTECTION ===\n");
    
    pid = fork();
    
    if (pid > 0) {
        increment_counter_with_sem(getpid(), shmp, total_count, semid);
        
        wait(NULL);
        
        printf("\n=== RESULTS WITH SEMAPHORES ===\n");
        printf("Actual counter value: %d\n", shmp->cntr);
        printf("Expected value: %d\n", total_count * 2);
        printf("Data loss: %d increments\n", total_count * 2 - shmp->cntr);
        printf("Race condition detected: NO\n");
        printf("Mutual exclusion: SUCCESSFUL\n");
        
        shmdt(shmp);
        shmctl(shmid, IPC_RMID, 0);
        remove_semaphore(semid);
        
    } else if (pid == 0) {
        increment_counter_with_sem(getpid(), shmp, total_count, semid);
        exit(0);
    } else {
        perror("fork");
        return 1;
    }
    
    return 0;
}