// Compile on Windows: gcc client.c -o client.exe -lws2_32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#define SERVER_IP   "10.56.255.216"
#define SERVER_PORT 9100
#define MAX_LINE    8192
#define MAX_PORTS   1024

int probe(const char *target, int port) {
    int sockfd;
    struct sockaddr_in addr;
    int result = 0;
#ifdef _WIN32
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#endif
    if (sockfd < 0) return 0;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, target, &addr.sin_addr);
    // Set timeout
#ifdef _WIN32
    DWORD timeout = 300;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval timeout;
    timeout.tv_sec = 0; timeout.tv_usec = 300000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0)
        result = 1;
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
    return result;
}

// Minimal JSON parser for this protocol
void parse_ports(const char *json, int *ports, int *count) {
    *count = 0;
    const char *p = strstr(json, "\"ports\"");
    if (!p) return;
    p = strchr(p, '[');
    if (!p) return;
    p++;
    while (*p && *p != ']') {
        while (*p == ' ' || *p == ',') p++;
        if (*p >= '0' && *p <= '9') {
            ports[*count] = atoi(p);
            (*count)++;
            if (*count >= MAX_PORTS) break;
            while (*p >= '0' && *p <= '9') p++;
        } else {
            p++;
        }
    }
}

void parse_target(const char *json, char *target, size_t tlen) {
    const char *p = strstr(json, "\"target\"");
    if (!p) { target[0] = 0; return; }
    p = strchr(p, ':');
    if (!p) { target[0] = 0; return; }
    p++;
    while (*p == ' ' || *p == '\"') p++;
    char *q = target;
    while (*p && *p != '\"' && *p != ',' && (q - target) < (tlen - 1)) {
        *q++ = *p++;
    }
    *q = 0;
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid server IP\n");
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
        return 1;
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif
        return 1;
    }

    while (1) {
        char buf[MAX_LINE] = {0};
        char target[64] = {0};
        int ports[MAX_PORTS], port_count = 0, open_ports[MAX_PORTS], open_count = 0;

        // Read a line (JSON task)
        int idx = 0;
        while (idx < MAX_LINE - 1) {
#ifdef _WIN32
            int n = recv(sockfd, buf + idx, 1, 0);
#else
            int n = read(sockfd, buf + idx, 1);
#endif
            if (n <= 0) break;
            if (buf[idx] == '\n') { idx++; break; }
            idx += n;
        }
        if (idx == 0) break; // connection closed

        buf[idx] = 0;
        if (strlen(buf) < 10) continue; // ignore empty/short lines

        printf("Received command: %s\n", buf);

        // Parse target and ports
        parse_target(buf, target, sizeof(target));
        parse_ports(buf, ports, &port_count);

        if (target[0] == 0 || port_count == 0) {
            fprintf(stderr, "Malformed command or no ports specified.\n");
            continue;
        }

        // Scan ports
        open_count = 0;
        for (int i = 0; i < port_count; ++i) {
            if (probe(target, ports[i])) {
                open_ports[open_count++] = ports[i];
            }
        }

        // Prepare and send report
        char report[MAX_LINE];
        int pos = snprintf(report, sizeof(report),
            "{\"target\":\"%s\",\"scanned_count\":%d,\"open_ports\":[",
            target, port_count);
        for (int i = 0; i < open_count; ++i) {
            pos += snprintf(report + pos, sizeof(report) - pos, "%d%s",
                open_ports[i], (i < open_count - 1) ? "," : "");
        }
        snprintf(report + pos, sizeof(report) - pos, "]}\n");

#ifdef _WIN32
        send(sockfd, report, (int)strlen(report), 0);
#else
        write(sockfd, report, strlen(report));
#endif
        printf("Sending report: %s\n", report);
    }

#ifdef _WIN32
    closesocket(sockfd);
    WSACleanup();
#else
    close(sockfd);
#endif

    return 0;
}