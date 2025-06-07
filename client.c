#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUFFER_SIZE 1024

char current_group[50] = "None"; // Śledzenie bieżącej grupy
char client_nick[50] = "User";   // Śledzenie aktualnego nicku

// Funkcja wątku do odbierania wiadomości
DWORD WINAPI receive_thread(LPVOID socket_ptr) {
    SOCKET sock = *(SOCKET *)socket_ptr;
    char buffer[BUFFER_SIZE];

    while (1) {
        int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (valread <= 0) {
            printf("Disconnected from server.\n");
            closesocket(sock);
            exit(1);
        }
        buffer[valread] = '\0';
        // Aktualizuj nazwę grupy na podstawie wiadomości od serwera
        if (strncmp(buffer, "You joined group ", 16) == 0) {
            sscanf(buffer + 16, "%49s", current_group);
            current_group[strcspn(current_group, "\n")] = '\0';
            printf("Joined group: %s\n", current_group);
        } else if (strncmp(buffer, "You left the group.", 19) == 0) {
            strcpy(current_group, "None");
            printf("Left group.\n");
        } else {
            printf("%s", buffer);
        }
        fflush(stdout);
    }
    return 0;
}

// Funkcja do parsowania i wysyłania komend
void handle_command(SOCKET sock, char *message) {
    if (strncmp(message, "/nick ", 6) == 0) {
        sscanf(message + 6, "%49s", client_nick); // Aktualizuj lokalny nick
        int sent = send(sock, message, strlen(message), 0);
        if (sent == SOCKET_ERROR) {
            printf("Failed to send nickname change: %d\n", WSAGetLastError());
        } else {
            printf("Nickname change requested.\n");
        }
    }
    else if (strncmp(message, "/join ", 6) == 0) {
        int sent = send(sock, message, strlen(message), 0);
        if (sent == SOCKET_ERROR) {
            printf("Failed to send join request: %d\n", WSAGetLastError());
        } else {
            printf("Join group requested.\n");
        }
    }
    else if (strncmp(message, "/leave", 6) == 0) {
        int sent = send(sock, message, strlen(message), 0);
        if (sent == SOCKET_ERROR) {
            printf("Failed to send leave request: %d\n", WSAGetLastError());
        } else {
            printf("Leave group requested.\n");
        }
    }
    else if (strncmp(message, "/msg ", 5) == 0) {
        int sent = send(sock, message, strlen(message), 0);
        if (sent == SOCKET_ERROR) {
            printf("Failed to send private message: %d\n", WSAGetLastError());
        } else {
            printf("Private message sent.\n");
        }
    }
    else if (strcmp(message, "/list\n") == 0) {
        int sent = send(sock, message, strlen(message), 0);
        if (sent == SOCKET_ERROR) {
            printf("Failed to send list request: %d\n", WSAGetLastError());
        } else {
            printf("User list requested.\n");
        }
    }
    else if (strcmp(message, "/help\n") == 0) {
        int sent = send(sock, message, strlen(message), 0);
        if (sent == SOCKET_ERROR) {
            printf("Failed to send help request: %d\n", WSAGetLastError());
        } else {
            printf("Help requested.\n");
        }
    }
    else {
        int sent = send(sock, message, strlen(message), 0);
        if (sent == SOCKET_ERROR) {
            printf("Failed to send message: %d\n", WSAGetLastError());
        }
    }
}

int main()
{
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];

    // Inicjalizacja Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Tworzenie gniazda
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Konfiguracja adresu serwera
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Połączenie z serwerem
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Uruchomienie wątku do odbierania wiadomości
    CreateThread(NULL, 0, receive_thread, &sock, 0, NULL);

    printf("Connected to the server. Enter your first message or type /help to see possible actions.\n");

    // Główna pętla klienta
    while (1) {
        printf("[%s] ", current_group);
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            printf("Input error.\n");
            break;
        }

        // Usuń znak nowej linii, jeśli istnieje
        message[strcspn(message, "\n")] = '\n';

        // Sprawdź, czy wiadomość to komenda
        handle_command(sock, message);
    }

    // Zamknięcie gniazda i czyszczenie
    closesocket(sock);
    WSACleanup();
    return 0;
}