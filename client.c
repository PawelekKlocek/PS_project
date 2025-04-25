#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUFFER_SIZE 1024

DWORD WINAPI receive_thread(LPVOID socket_ptr) {
    SOCKET sock = *(SOCKET *)socket_ptr;
    char buffer[BUFFER_SIZE];

    while (1) {
        int valread = recv(sock, buffer, BUFFER_SIZE, 0);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("%s", buffer);
        }
    }
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];

    WSAStartup(MAKEWORD(2, 2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&server, sizeof(server));

    CreateThread(NULL, 0, receive_thread, &sock, 0, NULL);

    printf("💬 Polaczono z serwerem. Wpisz wiadomość lub /nick <twoj_nick>\n");

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        send(sock, message, strlen(message), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
