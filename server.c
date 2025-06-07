#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

SOCKET client_sockets[MAX_CLIENTS] = {0};
char client_nicks[MAX_CLIENTS][50] = {0};

void broadcast(int sender_index, const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0 && i != sender_index) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    int addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int activity;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 3);

    printf("The server is running on the port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        SOCKET max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        activity = select(0, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    strcpy(client_nicks[i], "User");
                    printf("New user has been connected as User (#%d)\n", i);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                int valread = recv(sd, buffer, BUFFER_SIZE, 0);
                buffer[valread] = '\0';      // zakończenie bufora
                memset(buffer + valread, 0, BUFFER_SIZE - valread); // wyczyść resztę bufora
                if (valread <= 0) {
                    closesocket(sd);
                    client_sockets[i] = 0;
                    strcpy(client_nicks[i], "");
                } else {
                    buffer[valread] = '\0';

                    if (strncmp(buffer, "/help", 5) == 0) {
                        const char *help_message =
                                "Possible actions:\n"
                                "/nick <your_nickname> - set your new nickname\n"
                                "/join <chat_id> - join to group chat\n"
                                "/help - display this help message\n";

                        send(sd, help_message, strlen(help_message), 0);
                    }
                    if (strncmp(buffer, "/nick ", 6) == 0) {
                        char new_nick[50] = {0};

                        sscanf(buffer + 6, "%49s", new_nick);

                        if (strlen(new_nick) == 0) {
                            send(sd, "Enter correct nick!\n", strlen("Enter correct nick!\n"), 0);
                        } else {
                            char message[BUFFER_SIZE];
                            snprintf(message, sizeof(message), "%s changed nickname to: %s\n", client_nicks[i], new_nick);
                            printf("Klient #%d changed nickname to: %s\n", i, new_nick);
                            strcpy(client_nicks[i], new_nick);

                            broadcast(i, message);

                            send(sd, "Nickname has been changed correctly.\n", strlen("Nickname has been changed correctly.\n"), 0);
                        }
                    }
                    else {
                        char message[BUFFER_SIZE + 50];
                        snprintf(message, sizeof(message), "%s: %s", client_nicks[i], buffer);
                        broadcast(i, message);
                    }
                }
            }
        }
    }

    WSACleanup();
    return 0;
}
