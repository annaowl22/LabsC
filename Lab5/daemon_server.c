#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define CONFIG_FILE "/workspaces/LabsC/Lab5/daemon_server.conf"
#define SOCKET_PATH "/workspaces/LabsC/Lab5/socket"

volatile sig_atomic_t stop_flag = 0;
char *target_file = NULL;

void handle_signal(){
    stop_flag = 1;
}

void read_config() {
    FILE *conf = fopen(CONFIG_FILE, "r");
    if(!conf){
        perror("Cannot find config file(");
        exit(EXIT_FAILURE);
    }

    char line[256];
    if(fgets(line, sizeof(line), conf)){
        line[strcspn(line,"\n")] = '\0';
        target_file = strdup(line);
    }
    fclose(conf);
    if(!target_file){
        fprintf(stderr,"No file in config");
        exit(EXIT_FAILURE);
    }
}

//Работа сервера 
void run_server(){
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char response[64];

    //Создание сокета
    if((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);

    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) == -1){
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    //Ожидание запроса от клиента
    while(!stop_flag){
        if ((client_fd = accept(server_fd, NULL,NULL)) == -1){
            continue;
        }

        struct stat st;
        if (stat(target_file, &st) == -1){
            snprintf(response,sizeof(response),"File not found\n");
        }else{
            snprintf(response, sizeof(response),"Size of file: %ld bites\n",(long)st.st_size);
        }

        write(client_fd, response, strlen(response));
        close(client_fd);
    }
    close(server_fd);
    unlink(SOCKET_PATH);
}

//Демонизация по туториалу в лекции
void daemonize(const char* cmd){
    int fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    openlog(cmd, LOG_CONS, LOG_DAEMON);

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) perror("Cannot get max descriptor");

    if((pid = fork()) < 0){
        perror("Error function fork");
    }
    else if (pid != 0){
        exit(0);
    }
    setsid();


    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0){
        syslog(LOG_CRIT, "Cannot ignore SIGHUP");
    }

    if ((pid = fork()) < 0){
        syslog(LOG_CRIT, "Error function fork");
    }else if (pid != 0){
        exit(0);
    }

    if (chdir("/") < 0){
        syslog(LOG_CRIT, "Cannot make current directory /");
    }

    if (rl.rlim_max == RLIM_INFINITY){
        rl.rlim_max = 1024;
    }
    for (rlim_t i = 0; i < rl.rlim_max; i++){
        close((int)i);
    }

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    if(fd0 != 0 || fd1 != 1 || fd2 != 2){
        syslog(LOG_CRIT, "Wrong descriptors: %d %d %d", fd0, fd1, fd2);
    }
}

int main(){

    read_config();

    daemonize("daemon_server");

    signal(SIGTERM, handle_signal);

    run_server();

    free(target_file);

    syslog(LOG_INFO, "Daemon has stopped");
    closelog();
    return 0;
}

