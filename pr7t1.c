#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main() {
    int pid1, pid2;
    
    pid1 = fork();
    if(pid1 == 0) {
        while(1) {
            printf("Pid1 is alive\n");
            sleep(1);
        }
    } else {
        pid2 = fork();
        if(pid2 == 0) {
            while(1) {
                printf("Pid2 is alive\n");
                sleep(1);
            }
        } else {
            sleep(3);
            kill(pid1, SIGSTOP);
            sleep(3);
            kill(pid1, SIGCONT);
            sleep(3);
            kill(pid1, SIGINT);
            kill(pid2, SIGINT);
        }
    }
    return 0;
}