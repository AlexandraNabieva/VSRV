#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int parent_to_child[2];
    int child_to_parent[2];
    pid_t pid;
    char buffer[100];
    
    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        perror("pipe");
        return 1;
    }
    
    pid = fork();
    
    if (pid == 0) {
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        
        read(parent_to_child[0], buffer, sizeof(buffer));
        printf("[CHILD] Received from parent: '%s'\n", buffer);
        
        char *response = "Thank you for message! I'm doing well!";
        write(child_to_parent[1], response, strlen(response) + 1);
        printf("[CHILD] Sending to parent: '%s'\n", response);
        
        read(parent_to_child[0], buffer, sizeof(buffer));
        printf("[CHILD] Received from parent: '%s'\n", buffer);
        
        char *final_msg = "Goodbye! Have a nice day!";
        write(child_to_parent[1], final_msg, strlen(final_msg) + 1);
        printf("[CHILD] Sending to parent: '%s'\n", final_msg);
        
        close(parent_to_child[0]);
        close(child_to_parent[1]);
    } else {
        close(parent_to_child[0]);
        close(child_to_parent[1]);
        
        char *message1 = "Hello child! How are you doing?";
        write(parent_to_child[1], message1, strlen(message1) + 1);
        printf("[PARENT] Sending to child: '%s'\n", message1);
        
        read(child_to_parent[0], buffer, sizeof(buffer));
        printf("[PARENT] Received from child: '%s'\n", buffer);
        
        char *message2 = "Glad to hear that! Have a great day!";
        write(parent_to_child[1], message2, strlen(message2) + 1);
        printf("[PARENT] Sending to child: '%s'\n", message2);
        
        read(child_to_parent[0], buffer, sizeof(buffer));
        printf("[PARENT] Received from child: '%s'\n", buffer);
        
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        wait(NULL);
        printf("[PARENT] Conversation completed\n");
    }
    
    return 0;
}