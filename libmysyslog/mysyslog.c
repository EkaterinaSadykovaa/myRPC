#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>

void log_json(const char *message) {
    time_t now = time(NULL);
    printf("{\"time\":%ld,\"pid\":%d,\"message\":\"%s\"}\n", 
           now, getpid(), message);
    syslog(LOG_INFO, "%s", message); // Дублируем в системный лог
}
