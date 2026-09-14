/* probe_webview2.c — Microsoft Edge WebView2 runtime availability.
 *
 * Why: the Creative Cloud bootstrapper (Creative_Cloud_Set-Up.exe) is a 32-bit
 * native Win32 host for Microsoft Edge WebView2. Its whole UI -- including the
 * sign-in and product-picker screens -- lives in that browser. Before it can
 * render anything it must locate a WebView2 runtime and create an environment.
 *
 * This probe performs exactly that discovery, in the same order the WebView2
 * loader does, so the first behavioural divergence can be stated as
 * "Windows returns X, Wine returns Y" rather than inferred from a failure.
 *
 * Build 32-bit, matching the installer:
 *   i686-w64-mingw32-gcc -O2 -o probe_webview2.exe probe_webview2.c -lole32 -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

/* {F3017226-FE2A-4295-8BDF-00C3A9A7E4C5} — the WebView2 Runtime's EdgeUpdate
 * client id. The loader looks this up to find the installed runtime. */
static const WCHAR *WV2_CLIENT_KEY =
    L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}";

static void p(const char *k, const char *v) { printf("%s=%s\n", k, v ? v : "(null)"); }

static void ascii_from_wide(const WCHAR *w, char *out, int n) {
    int i;
    for (i = 0; i < n - 1 && w[i]; i++) out[i] = (w[i] < 128) ? (char)w[i] : '?';
    out[i] = 0;
}

/* --- registry discovery, in both views ---------------------------------- */
static void check_key(const char *label, HKEY root, REGSAM view) {
    HKEY hk;
    DWORD type = 0, size = 0;
    LONG r;
    char buf[512];

    r = RegOpenKeyExW(root, WV2_CLIENT_KEY, 0, KEY_READ | view, &hk);
    printf("%s_open=%s (%ld)\n", label, r == ERROR_SUCCESS ? "found" : "missing", (long)r);
    if (r != ERROR_SUCCESS) return;

    size = sizeof(buf);
    r = RegQueryValueExW(hk, L"pv", NULL, &type, (BYTE *)buf, &size);
    if (r == ERROR_SUCCESS) {
        char out[256];
        ascii_from_wide((WCHAR *)buf, out, sizeof(out));
        printf("%s_pv=%s\n", label, out);
        printf("%s_pv_type=%lu\n", label, (unsigned long)type);
    } else {
        printf("%s_pv=(missing) (%ld)\n", label, (long)r);
    }

    size = sizeof(buf);
    r = RegQueryValueExW(hk, L"location", NULL, &type, (BYTE *)buf, &size);
    if (r == ERROR_SUCCESS) {
        char out[512];
        ascii_from_wide((WCHAR *)buf, out, sizeof(out));
        printf("%s_location=%s\n", label, out);
    } else {
        printf("%s_location=(missing)\n", label);
    }

    size = sizeof(buf);
    r = RegQueryValueExW(hk, L"name", NULL, &type, (BYTE *)buf, &size);
    if (r == ERROR_SUCCESS) {
        char out[256];
        ascii_from_wide((WCHAR *)buf, out, sizeof(out));
        printf("%s_name=%s\n", label, out);
    }

    RegCloseKey(hk);
}

/* --- a minimal ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler -
 * Needed so the environment-creation call can be answered without crashing.
 * Invoke() aborts, which makes the runtime tear the environment down again. */
typedef struct Handler {
    void **lpVtbl;
    LONG ref;
} Handler;

static HRESULT WINAPI H_QueryInterface(void *iface, const void *riid, void **ppv) {
    (void)riid;
    *ppv = iface;
    return S_OK;
}
static ULONG WINAPI H_AddRef(void *iface) { return (ULONG)InterlockedIncrement(&((Handler *)iface)->ref); }
static ULONG WINAPI H_Release(void *iface) { return (ULONG)InterlockedDecrement(&((Handler *)iface)->ref); }
/* Invoke(HRESULT errorCode, ICoreWebView2Environment *createdEnvironment) */
static HRESULT WINAPI H_Invoke(void *iface, HRESULT error, void *env) {
    (void)iface; (void)env;
    printf("Handler_Invoke_called=1 error=0x%08lx\n", (unsigned long)error);
    return E_ABORT;
}
static void *handler_vtbl[] = { (void *)H_QueryInterface, (void *)H_AddRef,
                                (void *)H_Release, (void *)H_Invoke };

typedef HRESULT (STDAPICALLTYPE *CreateEnv_t)(const WCHAR *, const WCHAR *, void *, void *);
typedef HRESULT (STDAPICALLTYPE *GetVersion_t)(const WCHAR *, WCHAR **);

