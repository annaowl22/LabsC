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


void daemonize(const char* cmd){
    int i, fd0, fd1, fd2;
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
    for (i = 0; i < rl.rlim_max; i++){
        close(i);
    }

    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    if(fd0 != 0 || fd1 != 1 || fd2 != 2){
        syslog(LOG_CRIT, "Wrong descriptors: %d %d %d", fd0, fd1, fd2);
    }
}
//Заметка на сделать: Журналирование начинается на 55 минуте