/* probe_wv2_internal.c — replicate the Creative Cloud installer's WebView2 path.
 *
 * Established by analysis of the unpacked bootstrapper:
 *   - it contains the string "CreateWebViewEnvironmentWithOptionsInternal"
 *   - it contains no reference to "WebView2Loader.dll" or the public
 *     "CreateCoreWebView2EnvironmentWithOptions"
 * so it drives WebView2 through the runtime's own internal export rather than
 * the public loader:
 *
 *   1. read HKLM\SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-...} -> pv, location
 *   2. LoadLibraryW( "<location>\<pv>\EBWebView\x86\EmbeddedBrowserWebView.dll" )
 *   3. CreateWebViewEnvironmentWithOptionsInternal( NULL, userDataFolder,
 *                                                   NULL, completedHandler )
 *
 * Steps 1 and 2 were measured to work under Wine once the runtime is staged and
 * registered. This probe performs all three and reports the HRESULT from step 3
 * and whether a browser process results, which is the question that decides
 * whether the genuine runtime can be hosted by Wine.
 *
 * Build 32-bit, matching the installer:
 *   i686-w64-mingw32-gcc -O2 -o probe_wv2_internal.exe probe_wv2_internal.c \
 *       -lole32 -ladvapi32 -luuid
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define WV2_GUID L"{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"

static void w2a(const WCHAR *w, char *o, int n) {
    int i;
    for (i = 0; i < n - 1 && w[i]; i++) o[i] = (w[i] < 128) ? (char)w[i] : '?';
    o[i] = 0;
}

/* The completed handler the environment creation calls back into. */
typedef struct Handler {
    void **lpVtbl;
    LONG ref;
} Handler;

static volatile LONG handler_called = 0;
static volatile HRESULT handler_hr = 0;
static volatile LONG handler_env_nonnull = 0;

static HRESULT WINAPI H_QueryInterface(void *iface, const void *riid, void **ppv) {
    (void)riid; *ppv = iface; return S_OK;
}
static ULONG WINAPI H_AddRef(void *iface) { return (ULONG)InterlockedIncrement(&((Handler *)iface)->ref); }
static ULONG WINAPI H_Release(void *iface) { return (ULONG)InterlockedDecrement(&((Handler *)iface)->ref); }
static HRESULT WINAPI H_Invoke(void *iface, HRESULT error, void *env) {
    (void)iface;
    handler_hr = error;
    handler_env_nonnull = env != NULL;
    InterlockedExchange(&handler_called, 1);
    printf("HANDLER_called=1 error=0x%08lx env_nonnull=%d\n",
           (unsigned long)error, env != NULL);
    return S_OK;
}
static void *handler_vtbl[] = { (void *)H_QueryInterface, (void *)H_AddRef,
                                (void *)H_Release, (void *)H_Invoke };

/* The runtime's internal entry point takes five arguments; its epilogue is
 * `ret 0x14` and it reads [ebp+8] through [ebp+0x18]. This matches the
 * argument list in Wine's MR 7032 stub:
 *     (UINT, PCWSTR browserExecutableFolder, PCWSTR userDataFolder,
 *      ICoreWebView2EnvironmentOptions *, handler)
 * Passing four arguments, as an earlier version of this probe did, makes the
 * callee read a fifth from unused stack and clean more than was pushed. */
typedef HRESULT (STDAPICALLTYPE *CreateEnvInternal_t)(UINT, const WCHAR *, const WCHAR *,
                                                      void *, void *);

