#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "mysyslog.h"

#define MAX_BUFFER 1024
#define CONFIG_FILE "/etc/myRPC/myRPC.conf"
#define USERS_FILE "/etc/myRPC/users.conf"
#define TMP_STDOUT "/tmp/myRPC_%d.stdout"
#define TMP_STDERR "/tmp/myRPC_%d.stderr"

typedef struct {
    int port;
    int socket_type; // 1 for SOCK_STREAM, 2 for SOCK_DGRAM
} Config;

int read_config(Config *config) {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        perror("Failed to open config file");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "port=", 5) == 0) {
            config->port = atoi(line + 5);
        } else if (strncmp(line, "socket_type=", 12) == 0) {
            int type = atoi(line + 12);
            config->socket_type = (type == 1) ? SOCK_STREAM : SOCK_DGRAM; // 1 - TCP, 2 - UDP
        }
    }

    fclose(file);
    return 1; // Успех
}

int is_user_allowed(const char *username) {
    FILE *file = fopen(USERS_FILE, "r");
    if (!file) {
        perror("Failed to open users file");
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0; // Удаляем символ новой строки
        if (strcmp(line, username) == 0) {
            fclose(file);
            return 1; // Пользователь найден
        }
    }

    fclose(file);
    return 0; // Пользователь не найден
}

void execute_command(const char *command, char *result, int *code) {
    char stdout_file[256], stderr_file[256];
    //"/tmp/stdout_12345.txt" подставляем идентификатор
    snprintf(stdout_file, sizeof(stdout_file), TMP_STDOUT, getpid());
    snprintf(stderr_file, sizeof(stderr_file), TMP_STDERR, getpid());

    // Открываем файлы для записи
    FILE *stdout_fp = fopen(stdout_file, "w");
    FILE *stderr_fp = fopen(stderr_file, "w");

    if (!stdout_fp || !stderr_fp) {
        perror("Failed to open temp files");
        *code = 1;
        strcpy(result, "Failed to open temp files");
        return;
    }

    // Выполняем команду
    int pid = fork();
    if (pid == 0) { // Дочерний процесс
        dup2(fileno(stdout_fp), STDOUT_FILENO); // Перенаправляем stdout
        dup2(fileno(stderr_fp), STDERR_FILENO); // Перенаправляем stderr
        execlp("bash", "bash", "-c", command, NULL); // Выполняем команду
        perror("exec failed");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Fork failed");
        *code = 1;
        strcpy(result, "Fork failed");
        fclose(stdout_fp);
        fclose(stderr_fp);
        return;
    }

    fclose(stdout_fp);
    fclose(stderr_fp);

    // Ожидаем завершения дочернего процесса
    waitpid(pid, NULL, 0);

    // Чтение результата из временных файлов
    FILE *result_fp = fopen(stdout_file, "r");
    FILE *error_fp = fopen(stderr_file, "r");

    if (result_fp) {
        fread(result, 1, MAX_BUFFER, result_fp);
        fclose(result_fp);
        *code = 0; // Успех
    } else {
        *code = 1;
        strcpy(result, "No output");
    }

    if (error_fp) {
        char error_buffer[MAX_BUFFER];
        fread(error_buffer, 1, MAX_BUFFER, error_fp);
        fclose(error_fp);
        if (*code != 0) {
            strcpy(result, error_buffer); // Если ошибка, заменяем результат на stderr
        }
    }
}

void format_json_response(char *response, int code, const char *result) {
    snprintf(response, MAX_BUFFER, "{\"code\":%d,\"result\":\"%s\"}", code, result);
}
int main() {

	// В начале main():
	log_json("Сервер запущен");

    Config config;
    if (!read_config(&config)) {
        return EXIT_FAILURE;
    }

    // Создание сокета
    int server_sock = socket(AF_INET, config.socket_type, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config.port);

    // Привязка сокета
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        return EXIT_FAILURE;
    }

    // Прослушивание
    if (config.socket_type == SOCK_STREAM) {
        listen(server_sock, 5);
        printf("Server listening on port %d (TCP)\n", config.port);
    } else {
        printf("Server listening on port %d (UDP)\n", config.port);
    }

    while (1) {
        char buffer[MAX_BUFFER];
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock;

        if (config.socket_type == SOCK_STREAM) {
            client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
            if (client_sock < 0) {
                perror("Accept failed");
                continue;
            }
        } else {
            client_sock = server_sock; // Для UDP
        }

        // Получение сообщения от клиента
        int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        // При получении команды:
        log_json("Получена новая команда");
        if (bytes_received < 0) {
            //perror("Failed to receive message");
            // При ошибках:
            log_json("Ошибка выполнения команды");
            close(client_sock);
            continue;
        }

        buffer[bytes_received] = '\0'; // Завершение строки

        // Парсинг JSON
        char username[256];
        char command[256];
        sscanf(buffer, "{\"login\":\"%[^\"]\",\"command\":\"%[^\"]\"}", username, command);

        char result[MAX_BUFFER];
        int code = 0;

        if (is_user_allowed(username)) {
            execute_command(command, result, &code);
        } else {
            code = 1;
            strcpy(result, "User  not allowed");
        }

        // Формируем ответ в JSON
        char json_response[MAX_BUFFER];
        format_json_response(json_response, code, result);

        // Отправка ответа клиенту
        send(client_sock, json_response, strlen(json_response), 0);

        if (config.socket_type == SOCK_STREAM) {
            close(client_sock);
        }
    }

    close(server_sock);
    return EXIT_SUCCESS;
}
