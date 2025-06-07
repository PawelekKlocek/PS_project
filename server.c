#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define MAX_CLIENTS 10
#define MAX_GROUPS 10
#define MAX_CLIENTS_PER_GROUP 10
#define BUFFER_SIZE 1024

SOCKET client_sockets[MAX_CLIENTS] = {0};
char client_nicks[MAX_CLIENTS][50] = {0};
int client_groups[MAX_CLIENTS] = {-1}; // Indeks grupy dla każdego klienta (-1 = brak grupy)

typedef struct {
    char group_name[50];
    SOCKET members[MAX_CLIENTS_PER_GROUP];
    int member_count;
} ChatGroup;

ChatGroup groups[MAX_GROUPS];
int group_count = 0;

void broadcast_group(int group_index, const char *message, int sender_index) {
    if (group_index < 0 || group_index >= group_count) return;
    for (int i = 0; i < groups[group_index].member_count; i++) {
        if (groups[group_index].members[i] != client_sockets[sender_index]) {
            send(groups[group_index].members[i], message, strlen(message), 0);
        }
    }
}

void broadcast_all(int sender_index, const char *message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] != 0 && i != sender_index && client_groups[i] == -1) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
}

void handle_command(int client_index, char *buffer) {
    if (strncmp(buffer, "/help", 5) == 0) {
        const char *help_message =
                "Possible actions:\n"
                "/nick <nickname> - set your nickname\n"
                "/join <group_name> - join a group chat\n"
                "/leave - leave the current group\n"
                "/msg <nick> <message> - send a private message\n"
                "/list - list online users\n"
                "/help - display this help message\n";
        send(client_sockets[client_index], help_message, strlen(help_message), 0);
    }
    else if (strncmp(buffer, "/nick ", 6) == 0) {
        char new_nick[50] = {0};
        sscanf(buffer + 6, "%49s", new_nick);
        if (strlen(new_nick) == 0) {
            send(client_sockets[client_index], "Enter correct nick!\n", strlen("Enter correct nick!\n"), 0);
        } else {
            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "%s changed nickname to: %s\n", client_nicks[client_index], new_nick);
            printf("Client #%d changed nickname to: %s\n", client_index, new_nick);
            strcpy(client_nicks[client_index], new_nick);
            if (client_groups[client_index] != -1) {
                broadcast_group(client_groups[client_index], message, client_index);
            } else {
                broadcast_all(client_index, message);
            }
            send(client_sockets[client_index], "Nickname changed.\n", strlen("Nickname changed.\n"), 0);
        }
    }
    else if (strncmp(buffer, "/join ", 6) == 0) {
        char group_name[50];
        sscanf(buffer + 6, "%49s", group_name);

        int group_index = -1;
        for (int i = 0; i < group_count; i++) {
            if (strcmp(groups[i].group_name, group_name) == 0) {
                group_index = i;
                break;
            }
        }

        if (group_index == -1 && group_count < MAX_GROUPS) {
            group_index = group_count++;
            strcpy(groups[group_index].group_name, group_name);
            groups[group_index].member_count = 0;
        }

        if (group_index != -1 && groups[group_index].member_count < MAX_CLIENTS_PER_GROUP) {
            // Usuń z poprzedniej grupy, jeśli należy
            if (client_groups[client_index] != -1) {
                for (int m = 0; m < groups[client_groups[client_index]].member_count; m++) {
                    if (groups[client_groups[client_index]].members[m] == client_sockets[client_index]) {
                        groups[client_groups[client_index]].members[m] = groups[client_groups[client_index]].members[--groups[client_groups[client_index]].member_count];
                        // Usuń pustą grupę
                        if (groups[client_groups[client_index]].member_count == 0) {
                            groups[client_groups[client_index]] = groups[--group_count];
                        }
                        break;
                    }
                }
            }

            groups[group_index].members[groups[group_index].member_count++] = client_sockets[client_index];
            client_groups[client_index] = group_index;
            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "You joined group %s\n", group_name);
            send(client_sockets[client_index], message, strlen(message), 0);
            snprintf(message, sizeof(message), "%s joined group %s\n", client_nicks[client_index], group_name);
            broadcast_group(group_index, message, client_index);
        } else {
            send(client_sockets[client_index], "Cannot join group!\n", strlen("Cannot join group!\n"), 0);
        }
    }
    else if (strncmp(buffer, "/leave", 6) == 0) {
        if (client_groups[client_index] != -1) {
            int group_index = client_groups[client_index];
            for (int m = 0; m < groups[group_index].member_count; m++) {
                if (groups[group_index].members[m] == client_sockets[client_index]) {
                    groups[group_index].members[m] = groups[group_index].members[--groups[group_index].member_count];
                    break;
                }
            }
            // Usuń pustą grupę
            if (groups[group_index].member_count == 0) {
                groups[group_index] = groups[--group_count];
            }
            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), "%s left group %s\n", client_nicks[client_index], groups[group_index].group_name);
            broadcast_group(group_index, message, client_index);
            send(client_sockets[client_index], "You left the group.\n", strlen("You left the group.\n"), 0);
            client_groups[client_index] = -1;
        } else {
            send(client_sockets[client_index], "You are not in any group!\n", strlen("You are not in any group!\n"), 0);
        }
    }
    else if (strncmp(buffer, "/msg ", 5) == 0) {
        char target_nick[50], message[BUFFER_SIZE];
        sscanf(buffer + 5, "%49s %[^\n]", target_nick, message);

        int target_index = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (strcmp(client_nicks[i], target_nick) == 0) {
                target_index = i;
                break;
            }
        }

        if (target_index != -1 && client_sockets[target_index] != 0) {
            char formatted_message[BUFFER_SIZE + 50];
            snprintf(formatted_message, sizeof(formatted_message), "Private from %s: %s\n", client_nicks[client_index], message);
            send(client_sockets[target_index], formatted_message, strlen(formatted_message), 0);
        } else {
            send(client_sockets[client_index], "User not found!\n", strlen("User not found!\n"), 0);
        }
    }
    else if (strncmp(buffer, "/list", 5) == 0) {
        char user_list[BUFFER_SIZE] = "Online users:\n";
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] != 0) {
                char user_info[100];
                snprintf(user_info, sizeof(user_info), "%s (Group: %s)\n", client_nicks[i],
                         client_groups[i] == -1 ? "None" : groups[client_groups[i]].group_name);
                strcat(user_list, user_info);
            }
        }
        send(client_sockets[client_index], user_list, strlen(user_list), 0);
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    int addrlen = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    WSAStartup(MAKEWORD(2, 2), &wsa);
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 3);

    printf("Server running on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        SOCKET max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        select(0, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    strcpy(client_nicks[i], "User");
                    client_groups[i] = -1;
                    printf("New client %d connected as User\n", i);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = client_sockets[i];
            if (sd && FD_ISSET(sd, &readfds)) {
                int valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);
                if (valread <= 0) {
                    printf("Client %d disconnected\n", i);
                    closesocket(client_sockets[i]);
                    client_sockets[i] = 0;
                    strcpy(client_nicks[i], "");
                    if (client_groups[i] != -1) {
                        int group_index = client_groups[i];
                        for (int m = 0; m < groups[group_index].member_count; m++) {
                            if (groups[group_index].members[m] == sd) {
                                groups[group_index].members[m] = groups[group_index].members[--groups[group_index].member_count];
                                // Usuń pustą grupę
                                if (groups[group_index].member_count == 0) {
                                    groups[group_index] = groups[--group_count];
                                }
                                break;
                            }
                        }
                        client_groups[i] = -1;
                    }
                } else {
                    buffer[valread] = '\0';
                    if (buffer[0] == '/') {
                        handle_command(i, buffer);
                    } else {
                        char message[BUFFER_SIZE + 50];
                        snprintf(message, sizeof(message), "%s: %s", client_nicks[i], buffer);
                        if (client_groups[i] != -1) {
                            broadcast_group(client_groups[i], message, i);
                        } else {
                            broadcast_all(i, message);
                        }
                    }
                }
            }
        }
    }

    WSACleanup();
    return 0;
}