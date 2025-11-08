#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

static FILE *file_ptr;

void onintr(int sig) {
    char response[10];
    
    signal(SIGINT, onintr);
    
    printf("\n\nContinue? (y/n/r): ");
    fflush(stdout);
    
    if (fgets(response, sizeof(response), stdin) != NULL) {
        switch(response[0]) {
            case 'y':
            case 'Y':
                printf("Continuing output...\n");
                break;
            case 'n':
            case 'N':
                printf("Exiting program...\n");
                fclose(file_ptr);
                exit(0);
                break;
            case 'r':
            case 'R':
                printf("Restarting from beginning...\n");
                rewind(file_ptr);
                break;
            default:
                printf("Unknown command. Continuing...\n");
                break;
        }
    }
}

int main() {
    char line[256];
    
    file_ptr = fopen("/etc/termcap", "r");
    if (file_ptr == NULL) {
        printf("File /etc/termcap not found. Creating test file...\n");
        FILE *test_file = fopen("test_termcap.txt", "w");
        if (test_file) {
            for (int i = 0; i < 10; i++) {
                fprintf(test_file, "Line %d: Testing signal handling - Ctrl+C to interrupt\n", i+1);
            }
            fclose(test_file);
            printf("Test file 'test_termcap.txt' created successfully\n");
        }
        file_ptr = fopen("test_termcap.txt", "r");
        if (file_ptr == NULL) {
            perror("File opening error");
            return 1;
        }
    }
    
    signal(SIGINT, onintr);
    
    printf("Starting file output. Press Ctrl+C to control output.\n");
    printf("Each line will be displayed with 3 second delay.\n");
    printf("Options: y-continue, n-exit, r-restart from beginning\n");
    printf("====================================================\n");
    
    while (fgets(line, sizeof(line), file_ptr) != NULL) {
        printf("%s", line);
        fflush(stdout);
        
        sleep(3);
    }
    
    printf("\n====================================================\n");
    printf("File output completed successfully.\n");
    
    fclose(file_ptr);
    return 0;
}