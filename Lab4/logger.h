#ifndef LOGGER_H
#define LOGGER_H
#define MAX_LOG_MSG_LEN 1024
#include <stdbool.h>
#include <stdio.h>
#include <execinfo.h>
#include <pthread.h>
#include <time.h>



// Уровни логирования
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_NONE
} LogLevel;

void log_set_level(LogLevel level);

LogLevel log_get_level();

void log_enqueue(LogLevel level, const char *message, void* callstack, int frames,time_t time);

// Инициализация логгера
bool logger_init(const char *filename);

// Запись сообщения в лог
void logger_log(LogLevel level, time_t time, const char *format, ...);

// Завершение работы 
void logger_shutdown();

// Макросы
#define LOG_DEBUG(...)    if(log_get_level() <= LOG_DEBUG) logger_log(LOG_DEBUG,time(NULL), ##__VA_ARGS__)
#define LOG_INFO(...)     if(log_get_level() <= LOG_INFO) logger_log(LOG_INFO,time(NULL), ##__VA_ARGS__)
#define LOG_WARNING(...)  if(log_get_level() <= LOG_WARNING) logger_log(LOG_WARNING,time(NULL), ##__VA_ARGS__)
#define LOG_ERROR(...)if(log_get_level() <= LOG_ERROR){\
    void* callstack[50];\
    int frames = backtrace((void**)callstack, 50);\
    char buffer[MAX_LOG_MSG_LEN];\
    snprintf(buffer, sizeof(buffer),##__VA_ARGS__);\
    log_enqueue(LOG_ERROR, buffer, callstack, frames,time(NULL));\
}

#endif