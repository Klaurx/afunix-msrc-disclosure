#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

struct sockaddr_un {
    unsigned short sun_family;
    char sun_path[108];
};

#define SOCK_PATH "C:\\Temp\\probe.sock"

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
    CreateDirectoryA("C:\\Temp", NULL);
    DeleteFileA(SOCK_PATH);

    SOCKET srv = WSASocketA(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path)-1);
    bind(srv, (struct sockaddr*)&addr, sizeof(addr));
    listen(srv, 1);

    SOCKET cli = WSASocketA(AF_UNIX, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    connect(cli, (struct sockaddr*)&addr, sizeof(addr));
    SOCKET conn = accept(srv, NULL, NULL);

    printf("conn handle: %llu\n", (UINT64)conn);
    printf("Attach WinDbg now, arm bp on AfUnixDeliverDataToClient, then press enter\n");
    getchar();

    printf("Trying shutdown(conn, SD_RECEIVE)...\n");
    int r = shutdown(conn, SD_RECEIVE);
    printf("shutdown returned %d, WSAGetLastError=%d\n", r, WSAGetLastError());

    printf("Press enter to exit\n");
    getchar();
    return 0;
}