#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    pid_t pid;
    char message[100];
    
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }
    
    pid = fork();
    
    if (pid == 0) {
        close(fd[0]);
        
        char *messages[] = {"Hello from child!", "How are you?", "This works great!", "END"};
        
        for (int i = 0; i < 4; i++) {
            printf("[SENDER] Sending: '%s'\n", messages[i]);
            write(fd[1], messages[i], strlen(messages[i]) + 1);
            sleep(1);
        }
        
        close(fd[1]);
        printf("[SENDER] All messages sent\n");
    } else {
        close(fd[1]);
        
        printf("[RECEIVER] Waiting for messages...\n");
        
        while (1) {
            int bytes = read(fd[0], message, sizeof(message));
            if (bytes <= 0) break;
            
            printf("[RECEIVER] Received: '%s'\n", message);
            
            if (strcmp(message, "END") == 0) {
                printf("[RECEIVER] Termination signal received\n");
                break;
            }
        }
        
        close(fd[0]);
        wait(NULL);
        printf("[RECEIVER] All messages processed\n");
    }
    
    return 0;
}