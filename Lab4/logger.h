#ifndef LOGGER_H
#define LOGGER_H
#define MAX_LOG_MSG_LEN 1024
#include <stdbool.h>
#include <stdio.h>
#include <execinfo.h>
#include <pthread.h>



// Уровни логирования
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

void log_enqueue(LogLevel level, const char *message, void* callstack, int frames);

// Инициализация логгера
bool logger_init(const char *filename);

// Запись сообщения в лог
void logger_log(LogLevel level, const char *format, ...);

// Завершение работы 
void logger_shutdown();

// Макросы
#define LOG_DEBUG(...)    logger_log(LOG_DEBUG, ##__VA_ARGS__)
#define LOG_INFO(...)     logger_log(LOG_INFO, ##__VA_ARGS__)
#define LOG_WARNING(...)  logger_log(LOG_WARNING, ##__VA_ARGS__)
#define LOG_ERROR(...)do{\
    void* callstack[50];\
    int frames = backtrace((void**)callstack, 50);\
    char buffer[MAX_LOG_MSG_LEN];\
    snprintf(buffer, sizeof(buffer),##__VA_ARGS__);\
    log_enqueue(LOG_ERROR, buffer, callstack, frames);\
}while(0)

#endif