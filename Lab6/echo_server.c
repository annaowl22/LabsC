#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024
#define PORT 8080

static void set_nonblocking(int fd){
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0){
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    int rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc < 0){
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

int main(){
    int server_fd, client_fd, epoll_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct epoll_event ev, events[MAX_EVENTS];
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))){
        perror("bind");
        close(server_fd);
        EXIT_FAILURE;
    }

    set_nonblocking(server_fd);

    if(listen(server_fd, SOMAXCONN) == -1){
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1){
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1){
        perror("epoll_ctl");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен на порте %d\n", PORT);

    while(1){
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1){
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++){
            if (events[i].data.fd == server_fd){
                client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd == -1){
                    if (errno != EAGAIN && errno != EWOULDBLOCK){
                        perror("accept");
                    }continue;
                }
                printf("Соединение с %s: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                set_nonblocking(client_fd);

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1){
                    perror("epoll_ctl: client");
                    close(client_fd);
                }
            }
            else{
                int n = read(events[i].data.fd, buffer, BUFFER_SIZE);
                if(n <= 0){
                    if (n == 0 || errno == ECONNREFUSED){
                        printf("Клиент отключен\n");
                    }else{
                        perror("read");
                    }
                    close(events[i].data.fd);
                }else{
                    buffer[n] = '\0';
                    write(events[i].data.fd, buffer, n);
                }
            }
        }
    }
    close(server_fd);
    close(epoll_fd);
    return 0;
}