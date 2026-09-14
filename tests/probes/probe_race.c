// probe_race: stress window map/withdraw/destroy for BadWindow Unmap.
// Usage: probe_race.exe [iterations]
// Prints iterations completed. Any X Error lines in stderr = repro.
#include <windows.h>
#include <stdio.h>
#include <string.h>

static DWORD WINAPI churn(LPVOID p)
{
    int n = (int)(INT_PTR)p;
    for (int i = 0; i < n; i++)
    {
        HWND w = CreateWindowExA(0, "STATIC", "race", WS_POPUP,
                                 0, 0, 64, 64, NULL, NULL, NULL, NULL);
        if (!w) { printf("create failed %lu\n", GetLastError()); return 1; }
        ShowWindow(w, SW_SHOWNA);
        UpdateWindow(w);
        ShowWindow(w, SW_HIDE);
        DestroyWindow(w);
    }
    return 0;
}

static HWND shared[512];
static volatile LONG share_count;
static volatile BOOL stop_feed;

static DWORD WINAPI feeder(LPVOID p)
{
    int n = (int)(INT_PTR)p;
    for (int i = 0; i < n; i++)
    {
        HWND w = CreateWindowExA(0, "STATIC", "race", WS_POPUP,
                                 0, 0, 64, 64, NULL, NULL, NULL, NULL);
        if (!w) continue;
        ShowWindow(w, SW_SHOWNA);
        LONG idx = InterlockedIncrement(&share_count) - 1;
        if (idx < 512) shared[idx] = w;
        else DestroyWindow(w);
        if (stop_feed) break;
    }
    return 0;
}

static DWORD WINAPI reaper(LPVOID p)
{
    int n = (int)(INT_PTR)p;
    int done = 0;
    while (done < n)
    {
        LONG c = share_count;
        for (LONG i = 0; i < c && i < 512; i++)
        {
            HWND w = (HWND)InterlockedExchangePointer((PVOID *)&shared[i], NULL);
            if (w) { ShowWindow(w, SW_HIDE); DestroyWindow(w); done++; }
        }
        SwitchToThread();
    }
    stop_feed = TRUE;
    return 0;
}

int main(int argc, char **argv)
{
    int iters = argc > 1 ? atoi(argv[1]) : 500;
    int xthread = argc > 2 && !strcmp(argv[2], "x");
    HANDLE t[4];
    if (xthread) {
        t[0] = CreateThread(NULL, 0, feeder, (LPVOID)(INT_PTR)iters, 0, NULL);
        t[1] = CreateThread(NULL, 0, reaper, (LPVOID)(INT_PTR)iters, 0, NULL);
        WaitForMultipleObjects(2, t, TRUE, INFINITE);
    } else {
        for (int i = 0; i < 4; i++)
            t[i] = CreateThread(NULL, 0, churn, (LPVOID)(INT_PTR)iters, 0, NULL);
        WaitForMultipleObjects(4, t, TRUE, INFINITE);
    }
    printf("RACE_DONE iters=%d xthread=%d\n", iters, xthread);
    return 0;
}
