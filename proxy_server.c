#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>

#define MAX_CONNECTIONS 512
#define BUF_SIZE 4096

typedef struct {
    int client_fd;
    int remote_fd;
    char *target_ip;
    int target_port;
} connection_pair_t;

connection_pair_t connections[MAX_CONNECTIONS];

int connect_to_remote(const char *hostname, int port) {
    struct sockaddr_in remote_addr;
    struct hostent *he;
    int remote_fd;

    if ((he = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname");
        return -1;
    }

    if ((remote_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    remote_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(remote_addr.sin_zero), '\0', 8);

    if (connect(remote_fd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect to remote failed");
        close(remote_fd);
        return -1;
    }

    return remote_fd;
}

void close_connection(struct pollfd fds[], int *nfds, int i) {
    int client_fd = fds[i].fd;
    int remote_fd = connections[i].remote_fd;

    if (client_fd >= 0) {
        close(client_fd);
    }
    if (remote_fd >= 0) {
        close(remote_fd);
    }

    printf("[INFO] Connection closed: Client FD %d, Remote FD %d\n", client_fd, remote_fd);

    if (i < *nfds - 1) {
        fds[i] = fds[*nfds - 1];
        connections[i] = connections[*nfds - 1];
    }
    (*nfds)--;
}

int proxy_data(int read_fd, int write_fd, struct pollfd fds[], int *nfds, int idx) {
    char buffer[BUF_SIZE];
    ssize_t bytes_read, bytes_sent;
    
    bytes_read = recv(read_fd, buffer, BUF_SIZE, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            printf("[INFO] Peer closed connection on FD %d.\n", read_fd);
        } else {
            perror("recv error");
        }
        
        close_connection(fds, nfds, idx);
        return 1;
    }

    bytes_sent = send(write_fd, buffer, bytes_read, 0);

    if (bytes_sent == -1) {
        perror("send error");
        close_connection(fds, nfds, idx);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int listen_port_X, target_port_X_prime;
    char *target_host_Y;
    int listen_fd;
    struct sockaddr_in serv_addr;
    
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <listen_port_X> <target_host_Y> <target_port_X'>\n", argv[0]);
        return EXIT_FAILURE;
    }

    listen_port_X = atoi(argv[1]);
    target_host_Y = argv[2];
    target_port_X_prime = atoi(argv[3]);

    if (listen_port_X <= 0 || target_port_X_prime <= 0) {
        fprintf(stderr, "Invalid port numbers.\n");
        return EXIT_FAILURE;
    }
    
    printf("Starting Proxy Server:\n");
    printf("  Listening on Port: %d\n", listen_port_X);
    printf("  Forwarding to: %s:%d\n", target_host_Y, target_port_X_prime);
    
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Listen socket creation failed");
        return EXIT_FAILURE;
    }

    int yes = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt SO_REUSEADDR");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(listen_port_X);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serv_addr.sin_zero), '\0', 8);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
        perror("Bind failed");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    if (listen(listen_fd, 512) == -1) {
        perror("Listen failed");
        close(listen_fd);
        return EXIT_FAILURE;
    }

    struct pollfd fds[MAX_CONNECTIONS];
    int nfds = 0;

    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    printf("[INFO] Proxy server is running...\n");

    while (1) {
        int poll_count = poll(fds, nfds, -1);

        if (poll_count == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll error");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents == 0) {
                continue;
            }

            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                fprintf(stderr, "[ERROR] Poll error/hangup on FD %d\n", fds[i].fd);
                if (fds[i].fd != listen_fd) {
                    close_connection(fds, &nfds, i);
                    i--;
                }
                continue;
            }

            if (fds[i].fd == listen_fd) {
                if (fds[i].revents & POLLIN) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd;

                    client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
                    if (client_fd == -1) {
                        perror("accept failed");
                        continue;
                    }

                    printf("[INFO] New client accepted on FD %d from %s:%d\n", 
                           client_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    if (nfds >= MAX_CONNECTIONS) {
                        fprintf(stderr, "[WARNING] Connection limit reached. Closing client FD %d.\n", client_fd);
                        close(client_fd);
                        continue;
                    }

                    int remote_fd = connect_to_remote(target_host_Y, target_port_X_prime);
                    
                    if (remote_fd == -1) {
                        fprintf(stderr, "[ERROR] Remote server %s:%d refused connection. Closing client FD %d.\n", 
                                target_host_Y, target_port_X_prime, client_fd);
                        close(client_fd);
                        continue;
                    }
                    
                    int new_client_idx = nfds;
                    int new_remote_idx = nfds + 1;
                    
                    if (new_remote_idx >= MAX_CONNECTIONS) {
                        fprintf(stderr, "[WARNING] Connection limit reached. Closing client/remote FDs.\n");
                        close(client_fd);
                        close(remote_fd);
                        continue;
                    }

                    fds[new_client_idx].fd = client_fd;
                    fds[new_client_idx].events = POLLIN;
                    fds[new_remote_idx].fd = remote_fd;
                    fds[new_remote_idx].events = POLLIN;

                    connections[new_client_idx].client_fd = client_fd;
                    connections[new_client_idx].remote_fd = remote_fd;
                    connections[new_remote_idx].client_fd = client_fd;
                    connections[new_remote_idx].remote_fd = remote_fd;

                    nfds += 2; 
                }
            }
            else {
                int client_fd = connections[i].client_fd;
                int remote_fd = connections[i].remote_fd;
                int other_idx = (fds[i].fd == client_fd) ? i + 1 : i - 1; 

                if (i >= nfds || other_idx >= nfds || fds[other_idx].fd < 0) {
                     continue;
                }

                if (fds[i].revents & POLLIN) {
                    if (fds[i].fd == client_fd) {
                        if (proxy_data(client_fd, remote_fd, fds, &nfds, i)) {
                            i--;
                        }
                    } 
                    else if (fds[i].fd == remote_fd) {
                        if (proxy_data(remote_fd, client_fd, fds, &nfds, i)) {
                             i--;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0) {
            close(fds[i].fd);
        }
    }
    printf("Server shutting down.\n");
    return EXIT_SUCCESS;
}