int main(void) {
    WCHAR key[512], pv[64], location[MAX_PATH], dll[MAX_PATH * 2];
    WCHAR userdata[MAX_PATH];
    HKEY hk;
    DWORD type = 0, size;
    LONG r;
    char ascii[MAX_PATH * 2];
    HMODULE mod;
    CreateEnvInternal_t create_env;
    Handler handler;
    HRESULT hr;
    int i;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));

    /* --- step 1: the registry probe the installer performs ------------- */
    lstrcpyW(key, L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\" WV2_GUID);
    r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hk);
    printf("STEP1_open=0x%08lx (%s)\n", (unsigned long)r,
           r == ERROR_SUCCESS ? "found" : "missing");
    if (r != ERROR_SUCCESS) { printf("PROBE_RESULT=NO_RUNTIME\n"); return 2; }

    size = sizeof(pv);
    r = RegQueryValueExW(hk, L"pv", NULL, &type, (BYTE *)pv, &size);
    printf("STEP1_pv_read=0x%08lx\n", (unsigned long)r);
    if (r != ERROR_SUCCESS) { RegCloseKey(hk); printf("PROBE_RESULT=NO_PV\n"); return 2; }
    w2a(pv, ascii, sizeof(ascii));
    printf("STEP1_pv=%s\n", ascii);

    size = sizeof(location);
    r = RegQueryValueExW(hk, L"location", NULL, &type, (BYTE *)location, &size);
    printf("STEP1_location_read=0x%08lx\n", (unsigned long)r);
    RegCloseKey(hk);
    if (r != ERROR_SUCCESS) { printf("PROBE_RESULT=NO_LOCATION\n"); return 2; }
    w2a(location, ascii, sizeof(ascii));
    printf("STEP1_location=%s\n", ascii);

    /* --- step 2: load the runtime's internal host DLL ------------------- */
    /* Dependencies of a loaded DLL resolve from the *executable's* directory,
     * not the DLL's own, so the runtime directory has to be added to the
     * search path first. This is what the real loader does before loading the
     * host DLL. */
    {
        WCHAR rt[MAX_PATH * 2];
        _snwprintf(rt, MAX_PATH * 2, L"%s\\%s", location, pv);
        SetDllDirectoryW(rt);
        w2a(rt, ascii, sizeof(ascii));
        printf("STEP2_dll_directory=%s\n", ascii);
    }
    for (i = 0; i < 2; i++) {
        const WCHAR *arch = i == 0 ? L"x86" : L"x64";
        _snwprintf(dll, MAX_PATH * 2, L"%s\\%s\\EBWebView\\%s\\EmbeddedBrowserWebView.dll",
                   location, pv, arch);
        w2a(dll, ascii, sizeof(ascii));
        printf("STEP2_path_%ls=%s\n", arch, ascii);
        SetLastError(0);
        mod = LoadLibraryW(dll);
        printf("STEP2_load_%ls_ok=%d err=%lu\n", arch, mod != NULL,
               (unsigned long)GetLastError());
        if (mod) break;
    }
    if (!mod) { printf("PROBE_RESULT=NO_HOST_DLL\n"); return 3; }

    create_env = (CreateEnvInternal_t)(void *)
        GetProcAddress(mod, "CreateWebViewEnvironmentWithOptionsInternal");
    printf("STEP2_export_present=%d\n", create_env != NULL);
    if (!create_env) { printf("PROBE_RESULT=NO_EXPORT\n"); return 3; }

    /* --- step 3: create the environment --------------------------------- */
    GetTempPathW(MAX_PATH, userdata);
    lstrcatW(userdata, L"wv2probe");
    w2a(userdata, ascii, sizeof(ascii));
    printf("STEP3_userdata=%s\n", ascii);

    handler.lpVtbl = handler_vtbl;
    handler.ref = 1;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    printf("STEP3_CoInitializeEx=0x%08lx\n", (unsigned long)hr);

    SetLastError(0);
    hr = create_env(0, NULL, userdata, NULL, (void *)&handler);
    printf("STEP3_CreateEnv_hr=0x%08lx\n", (unsigned long)hr);
    printf("STEP3_CreateEnv_win32err=%lu\n", (unsigned long)GetLastError());

    /* The call is asynchronous; pump so the handler can be invoked. */
    for (i = 0; i < 300 && !handler_called; i++) {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(100);
        if (i == 30) printf("STEP3_waiting_3s\n");
    }
    printf("STEP3_handler_called=%ld\n", (long)handler_called);
    if (handler_called) {
        printf("STEP3_handler_hr=0x%08lx\n", (unsigned long)handler_hr);
        printf("STEP3_env_created=%d\n", (int)handler_env_nonnull);
    }

    /* --- observable side effect: did the runtime write its user data dir? */
    {
        DWORD attrs = GetFileAttributesW(userdata);
        printf("STEP3_userdata_dir_created=%d\n",
               attrs != INVALID_FILE_ATTRIBUTES ? 1 : 0);
    }

    CoUninitialize();
    printf("PROBE_RESULT=%s\n",
           (handler_called && handler_env_nonnull) ? "ENVIRONMENT_CREATED" : "ENV_NOT_CREATED");
    return 0;
}
