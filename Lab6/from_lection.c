#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>

#define PORT 8080

static char buffer[2048];

bool set_nonblocking(int fd){
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0){
        perror("fcntl");
        return false;
    }
    int rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc < 0){
        perror("fcntl");
        return false;
    }
    return true;
}

int main(){
    int rc;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0){
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if(!set_nonblocking(listen_fd)){
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    rc = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc < 0){
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    int max_fd = listen_fd;
    FD_SET(listen_fd, &master_set);

    struct timeval timeout;
    timeout.tv_sec = 180;
    timeout.tv_usec = 0;

    bool end_server = false;
    while(!end_server){
        memcpy(&working_set, &master_set, sizeof(master_set));

        rc = select(max_fd + 1, &working_set, NULL, NULL, &timeout);
        if(rc < 0){
            perror("select");
            break;
        }
        if(rc == 0){
            printf("Timeout ending\n");
            break;
        }
        int desc_ready = rc;
        for (int i = 0; i <= max_fd && desc_ready > 0; i++){
            if (FD_ISSET(i, &working_set)){
                desc_ready--;
                if (i == listen_fd){
                    int new_fd;
                    do{
                        new_fd = accept(listen_fd, NULL, NULL);
                        if (new_fd < 0){
                            if (errno != EWOULDBLOCK){
                                perror("accept");
                                end_server = true;
                            }
                            break;
                        }
                        set_nonblocking(new_fd);
                        FD_SET(new_fd, &master_set);
                        if (new_fd > max_fd){
                            max_fd = new_fd;
                        }
                    }while (new_fd != -1);
                }
                else{
                    bool close_conn = false;
                    for(;;){
                        rc = recv(i, buffer, sizeof(buffer), 0);
                        if (rc < 0){
                            if (errno != EWOULDBLOCK){
                                perror("recv");
                                close_conn = true;
                            }
                            break;
                        }
                        if (rc == 0){
                            close_conn = true;
                            break;
                        }
                        int len = rc;
                        rc = send(i, buffer, len, 0);
                        if (rc < 0){
                            perror("send");
                            close_conn = true;
                            break;
                        }
                    }
                    if (close_conn){
                        close(i);
                        FD_CLR(i, &master_set);
                        if (i == max_fd){
                            while (FD_ISSET(max_fd, &master_set) == false) max_fd -= 1;
                        }
                    }
                }
            }
        }
    }
    for (int i = 0; i <= max_fd; ++i){
        if (FD_ISSET(i, &master_set)){
            close(i);
        }
    }
}