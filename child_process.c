#include <stdio.h>
#include <unistd.h>

#define MAX_COUNT 100

void main(){
    printf("I'm child process with pid %d, ppid %d\n" , getpid() , getppid());
    for (int i = 0 ; i<MAX_COUNT ; i++){
        printf("This line is from child: %d\n" , i);
        usleep(1000);
    }
}