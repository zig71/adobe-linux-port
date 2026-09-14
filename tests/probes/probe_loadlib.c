/* probe_loadlib.c — load a DLL and report exactly how it fails.
 *
 * Why: the genuine Microsoft Edge WebView2 runtime is shipped from the Windows
 * oracle, and the question is whether Wine can host it as-is. The full runtime
 * is 680 MB and its launcher exits with code 13 after loading only
 * msedge_elf.dll, never reaching msedge.dll (the Chromium core, 332 MB).
 *
 * This probe isolates the "can Wine load this binary at all" question from the
 * "does the whole WebView2 stack work" question: it loads a named DLL, and on
 * failure reports the Win32 error and, for a missing-dependency failure, which
 * imports could not be resolved.
 *
 * Usage: probe_loadlib.exe <path-to-dll>
 * Build for the architecture of the DLL:
 *   x86_64-w64-mingw32-gcc -O2 -o probe_loadlib.exe probe_loadlib.c
 *   i686-w64-mingw32-gcc   -O2 -o probe_loadlib32.exe probe_loadlib.c
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static void w2a(const WCHAR *w, char *o, int n) {
    int i;
    for (i = 0; i < n - 1 && w[i]; i++) o[i] = (w[i] < 128) ? (char)w[i] : '?';
    o[i] = 0;
}

int main(int argc, char **argv) {
    WCHAR path[MAX_PATH * 2];
    HMODULE h;
    DWORD err;
    char ascii[512];

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));
    if (argc < 2) { printf("PROBE_RESULT=NO_ARGUMENT\n"); return 2; }

    printf("TARGET_ascii=%s\n", argv[1]);
    MultiByteToWideChar(CP_ACP, 0, argv[1], -1, path, MAX_PATH * 2);

    SetLastError(0);
    h = LoadLibraryW(path);
    err = GetLastError();
    printf("LoadLibrary_ok=%d\n", h != NULL);
    printf("GetLastError=%lu\n", (unsigned long)err);

    if (h) {
        /* Report a couple of well-known WebView2 entry points if present. */
        static const char *syms[] = {
            "CreateCoreWebView2EnvironmentWithOptions",
            "GetAvailableCoreWebView2BrowserVersionString",
            "CreateCoreWebView2Environment",
        };
        int i;
        for (i = 0; i < 3; i++)
            printf("export_%s=%d\n", syms[i], GetProcAddress(h, syms[i]) != NULL);
        printf("PROBE_RESULT=OK\n");
        return 0;
    }

    /* On a missing-dependency failure, name the imports that could not resolve
     * by attempting each in turn. Report the first that fails. */
    {
        HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        printf("file_open=%d\n", f != INVALID_HANDLE_VALUE);
        if (f != INVALID_HANDLE_VALUE) {
            DWORD sz = GetFileSize(f, NULL);
            printf("file_size=%lu\n", (unsigned long)sz);
            CloseHandle(f);
        }
    }

    /* Resolving each import by hand is the reliable way to name the blocker. */
    if (argc >= 3) {
        int i;
        for (i = 2; i < argc; i++) {
            HMODULE dep;
            MultiByteToWideChar(CP_ACP, 0, argv[i], -1, path, MAX_PATH * 2);
            SetLastError(0);
            dep = LoadLibraryW(path);
            printf("dep_%s_ok=%d err=%lu\n", argv[i], dep != NULL,
                   (unsigned long)GetLastError());
            if (dep) FreeLibrary(dep);
        }
    }

    printf("PROBE_RESULT=LOAD_FAILED\n");
    return 3;
}
