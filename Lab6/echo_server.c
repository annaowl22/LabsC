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

void send_file(int client_fd, const char *filepath){
    FILE *file = fopen(filepath, "rb");
    if (!file){
        perror("fopen");
        printf("Полный путь: %s\n",filepath);
        if(errno == ENOENT){
            const char *response = "HTTP/1.1 404 Not Found\n"
                                   "Content-Type: text/plain\n"
                                   "\n"
                                   "Файл с данным именем не найден\n";
            write(client_fd,response, strlen(response));
        }else{
            const char *response = "HTTP/1.1 403 Forbidden\n"
                                 "Content-Type: text/plain\n"
                                 "\n"
                                 "403: Файл недоступен для чтения. Может быть временно, попробуйте ещё раз позже\n";
            write(client_fd, response, strlen(response));
        }
        return;
    }
    const char *header = "HTTP/1.1 200 OK\n"
                         "Content-Type: text/plain\r\n"
                         "\n";
    write(client_fd,header,strlen(header));

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file))>0){
        if (write(client_fd, buffer, bytes_read)!= bytes_read){
            perror("write");
            break;
        }
    }
    write(client_fd,"\n",1);
    fclose(file);
}

int main(int argc, char* argv[]){
    if (argc < 3){
        fprintf(stderr, "Не выбран адрес и порт\n");
        exit(EXIT_FAILURE);
    }
    if (argc < 4){
        fprintf(stderr, "Не выбрана директория\n");
        exit(EXIT_FAILURE);
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    if(port < 1 || port > 65535){
        fprintf(stderr, "Порт должен быть числом от 1 до 65535\n");
        exit(EXIT_FAILURE);
    }
    const char *dir = argv[3];
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
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0){
        perror("inet_pton");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

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
        perror("epoll_ctl: server");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен на порте %d\n", port);

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
                    printf("Клиент %d запросил файл %s", client_addr.sin_port, buffer);
                    char *method = strtok(buffer, " ");
                    char *filename = strtok(NULL, " ");
                    if (strstr(filename, "..")){
                        const char *errmess = "Не пытайтесь покинуть директорию\n";
                        write(client_fd,errmess,strlen(errmess));
                    }else{
                        if(method && filename && strcmp(method, "GET") == 0){
                        char filepath[BUFFER_SIZE+sizeof(dir)];
                        snprintf(filepath, sizeof(filepath), "%s/%s",dir,filename);
                        send_file(events[i].data.fd, filepath);
                        }
                    }
                }
            }
        }
    }
    close(server_fd);
    close(epoll_fd);
    return 0;
}