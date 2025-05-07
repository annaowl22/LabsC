#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int flag = 1;
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];

    setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    // Создание сокета
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Слушаем все интерфейсы
    server_addr.sin_port = htons(PORT);       // Порт 8080

    // Привязка сокета
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Ожидание подключений
    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d\n", PORT);

    while (1) {
        // Принятие подключения
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        read(client_fd, buffer, sizeof(buffer) - 1);
        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

        // HTTP-ответ
        char response[] = "HTTP/1.1 200 OK\r\n"
                          "Connection: keep-alive\r\n"
                          "Content-Type: text/html\r\n"
                          "Content-Length: 13\r\n"
                          "\r\n"
                          "Hello, Habr!\n";
        write(client_fd, response, strlen(response));
        fsync(client_fd);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}