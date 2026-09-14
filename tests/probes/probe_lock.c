// probe_lock: time LockFileEx/UnlockFileEx round-trips (SQLite's substrate).
// Same PE both platforms. Prints per-op microseconds.
#include <windows.h>
#include <stdio.h>

int main(void) {
    HANDLE h = CreateFileA("locktest.tmp", GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           CREATE_ALWAYS, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        printf("create gle=%lu\n", GetLastError());
        return 1;
    }
    OVERLAPPED ov = {0};
    const int N = 200;
    DWORD t0 = GetTickCount();
    for (int i = 0; i < N; i++) {
        ov.Offset = (i % 8) * 1024;
        if (!LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK, 0, 1024, 0, &ov)) {
            printf("lock %d gle=%lu\n", i, GetLastError());
            return 1;
        }
        if (!UnlockFileEx(h, 0, 1024, 0, &ov)) {
            printf("unlock %d gle=%lu\n", GetLastError());
            return 1;
        }
    }
    DWORD dt = GetTickCount() - t0;
    printf("lockops=%d total_ms=%lu per_op_us=%lu\n", N, dt,
           (dt * 1000) / N);
    CloseHandle(h);
    DeleteFileA("locktest.tmp");
    return 0;
}
