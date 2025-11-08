#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <signal.h>

int main() {
    char input[256];
    fd_set readfds;
    struct timeval timeout;
    int result;
    
    printf("You have 10 seconds to enter text: ");
    fflush(stdout);
    
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    result = select(1, &readfds, NULL, NULL, &timeout);
    
    if (result == 0) {
        printf("\nNo input\n");
    } else if (result > 0) {
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = 0;
            printf("Thank you! You entered: %s\n", input);
        }
    } else {
        printf("\nSelect error: %s\n", strerror(errno));
    }
    
    printf("\nTesting fgets interruption...\n");
    printf("Press Ctrl+C: ");
    fflush(stdout);
    
    signal(SIGINT, SIG_IGN);
    
    char *fgets_result = fgets(input, sizeof(input), stdin);
    
    printf("\nfgets returned: %p\n", (void*)fgets_result);
    if (fgets_result == NULL) {
        printf("fgets returned NULL, errno: %s\n", strerror(errno));
    } else {
        printf("Input: %s", input);
    }
    
    return 0;
}