int main(void) {
    char buf[MAX_PATH * 2];
    WCHAR wbuf[MAX_PATH];
    HMODULE loader;
    CreateEnv_t create_env;
    GetVersion_t get_version;
    Handler handler;
    HRESULT hr;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));

    /* --- registry: default (redirected) and explicit 64-bit view ---------- */
    check_key("HKLM_default", HKEY_LOCAL_MACHINE, 0);
    check_key("HKLM_64view", HKEY_LOCAL_MACHINE, KEY_WOW64_64KEY);
    check_key("HKLM_32view", HKEY_LOCAL_MACHINE, KEY_WOW64_32KEY);

    /* --- runtime directory on disk ---------------------------------------- */
    {
        static const WCHAR *dirs[] = {
            L"C:\\Program Files (x86)\\Microsoft\\EdgeWebView\\Application",
            L"C:\\Program Files\\Microsoft\\EdgeWebView\\Application",
        };
        int i, j;
        for (i = 0; i < 2; i++) {
            char label[64];
            HANDLE h;
            WIN32_FIND_DATAW fd;
            WCHAR pat[MAX_PATH];
            int count = 0;
            sprintf(label, "RUNTIME_DIR%d", i);
            _snwprintf(pat, MAX_PATH, L"%s\\*", dirs[i]);
            h = FindFirstFileW(pat, &fd);
            if (h == INVALID_HANDLE_VALUE) {
                printf("%s=absent\n", label);
                continue;
            }
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                if (fd.cFileName[0] == L'.') continue;
                count++;
                if (count == 1) {
                    ascii_from_wide(fd.cFileName, buf, sizeof(buf));
                    printf("%s_first_version=%s\n", label, buf);
                }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
            printf("%s_version_dirs=%d\n", label, count);
        }
    }

    /* --- executable and library presence ---------------------------------- */
    {
        static const char *cands[] = {
            "C:\\Program Files (x86)\\Microsoft\\EdgeWebView\\Application\\153.0.4234.32\\msedgewebview2.exe",
            "C:\\Program Files (x86)\\Microsoft\\EdgeWebView\\Application\\153.0.4234.32\\EBWebView\\x86\\EmbeddedBrowserWebView.dll",
            "C:\\Program Files (x86)\\Microsoft\\EdgeWebView\\Application\\153.0.4234.32\\EBWebView\\x64\\EmbeddedBrowserWebView.dll",
        };
        const char *names[] = { "msedgewebview2_exe", "EmbeddedBrowserWebView_x86", "EmbeddedBrowserWebView_x64" };
        int i;
        for (i = 0; i < 3; i++) {
            MultiByteToWideChar(CP_ACP, 0, cands[i], -1, wbuf, MAX_PATH);
            printf("%s=%s\n", names[i], GetFileAttributesW(wbuf) == INVALID_FILE_ATTRIBUTES ? "absent" : "present");
        }
    }

    /* --- the WebView2 loader itself, which the app links against ---------- */
    loader = LoadLibraryW(L"WebView2Loader.dll");
    printf("LoadLibrary_WebView2Loader=%s\n", loader ? "ok" : "failed");
    if (!loader) {
        printf("GetLastError=%lu\n", (unsigned long)GetLastError());
        /* Try the well-known sibling locations the loader probes. */
        loader = LoadLibraryW(L"C:\\Program Files (x86)\\Microsoft\\EdgeWebView\\Application\\153.0.4234.32\\EBWebView\\x86\\WebView2Loader.dll");
        printf("LoadLibrary_WebView2Loader_fromruntime=%s\n", loader ? "ok" : "failed");
    }

    if (loader) {
        get_version = (GetVersion_t)(void *)GetProcAddress(loader, "GetAvailableCoreWebView2BrowserVersionString");
        printf("GetAvailableCoreWebView2BrowserVersionString_present=%d\n", get_version != NULL);
        if (get_version) {
            WCHAR *ver = NULL;
            hr = get_version(NULL, &ver);
            printf("GetAvailable_version_hr=0x%08lx\n", (unsigned long)hr);
            if (ver) {
                ascii_from_wide(ver, buf, sizeof(buf));
                printf("GetAvailable_version=%s\n", buf);
                CoTaskMemFree(ver);
            } else {
                printf("GetAvailable_version=(null)\n");
            }
        }

        create_env = (CreateEnv_t)(void *)GetProcAddress(loader, "CreateCoreWebView2EnvironmentWithOptions");
        printf("CreateCoreWebView2EnvironmentWithOptions_present=%d\n", create_env != NULL);
        if (create_env) {
            handler.lpVtbl = handler_vtbl;
            handler.ref = 1;
            /* NULL browser folder = use the installed runtime; NULL options. */
            hr = create_env(NULL, L"C:\\Users\\Public\\wv2probe", NULL, &handler);
            printf("CreateEnv_hr=0x%08lx\n", (unsigned long)hr);
            printf("CreateEnv_sts_win32=%lu\n", (unsigned long)HRESULT_CODE(hr));
        }
    } else {
        printf("WebView2_loader_unavailable=1\n");
    }

    printf("PROBE_RESULT=OK\n");
    return 0;
}
