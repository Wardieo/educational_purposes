#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char buf[1024], cmd[16], arg[1000];

    WSAStartup(MAKEWORD(2,2), &wsa);

    s = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr("192.168.1.11"); // Change to server IP if needed
    server.sin_family = AF_INET;
    server.sin_port = htons(9001);

    if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
        // No console output
        return 1;
    }

    while (1) {
        int len = recv(s, buf, sizeof(buf)-1, 0);
        if (len <= 0) break;
        buf[len] = 0;

        sscanf(buf, "%15s %999[^\n]", cmd, arg);

        if (_stricmp(cmd, "PRINT") == 0) {
            // Only show message box, no console
            MessageBoxA(NULL, arg, "Server Message", MB_OK | MB_ICONINFORMATION);
            send(s, "Printed OK", 10, 0);
        } else if (_stricmp(cmd, "MSGBOX") == 0) {
            MessageBoxA(NULL, arg, "Server Message", MB_OK | MB_ICONINFORMATION);
            send(s, "MessageBox OK", 13, 0);
        } else if (_stricmp(cmd, "EXIT") == 0) {
            send(s, "Bye", 3, 0);
            break;
        } else {
            send(s, "Unknown command", 15, 0);
        }
    }

    closesocket(s);
    WSACleanup();
    return 0;
}