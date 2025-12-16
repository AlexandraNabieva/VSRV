#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>
#include <termios.h>

#define MAX_LINE_LEN 9000 
#define HTTP_REQUEST_SIZE (MAX_LINE_LEN + 512)

#define HOSTNAME_SIZE 256
#define SCREEN_HEIGHT 25
#define BUFFER_SIZE 4096

struct termios old_term;

void set_cbreak_mode() {
    struct termios new_term;
    if (tcgetattr(STDIN_FILENO, &old_term) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_term) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

void reset_terminal_mode() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old_term) == -1) {
        perror("tcsetattr_reset");
    }
}

int parse_url(const char *url, char *hostname, char *path) {
    char *url_copy = strdup(url);
    char *protocol_end = strstr(url_copy, "://");
    char *host_start = url_copy;
    
    if (protocol_end) {
        host_start = protocol_end + 3;
    }
    
    char *path_start = strchr(host_start, '/');
    
    if (path_start) {
        size_t host_len = path_start - host_start;
        if (host_len >= HOSTNAME_SIZE) { 
             fprintf(stderr, "Hostname too long!\n");
             free(url_copy);
             return -1;
        }
        strncpy(hostname, host_start, host_len);
        hostname[host_len] = '\0';
        
        strncpy(path, path_start, MAX_LINE_LEN - 1);
        path[MAX_LINE_LEN - 1] = '\0';

    } else {
        strncpy(hostname, host_start, HOSTNAME_SIZE - 1);
        hostname[HOSTNAME_SIZE - 1] = '\0';
        strcpy(path, "/");
    }
    
    free(url_copy);
    return 0;
}

int main(int argc, char *argv[]) {
    char hostname[HOSTNAME_SIZE]; 
    char path[MAX_LINE_LEN];

    if (argc != 2) {
        fprintf(stderr, "Использование: %s <URL>\n", argv[0]);
        return 1;
    }

    if (parse_url(argv[1], hostname, path) != 0) {
        return 1;
    }
    const char *port = "80";
    
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, port, &hints, &servinfo) != 0) {
        fprintf(stderr, "Ошибка: Не удалось разрешить адрес %s\n", hostname);
        return 2;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Ошибка сокета");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("Ошибка соединения");
            continue;
        }
        break; 
    }

    if (p == NULL) {
        fprintf(stderr, "Ошибка: Не удалось подключиться к %s\n", hostname);
        freeaddrinfo(servinfo);
        return 2;
    }
    freeaddrinfo(servinfo);

    char request[HTTP_REQUEST_SIZE]; 
    
    snprintf(request, sizeof(request), 
             "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", 
             path, hostname);
    
    if (send(sockfd, request, strlen(request), 0) == -1) {
        perror("Ошибка отправки запроса");
        close(sockfd);
        return 3;
    }

    set_cbreak_mode();
    
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int line_count = 0;
    int is_scrolling_paused = 0;
    
    int socket_active = 1; 

    fd_set read_fds;
    int max_fd = (sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO);

    printf("--- Тело ответа ---\n");

    while (socket_active || is_scrolling_paused) {
        FD_ZERO(&read_fds);
        
        if (socket_active) {
            FD_SET(sockfd, &read_fds);
        }
        
        if (is_scrolling_paused) {
            FD_SET(STDIN_FILENO, &read_fds);
        }

        if (!socket_active && !is_scrolling_paused) {
            break; 
        }

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int select_result = select(max_fd + 1, &read_fds, NULL, NULL, &tv);

        if (select_result == -1) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        if (select_result == 0) {
            if (!socket_active) break;
            continue;
        }

        if (is_scrolling_paused && FD_ISSET(STDIN_FILENO, &read_fds)) {
            char key;
            if (read(STDIN_FILENO, &key, 1) == 1 && key == ' ') {
                is_scrolling_paused = 0;
                line_count = 0;
                printf("\r%70s\r", ""); 
                fflush(stdout);
            }
        }
        
        if (socket_active && !is_scrolling_paused && FD_ISSET(sockfd, &read_fds)) {
            bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                
                for (int i = 0; i < bytes_received; i++) {
                    fputc(buffer[i], stdout);
                    if (buffer[i] == '\n') {
                        line_count++;
                        if (line_count >= SCREEN_HEIGHT) {
                            is_scrolling_paused = 1;
                            printf("Press space to scroll down");
                            fflush(stdout);
                            break; 
                        }
                    }
                }
                fflush(stdout);
                
            } else if (bytes_received == 0) {
                socket_active = 0;
            } else {
                perror("recv");
                socket_active = 0;
            }
        }
    }

    reset_terminal_mode();
    close(sockfd);
    
    if (socket_active == 0) {
        printf("\n--- Конец ответа ---\n");
    }

    return 0;
}