#include <stdio.h>
#include <unistd.h>

#define MAX_COUNT 200

void child_process(){
    printf("I'm child process with pid %d, ppid %d\n" , getpid() , getppid());
    for(int i = 0 ; i<MAX_COUNT ; i++){
        printf("This line is from child: %d\n" , i);
        usleep(1000);
    }
}

void parent_process(){
    printf("I'm parent process with pid %d, ppid %d" , getpid(), getppid());
    for (int i = 0 ; i<MAX_COUNT ; i++){
        printf("This line is from parent: %d\n" , i);
        usleep(2000);
    }
}

int main(){
    int newpid = fork();
    if(newpid == 0){
        child_process();
    }
    else if(newpid > 0){
        parent_process();
    }
    else {
        perror("fork");
    }
    return 0;
}