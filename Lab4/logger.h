#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stdio.h>

// Уровни логирования
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

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
#define LOG_ERROR(...)    logger_log(LOG_ERROR, ##__VA_ARGS__)

#endif