#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pwd.h> // Для получения имени пользователя
#include <mysyslog.h>

#define MAX_BUFFER 1024

void print_help() {
    printf("Usage: myRPC-client [options]\n");
    printf("Options:\n");
    printf("  -c, --command \"команда bash\"  Command to execute\n");
    printf("  -h, --host \"ip_addr\"           Server address\n");
    printf("  -p, --port 1234                 Server port\n");
    printf("  -s, --stream                     Use stream socket\n");
    printf("  -d, --dgram                      Use datagram socket\n");
    printf("  --help                            Show this help message\n");
}

void escape_command(char *command) {
    char escaped[MAX_BUFFER];
    int j = 0;
    for (int i = 0; command[i] != '\0'; i++) {
        if (command[i] == '"') {
            escaped[j++] = '\\'; // Экранируем двойные кавычки
        }
        // Экранируем другие специальные символы
        if (command[i] == ';' || command[i] == '|' || command[i] == '`') {
            escaped[j++] = '\\'; // Экранируем специальные символы
        }
        escaped[j++] = command[i];
    }
    escaped[j] = '\0';
    strcpy(command, escaped);
}
int main(int argc, char *argv[]) {
    char *command = NULL;
    char *host = NULL;
    int port = 0;
    int socket_type = 0; // 1 for stream, 2 for datagram
    
    // В начале main():
    log_json("Клиент запущен");

    int opt;
    while ((opt = getopt(argc, argv, "c:h:p:sd")) != -1) {
        switch (opt) {
            case 'c':
                command = optarg;
                break;
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 's':
                socket_type = 1; // Stream
                break;
            case 'd':
                socket_type = 2; // Datagram
                break;
            case '?':
                print_help();
                exit(EXIT_FAILURE);
        }
    }

    // Проверка обязательных параметров
    if (!command || !host || port <= 0) {
        fprintf(stderr, "Error: Missing required parameters.\n");
        print_help();
        exit(EXIT_FAILURE);
    }

    // Получаем имя пользователя
    struct passwd *pw = getpwuid(getuid());
    char *username = pw->pw_name;

    // Экранирование команды
    escape_command(command);

    // Создание сокета
    int sock = (socket_type == 1) ? socket(AF_INET, SOCK_STREAM, 0) : socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        //perror("Socket creation failed");
        // При ошибках вместо perror():
        log_json("Ошибка создания сокета");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(sock);
        return EXIT_FAILURE;
    }

    // Подключение к серверу для потокового сокета
    if (socket_type == 1 && connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server failed");
        close(sock);
        return EXIT_FAILURE;
    }

    // Формируем JSON-строку для отправки
    char json_request[MAX_BUFFER];
    snprintf(json_request, sizeof(json_request), "{\"login\":\"%s\",\"command\":\"%s\"}", username, command);
    
    // Отправка JSON на сервер
    if (send(sock, json_request, strlen(json_request), 0) < 0) {
        perror("Failed to send command");
        close(sock);
        return EXIT_FAILURE;
    }
    
    // При успешной отправке:
    log_json("Команда отправлена на сервер");

    // Получение ответа от сервера (если необходимо)
    char buffer[MAX_BUFFER];
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("Failed to receive response");
        close(sock);
        return EXIT_FAILURE;
    }

    buffer[bytes_received] = '\0'; // Завершение строки
    printf("Response from server: %s\n", buffer);

    // Закрытие сокета
    close(sock);
    return EXIT_SUCCESS;
}
