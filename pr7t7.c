#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static volatile sig_atomic_t alarm_fired = 0;

void sigalrm_handler(int sig) {
    alarm_fired = 1;
}

unsigned int my_sleep(unsigned int seconds) {
    void (*original_handler)(int);
    unsigned int remaining_time;
    
    original_handler = signal(SIGALRM, sigalrm_handler);
    if (original_handler == SIG_ERR) {
        return seconds;
    }
    
    remaining_time = alarm(0);
    
    alarm_fired = 0;
    alarm(seconds);
    
    while (!alarm_fired) {
        pause();
    }
    
    signal(SIGALRM, original_handler);
    
    if (remaining_time > 0) {
        if (seconds < remaining_time) {
            remaining_time -= seconds;
        } else {
            remaining_time = 0;
        }
        alarm(remaining_time);
    } else {
        alarm(0);
    }
    
    return alarm_fired ? 0 : seconds;
}


int main() {
    printf("Program started\n");
    
    printf("Sleeping for 3 seconds using my_sleep...\n");
    unsigned int remaining = my_sleep(3);
    printf("Awake! Remaining time: %u seconds\n", remaining);
    
    printf("Sleeping for 5 seconds using my_sleep...\n");
    remaining = my_sleep(5);
    printf("Awake! Remaining time: %u seconds\n", remaining);
    
    printf("\nTesting with previous alarm...\n");
    alarm(10);
    printf("Previous alarm set for 10 seconds\n");
    
    printf("Sleeping for 3 seconds...\n");
    remaining = my_sleep(3);
    printf("Awake! Previous alarm remaining time preserved\n");
    
    unsigned int prev_remaining = alarm(0);
    printf("Previous alarm remaining time: %u seconds\n", prev_remaining);
    
    printf("Program completed successfully\n");
    return 0;
}