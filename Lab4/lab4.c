#include "logger.h"
#include <unistd.h>

int main() {
    if (!logger_init("app.log")) {
        return 1;
    }

    LOG_DEBUG("Сообщение об отладке");
    LOG_INFO("Программа начала выполнение");
    LOG_WARNING("Warning!");
    
    // Пример ошибки со стеком вызовов
    LOG_ERROR("Critical error occurred!");

    logger_shutdown();
    return 0;
}