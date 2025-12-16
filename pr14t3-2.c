#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "/tmp/my_unix_socket"
#define BUFFER_SIZE 128

int main() {
    int data_socket;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];

    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path)-1);

    if (connect(data_socket, (struct sockaddr *)&addr, 
                sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected to UNIX server. Type messages:\n");
    
    while(fgets(buffer, BUFFER_SIZE, stdin)) {
        write(data_socket, buffer, strlen(buffer)+1);
        read(data_socket, buffer, BUFFER_SIZE);
        printf("Uppercase: %s", buffer);
    }

    close(data_socket);
    return 0;
}