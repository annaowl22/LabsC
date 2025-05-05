#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <execinfo.h> 
#include <stdarg.h>

#define MAX_LOG_MSG_LEN 1024

typedef struct {
    LogLevel level;
    char message[MAX_LOG_MSG_LEN];
    void* callstack[50];
    int callstack_size;
} LogEntry;

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t log_thread;
static bool log_thread_running = false;

// Очередь
static LogEntry log_queue[1024];
static int log_queue_front = 0;
static int log_queue_rear = 0;

// Добавление сообщения в очередь
void log_enqueue(LogLevel level, const char *message, void* callstack, int frames) {
    pthread_mutex_lock(&log_mutex);
    if ((log_queue_rear + 1) % 1024 != log_queue_front) {
        LogEntry entry;
        entry.level = level;
        strncpy(entry.message, message, MAX_LOG_MSG_LEN - 1);
        if(level == LOG_ERROR && callstack && frames > 0){
            memcpy(entry.callstack, callstack, sizeof(void*)*frames);
            entry.callstack_size = frames;
        }else{
            entry.callstack_size = 0;
        }
        log_queue[log_queue_rear] = entry;
        log_queue_rear = (log_queue_rear + 1) % 1024;
    }
    pthread_mutex_unlock(&log_mutex);
}

// Получение сообщения из очереди 
static bool log_dequeue(LogEntry *entry) {
    pthread_mutex_lock(&log_mutex);
    if (log_queue_front == log_queue_rear) {
        pthread_mutex_unlock(&log_mutex);
        return false;
    }
    *entry = log_queue[log_queue_front];
    log_queue_front = (log_queue_front + 1) % 1024;
    pthread_mutex_unlock(&log_mutex);
    return true;
}

// Функция для асинхронной записи
static void *log_thread_func(void *arg) {
    (void)arg;
    while (log_thread_running || log_queue_front != log_queue_rear) {
        LogEntry entry;
        if (log_dequeue(&entry)) {
            const char *level_str;
            switch (entry.level) {
                case LOG_DEBUG:   level_str = "DEBUG"; break;
                case LOG_INFO:    level_str = "INFO"; break;
                case LOG_WARNING: level_str = "WARNING"; break;
                case LOG_ERROR:   level_str = "ERROR"; break;
                default:          level_str = "UNKNOWN"; break;
            }

            char timestamp[20];
            struct tm *utc;
            const time_t timer = time(NULL);
            utc = localtime(&timer);
            utc->tm_hour += 3;
            time_t rus = mktime(utc);
            utc = localtime(&rus);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", utc);

            fprintf(log_file, "[%s] [%s] %s\n", timestamp, level_str, entry.message);

            // Для ERROR добавляем стек вызовов
            if (entry.level == LOG_ERROR && entry.callstack_size > 0) {
                char** symbols = backtrace_symbols(entry.callstack, entry.callstack_size);
                fprintf(log_file, "Stack trace:\n");
                for (int i = 0; i < entry.callstack_size; i++){
                    fprintf(log_file, " %s\n",symbols[i]);
                } 
            }

            fflush(log_file);  // Сброс буфера
        }
    }
    return NULL;
}

// Инициализация логгера
bool logger_init(const char *filename) {
    log_file = fopen(filename, "a");
    if (!log_file) {
        perror("Failed to open log file");
        return false;
    }

    log_thread_running = true;
    if (pthread_create(&log_thread, NULL, log_thread_func, NULL) != 0) {
        perror("Failed to start log thread");
        fclose(log_file);
        return false;
    }

    return true;
}

// Запись сообщения в лог
void logger_log(LogLevel level, const char *format, ...) {
    char buffer[MAX_LOG_MSG_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    log_enqueue(level, buffer,NULL,0);
}

// Завершение работы логгера
void logger_shutdown() {
    log_thread_running = false;
    pthread_join(log_thread, NULL);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}