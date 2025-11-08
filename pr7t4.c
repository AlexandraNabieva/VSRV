#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void sigint_handler(int sig) {
    printf("\nSIGINT handler activated (Ctrl+C pressed)\n");
    printf("Suspending current process...\n");
    
    raise(SIGSTOP);
    
    printf("Process resumed after suspension\n");
}

int main() {

    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Signal handler setup error");
        return 1;
    }
    
    printf("Program started. PID: %d\n", getpid());
    printf("Press Ctrl+C to test the signal handler\n");
    printf("The process will suspend and then resume\n");
    
    int counter = 0;
    while(1) {
        printf("Working... iteration %d\n", counter++);
        
        alarm(2);
        
        pause();
        
        printf("Alarm received, continuing work...\n");
    }
    
    return 0;
}