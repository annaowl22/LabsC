#include "logger.h"
#include <unistd.h>
#include <string.h>
#include <time.h>

int main(int argc, char *argv[]){
    if (!logger_init("app.log")) {
        return 1;
    }
    if(argc > 1){
        if(strcmp(argv[1],"--debug")==0){
            log_set_level(LOG_DEBUG);
        }else if(strcmp(argv[1],"--info")==0){
            log_set_level(LOG_INFO);
        }else if(strcmp(argv[1],"--warning")==0){
            log_set_level(LOG_WARNING);
        }else if(strcmp(argv[1],"--error")==0){
            log_set_level(LOG_ERROR);
        }else if(strcmp(argv[1],"--none")==0){
            log_set_level(LOG_NONE);
        }
    }

    LOG_DEBUG("Сообщение об отладке");
    LOG_INFO("Программа начала выполнение");
    LOG_WARNING("Warning!");
    
    // Пример ошибки со стеком вызовов
    LOG_ERROR("Critical error occurred!");

    logger_shutdown();
    return 0;
    }