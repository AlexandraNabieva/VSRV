#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ctype.h>

#define SOCKET_NAME "/tmp/my_unix_socket"
#define BUFFER_SIZE 128

int main() {
    int connection_socket, data_socket;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];

    unlink(SOCKET_NAME);
    connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path)-1);
    bind(connection_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    listen(connection_socket, 5);

    printf("UNIX Server waiting for connections...\n");

    while(1) {
        data_socket = accept(connection_socket, NULL, NULL);
        printf("Client connected\n");

        while(1) {
            memset(buffer, 0, BUFFER_SIZE);
            if (read(data_socket, buffer, BUFFER_SIZE) <= 0) break;
            printf("Received: %s", buffer);
            
            for(int i = 0; buffer[i]; i++)
                buffer[i] = toupper(buffer[i]);
            
            write(data_socket, buffer, strlen(buffer)+1);
        }
        close(data_socket);
        printf("Client disconnected\n");
    }
    close(connection_socket);
    unlink(SOCKET_NAME);
    return 0;
}