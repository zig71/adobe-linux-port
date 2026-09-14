// probe_pipe: named-pipe server/client handshake across two processes.
// Usage: probe_pipe.exe server <pipename> | client <pipename>
// Prints each step + GetLastError. Exit 0 on full echo round-trip.
#include <windows.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) { printf("usage: server|client pipename\n"); return 2; }
    char name[256];
    snprintf(name, sizeof name, "\\\\.\\pipe\\%s", argv[2]);
    if (!strcmp(argv[1], "server")) {
        HANDLE h = CreateNamedPipeA(name, PIPE_ACCESS_DUPLEX,
                                    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                    1, 512, 512, 5000, NULL);
        printf("create -> %p gle=%lu\n", h, GetLastError());
        if (h == INVALID_HANDLE_VALUE) return 1;
        BOOL ok = ConnectNamedPipe(h, NULL);
        printf("connect(server accept) -> %d gle=%lu\n", ok, GetLastError());
        char buf[64] = {0};
        DWORD n = 0;
        ok = ReadFile(h, buf, sizeof buf - 1, &n, NULL);
        printf("read -> %d gle=%lu n=%lu msg=%.20s\n", ok, GetLastError(), n, buf);
        DWORD w = 0;
        ok = WriteFile(h, buf, n, &w, NULL);
        printf("write(echo) -> %d gle=%lu w=%lu\n", ok, GetLastError(), w);
        FlushFileBuffers(h);
        DisconnectNamedPipe(h);
        CloseHandle(h);
        return 0;
    } else {
        Sleep(500);
        HANDLE h = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, 0, NULL);
        printf("open(client) -> %p gle=%lu\n", h, GetLastError());
        if (h == INVALID_HANDLE_VALUE) return 1;
        DWORD mode = PIPE_READMODE_MESSAGE;
        SetNamedPipeHandleState(h, &mode, NULL, NULL);
        DWORD w = 0;
        BOOL ok = WriteFile(h, "hello-pipe", 10, &w, NULL);
        printf("write -> %d gle=%lu w=%lu\n", ok, GetLastError(), w);
        char buf[64] = {0};
        DWORD n = 0;
        ok = ReadFile(h, buf, sizeof buf - 1, &n, NULL);
        printf("read(echo) -> %d gle=%lu n=%lu msg=%.20s\n", ok, GetLastError(), n, buf);
        CloseHandle(h);
        return (ok && n == 10) ? 0 : 1;
    }
}
