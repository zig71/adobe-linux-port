// probe_http: time one HTTPS GET via WinINET. Same PE both platforms.
// Usage: probe_http.exe <url>
#include <windows.h>
#include <wininet.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) { printf("usage: url\n"); return 2; }
    DWORD t0 = GetTickCount();
    HINTERNET inet = InternetOpenA("probe", INTERNET_OPEN_TYPE_PRECONFIG,
                                   NULL, NULL, 0);
    if (!inet) { printf("open gle=%lu\n", GetLastError()); return 1; }
    HINTERNET url = InternetOpenUrlA(inet, argv[1], NULL, 0,
                                     INTERNET_FLAG_RELOAD |
                                     INTERNET_FLAG_SECURE, 0);
    if (!url) {
        printf("fetch FAILED gle=%lu ms=%lu\n", GetLastError(),
               GetTickCount() - t0);
        InternetCloseHandle(inet);
        return 1;
    }
    char buf[8192];
    DWORD n = 0, total = 0;
    while (InternetReadFile(url, buf, sizeof buf, &n) && n) total += n;
    printf("bytes=%lu ms=%lu\n", total, GetTickCount() - t0);
    InternetCloseHandle(url);
    InternetCloseHandle(inet);
    return 0;
}
