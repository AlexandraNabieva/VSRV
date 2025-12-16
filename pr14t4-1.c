#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    
    printf("TCP Server listening on port %d\n", PORT);

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, 
                           (socklen_t*)&addrlen);
        printf("New connection accepted\n");

        while(1) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(new_socket, buffer, BUFFER_SIZE);
            if(valread <= 0) break;
            
            printf("Received: %s", buffer);
            
            for(int i = 0; buffer[i]; i++)
                buffer[i] = toupper(buffer[i]);
            
            send(new_socket, buffer, strlen(buffer), 0);
        }
        
        close(new_socket);
        printf("Connection closed\n");
    }
    
    close(server_fd);
    return 0;
}