#include <stdio.h>
#include <unistd.h>

#define MAX_COUNT 150

void main(){
    printf("I'm parent process with pid %d, ppid %d\n" , getpid() , getppid());
    for (int i = 0 ; i<MAX_COUNT ; i++){
        printf("This line is from parent: %d\n" , i);
        usleep(2000);
    }
}