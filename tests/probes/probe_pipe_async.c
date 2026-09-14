// probe_pipe_async: overlapped named-pipe echo stress, N round-trips.
// Usage: probe_pipe_async.exe server <pipename> <n> | client <pipename> <n>
// Exit 0 iff every round-trip echoed intact within 5s each.
#include <windows.h>
#include <stdio.h>
#include <string.h>

static int wait_ov(HANDLE h, OVERLAPPED *ov, DWORD *n, DWORD to) {
    DWORD w = WaitForSingleObject(ov->hEvent, to);
    if (w != WAIT_OBJECT_0) { CancelIo(h); return 0; }
    return GetOverlappedResult(h, ov, n, FALSE);
}

int main(int argc, char **argv) {
    if (argc < 4) { printf("usage: server|client pipename n [bytes]\n"); return 2; }
    int N = atoi(argv[3]);
    int SZ = argc > 4 ? atoi(argv[4]) : 10;
    if (SZ < 10 || SZ > 1000000) { printf("bad size\n"); return 2; }
    char name[256];
    snprintf(name, sizeof name, "\\\\.\\pipe\\%s", argv[2]);
    if (!strcmp(argv[1], "server")) {
        HANDLE h = CreateNamedPipeA(name, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                    PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                    1, 4096, 4096, 5000, NULL);
        if (h == INVALID_HANDLE_VALUE) { printf("create gle=%lu\n", GetLastError()); return 1; }
        OVERLAPPED cov = {0}; cov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
        ConnectNamedPipe(h, &cov);
        DWORD w = WaitForSingleObject(cov.hEvent, 30000);
        if (w != WAIT_OBJECT_0) { printf("accept TIMEOUT\n"); return 1; }
        int bad = 0;
        for (int i = 0; i < N; i++) {
            static char msg[1000064], back[1000064];
            snprintf(msg, 32, "ping-%06d:", i);
            for (int k = (int)strlen(msg); k < SZ; k++) msg[k] = (char)(i + k);
            DWORD n = 0;
            OVERLAPPED rov = {0}; rov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
            OVERLAPPED wov = {0}; wov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
            BOOL ok = ReadFile(h, back, sizeof back, NULL, &rov);
            if (!ok && GetLastError() != ERROR_IO_PENDING) { printf("read issue gle=%lu\n", GetLastError()); bad++; break; }
            if (!wait_ov(h, &rov, &n, 5000)) { printf("round %d READ TIMEOUT\n", i); bad++; CloseHandle(rov.hEvent); CloseHandle(wov.hEvent); break; }
            DWORD m = 0;
            ok = WriteFile(h, back, n, NULL, &wov);
            if (!ok && GetLastError() != ERROR_IO_PENDING) { printf("write issue gle=%lu\n", GetLastError()); bad++; CloseHandle(rov.hEvent); CloseHandle(wov.hEvent); break; }
            if ((int)n != SZ || memcmp(back, msg, n)) { printf("round %d MISMATCH n=%lu\n", i, n); bad++; }
            CloseHandle(rov.hEvent); CloseHandle(wov.hEvent);
        }
        printf("server done bad=%d/%d\n", bad, N);
        CloseHandle(cov.hEvent); CloseHandle(h);
        return bad ? 1 : 0;
    } else {
        Sleep(500);
        HANDLE h = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                               OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
        if (h == INVALID_HANDLE_VALUE) { printf("open gle=%lu\n", GetLastError()); return 1; }
        int bad = 0;
        for (int i = 0; i < N; i++) {
            static char msg[1000064], back[1000064];
            snprintf(msg, 32, "ping-%06d:", i);
            for (int k = (int)strlen(msg); k < SZ; k++) msg[k] = (char)(i + k);
            DWORD m = 0, n = 0;
            OVERLAPPED wov = {0}; wov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
            OVERLAPPED rov = {0}; rov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
            BOOL ok = WriteFile(h, msg, SZ, NULL, &wov);
            if (!ok && GetLastError() != ERROR_IO_PENDING) { printf("write issue gle=%lu\n", GetLastError()); bad++; break; }
            if (!wait_ov(h, &wov, &m, 5000)) { printf("round %d WRITE TIMEOUT\n", i); bad++; break; }
            ok = ReadFile(h, back, SZ, NULL, &rov);
            if (!ok && GetLastError() != ERROR_IO_PENDING) { printf("read issue gle=%lu\n", GetLastError()); bad++; break; }
            if (!wait_ov(h, &rov, &n, 5000)) { printf("round %d READ TIMEOUT\n", i); bad++; break; }
            if ((int)n != SZ || memcmp(back, msg, n)) { printf("round %d MISMATCH n=%lu\n", i, n); bad++; }
            CloseHandle(wov.hEvent); CloseHandle(rov.hEvent);
        }
        printf("client done bad=%d/%d\n", bad, N);
        CloseHandle(h);
        return bad ? 1 : 0;
    }
}
