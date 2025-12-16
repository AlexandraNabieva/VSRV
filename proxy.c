#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_CONNECTIONS 100
#define PROXY_PORT 8082
#define BUF_SIZE 4096

typedef enum {
    STATE_FREE = 0,
    STATE_READING_REQUEST,
    STATE_CONNECTING_REMOTE,
    STATE_RELAYING,
    STATE_CACHED_RESPONSE,
} conn_state_t;

typedef struct {
    int client_fd;
    int remote_fd;
    conn_state_t state;
    char request_buf[BUF_SIZE];
    int buf_len;
} connection_t;

connection_t connections[MAX_CONNECTIONS];
int max_fd = 0;

void init_connections() {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connections[i].client_fd = -1;
        connections[i].remote_fd = -1;
        connections[i].state = STATE_FREE;
        connections[i].buf_len = 0;
    }
}

int find_free_slot() {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (connections[i].state == STATE_FREE) {
            return i;
        }
    }
    return -1;
}

void close_connection(int index) {
    if (connections[index].client_fd != -1) {
        close(connections[index].client_fd);
    }
    if (connections[index].remote_fd != -1) {
        close(connections[index].remote_fd);
    }
    printf("Connection closed (FD: %d)\n", connections[index].client_fd);
    connections[index].client_fd = -1;
    connections[index].remote_fd = -1;
    connections[index].state = STATE_FREE;
    connections[index].buf_len = 0;
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL, O_NONBLOCK)");
        return -1;
    }
    return 0;
}

void accept_new_connection(int listen_fd) {
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    
    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
    if (client_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("accept");
        }
        return;
    }

    if (set_nonblocking(client_fd) < 0) {
        close(client_fd);
        return;
    }

    int index = find_free_slot();
    if (index == -1) {
        printf("Max connections reached. Rejecting client.\n");
        close(client_fd);
        return;
    }

    connections[index].client_fd = client_fd;
    connections[index].state = STATE_READING_REQUEST;
    connections[index].buf_len = 0;
    
    if (client_fd > max_fd) {
        max_fd = client_fd;
    }
    printf("New connection accepted. FD: %d, Slot: %d\n", client_fd, index);
}

void handle_read(int index, int fd) {
    connection_t *conn = &connections[index];
    ssize_t bytes_read;

    bytes_read = read(fd, conn->request_buf + conn->buf_len, BUF_SIZE - conn->buf_len - 1);

    if (bytes_read > 0) {
        conn->buf_len += bytes_read;
        conn->request_buf[conn->buf_len] = '\0';
        
        printf("Slot %d: Read %zd bytes (State: %d)\n", index, bytes_read, conn->state);

        if (conn->state == STATE_READING_REQUEST) {
            if (conn->buf_len > 1024) { 
                conn->state = STATE_RELAYING;
            }
        }
        
    } else if (bytes_read == 0) {
        printf("Slot %d: Socket closed by peer (FD: %d).\n", index, fd);
        close_connection(index);
    } else {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("read");
            close_connection(index);
        }
    }
}

void handle_write(int index, int fd) {
    connection_t *conn = &connections[index];
    ssize_t bytes_written;

    if (conn->buf_len > 0) {
        bytes_written = write(fd, conn->request_buf, conn->buf_len);
        
        if (bytes_written > 0) {
            printf("Slot %d: Wrote %zd bytes to FD %d (State: %d)\n", index, bytes_written, fd, conn->state);
            
            conn->buf_len -= bytes_written;
            if (conn->buf_len > 0) {
                memmove(conn->request_buf, conn->request_buf + bytes_written, conn->buf_len);
            }
            if (conn->state == STATE_CACHED_RESPONSE && conn->buf_len == 0) {
                printf("Slot %d: Cached response sent completely.\n", index);
                close_connection(index);
            }
            
        } else if (bytes_written < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("write");
                close_connection(index);
            }
        }
    }
}

int main() {
    int listen_fd;
    struct sockaddr_in server_addr;

    init_connections();

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }
    
    if (set_nonblocking(listen_fd) < 0) {
        close(listen_fd);
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PROXY_PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("HTTP Proxy running on port %d (single-threaded, select-based).\n", PROXY_PORT);

    max_fd = listen_fd;

    while (1) {
        fd_set read_fds, write_fds;

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        FD_SET(listen_fd, &read_fds);

        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (connections[i].state != STATE_FREE) {
                
                if (connections[i].client_fd > 0) {
                    FD_SET(connections[i].client_fd, &read_fds);
                    if (connections[i].client_fd > max_fd) max_fd = connections[i].client_fd;
                }
                if (connections[i].remote_fd > 0) {
                    FD_SET(connections[i].remote_fd, &read_fds);
                    if (connections[i].remote_fd > max_fd) max_fd = connections[i].remote_fd;
                }
                
                if (connections[i].state == STATE_CACHED_RESPONSE) {
                    FD_SET(connections[i].client_fd, &write_fds);
                }
            }
        }
        
        int activity = select(max_fd + 1, &read_fds, &write_fds, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }
        
        if (FD_ISSET(listen_fd, &read_fds)) {
            accept_new_connection(listen_fd);
        }

        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (connections[i].state != STATE_FREE) {
                if (connections[i].client_fd > 0 && FD_ISSET(connections[i].client_fd, &read_fds)) {
                    handle_read(i, connections[i].client_fd);
                }
                if (connections[i].client_fd > 0 && FD_ISSET(connections[i].client_fd, &write_fds)) {
                    handle_write(i, connections[i].client_fd);
                }
                
                if (connections[i].remote_fd > 0 && FD_ISSET(connections[i].remote_fd, &read_fds)) {
                    handle_read(i, connections[i].remote_fd);
                }
                if (connections[i].remote_fd > 0 && FD_ISSET(connections[i].remote_fd, &write_fds)) {
                    handle_write(i, connections[i].remote_fd);
                }
            }
        }
    }

    close(listen_fd);
    return 0;
}