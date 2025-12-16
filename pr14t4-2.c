#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char *server_ip = "127.0.0.1";

    if(argc > 1) server_ip = argv[1];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    
    printf("Connected to server %s:%d\n", server_ip, PORT);
    printf("Type messages (Ctrl+C to exit):\n");

    while(fgets(buffer, BUFFER_SIZE, stdin)) {
        send(sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, BUFFER_SIZE);
        read(sock, buffer, BUFFER_SIZE);
        printf("Uppercase: %s", buffer);
    }

    close(sock);
    return 0;
}