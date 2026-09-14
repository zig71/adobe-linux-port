/* probe_wv2_controller.c — can Wine host a WebView2 CONTROLLER, not just an
 * environment?
 *
 * probe_wv2_internal established that CreateWebViewEnvironmentWithOptionsInternal
 * succeeds under Wine (S_OK, non-null environment) once the runtime is staged
 * and the browser runs in win7-compat mode. But hosting real UI needs one more
 * step the installer also takes: ICoreWebView2Environment::CreateCoreWebView2Controller
 * against a parent HWND, which is where composition attaches.
 *
 * Threading matters: CreateCoreWebView2Controller must be called on the UI
 * thread that owns the parent window (else UI_E_WRONG_THREAD 0x802A000C), so
 * the controller call is chained inside the environment-completed handler,
 * exactly as Microsoft's threading-model documentation prescribes.
 *
 * Reports the controller HRESULT and whether the controller is non-null on
 * Windows and under Wine. Same binary, both platforms.
 *
 * Build 32-bit, matching the installer:
 *   i686-w64-mingw32-gcc -O2 -o probe_wv2_controller.exe probe_wv2_controller.c \
 *       -lole32 -ladvapi32 -luuid -lgdi32 -luser32
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

/* --- completed-handler skeleton ---------------------------------------- */
typedef struct Handler {
    void **lpVtbl;
    LONG ref;
    volatile LONG called;
    volatile HRESULT hr;
    volatile void *obj;
    volatile DWORD thread;
    const char *tag;
    int is_env;
} Handler;

static HWND g_hwnd = NULL;
static Handler g_ctl_h;
static int g_started = 0;
static int g_ctl_started(void) { return InterlockedExchange(&g_started, 1); }

/* ICoreWebView2Environment slot 3: CreateCoreWebView2Controller. */
typedef HRESULT (STDAPICALLTYPE *CreateController_t)(void *, HWND, void *);

static HRESULT WINAPI H_QI(void *iface, const void *riid, void **ppv) {
    (void)riid; *ppv = iface; return S_OK;
}
static ULONG WINAPI H_AddRef(void *iface) { return (ULONG)InterlockedIncrement(&((Handler *)iface)->ref); }
static ULONG WINAPI H_Release(void *iface) { return (ULONG)InterlockedDecrement(&((Handler *)iface)->ref); }
static HRESULT WINAPI H_Invoke(void *iface, HRESULT error, void *obj) {
    Handler *h = (Handler *)iface;
    h->hr = error;
    h->obj = obj;
    h->thread = GetCurrentThreadId();
    InterlockedExchange(&h->called, 1);
    printf("%s_called=1 hr=0x%08lx objnull=%d tid=%lu\n", h->tag,
           (unsigned long)error, obj == NULL, (unsigned long)h->thread);
    if (h->is_env && error == S_OK && obj != NULL && g_hwnd != NULL && !g_ctl_started()) {
        void **vtbl = *(void ***)obj;
        CreateController_t create_ctl = (CreateController_t)vtbl[3];
        memset(&g_ctl_h, 0, sizeof(g_ctl_h));
        g_ctl_h.lpVtbl = h->lpVtbl; g_ctl_h.ref = 1; g_ctl_h.tag = "CTL";
        SetLastError(0);
        error = create_ctl(obj, g_hwnd, (void *)&g_ctl_h);
        printf("STEP_createctl_hr=0x%08lx err=%lu tid=%lu\n",
               (unsigned long)error, (unsigned long)GetLastError(),
               (unsigned long)GetCurrentThreadId());
    }
    return S_OK;
}
static void *handler_vtbl[] = { (void *)H_QI, (void *)H_AddRef,
                                (void *)H_Release, (void *)H_Invoke };

typedef HRESULT (STDAPICALLTYPE *CreateEnvInternal_t)(UINT, const WCHAR *, const WCHAR *,
                                                      void *, void *);

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return DefWindowProcW(h, m, w, l);
}

static void pump_until(volatile LONG *flag, int tenths) {
    int i;
    for (i = 0; i < tenths && !*flag; i++) {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(100);
    }
}

/* ICoreWebView2Controller slot 14: get_CoreWebView2. ICoreWebView2 slot 5: Navigate. */
typedef HRESULT (STDAPICALLTYPE *GetCore_t)(void *, void **);
typedef HRESULT (STDAPICALLTYPE *Navigate_t)(void *, const WCHAR *);

int main(void) {
    WCHAR key[512], pv[64], location[MAX_PATH], dll[MAX_PATH * 2];
    WCHAR userdata[MAX_PATH], rt[MAX_PATH * 2];
    HKEY hk;
    DWORD type = 0, size;
    HMODULE mod;
    CreateEnvInternal_t create_env;
    Handler env_h;
    WNDCLASSW wc;
    HRESULT hr;
    int i;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));
    printf("MAIN_tid=%lu\n", (unsigned long)GetCurrentThreadId());

    lstrcpyW(key, L"SOFTWARE\\Microsoft\\EdgeUpdate\\Clients\\" WV2_GUID);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hk) != ERROR_SUCCESS) {
        printf("PROBE_RESULT=NO_RUNTIME\n"); return 2;
    }
    size = sizeof(pv);
    if (RegQueryValueExW(hk, L"pv", NULL, &type, (BYTE *)pv, &size) != ERROR_SUCCESS) {
        RegCloseKey(hk); printf("PROBE_RESULT=NO_PV\n"); return 2;
    }
    size = sizeof(location);
    if (RegQueryValueExW(hk, L"location", NULL, &type, (BYTE *)location, &size) != ERROR_SUCCESS) {
        RegCloseKey(hk); printf("PROBE_RESULT=NO_LOCATION\n"); return 2;
    }
    RegCloseKey(hk);

    _snwprintf(rt, MAX_PATH * 2, L"%s\\%s", location, pv);
    SetDllDirectoryW(rt);
    _snwprintf(dll, MAX_PATH * 2, L"%s\\%s\\EBWebView\\x86\\EmbeddedBrowserWebView.dll",
               location, pv);
    mod = LoadLibraryW(dll);
    printf("STEP_host_load=%d\n", mod != NULL);
    if (!mod) { printf("PROBE_RESULT=NO_HOST_DLL\n"); return 3; }
    create_env = (CreateEnvInternal_t)(void *)
        GetProcAddress(mod, "CreateWebViewEnvironmentWithOptionsInternal");
    if (!create_env) { printf("PROBE_RESULT=NO_EXPORT\n"); return 3; }

    /* Parent window FIRST: it must exist on this UI thread before the
     * environment completes, so the chained controller call owns it. */
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"Wv2CtlProbe";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc)) { printf("PROBE_RESULT=NO_WINDOW_CLASS\n"); return 5; }
    g_hwnd = CreateWindowExW(0, L"Wv2CtlProbe", L"Wv2CtlProbe",
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);
    printf("STEP_hwnd=%p\n", g_hwnd);
    if (!g_hwnd) { printf("PROBE_RESULT=NO_WINDOW\n"); return 5; }
    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    GetTempPathW(MAX_PATH, userdata);
    lstrcatW(userdata, L"wv2ctlprobe");

    memset(&env_h, 0, sizeof(env_h));
    env_h.lpVtbl = handler_vtbl; env_h.ref = 1; env_h.tag = "ENV"; env_h.is_env = 1;
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    printf("STEP_coininit=0x%08lx\n", (unsigned long)hr);
    hr = create_env(0, NULL, userdata, NULL, (void *)&env_h);
    printf("STEP_createenv_hr=0x%08lx\n", (unsigned long)hr);

    pump_until(&g_ctl_h.called, 600);
    printf("STEP_env_called=%ld hr=0x%08lx objnull=%d tid=%lu\n",
           (long)env_h.called, (unsigned long)env_h.hr, env_h.obj == NULL,
           (unsigned long)env_h.thread);
    printf("STEP_ctl_called=%ld hr=0x%08lx objnull=%d tid=%lu\n",
           (long)g_ctl_h.called, (unsigned long)g_ctl_h.hr, g_ctl_h.obj == NULL,
           (unsigned long)g_ctl_h.thread);

    /* --- navigate: does a live page survive? -------------------------------- */
    if (g_ctl_h.called && g_ctl_h.obj) {
        void **cvtbl = *(void ***)g_ctl_h.obj;
        GetCore_t get_core = (GetCore_t)cvtbl[14];
        void *core = NULL;
        hr = get_core(g_ctl_h.obj, &core);
        printf("STEP_getcore_hr=0x%08lx corenull=%d\n",
               (unsigned long)hr, core == NULL);
        if (SUCCEEDED(hr) && core != NULL) {
            void **corevt = *(void ***)core;
            Navigate_t nav = (Navigate_t)corevt[5];
            hr = nav(core, L"https://example.com/");
            printf("STEP_navigate_hr=0x%08lx\n", (unsigned long)hr);
        }
    }

    /* Keep the message loop alive so a created controller can run. */
    for (i = 0; i < 200; i++) {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(100);
    }

    {
        RECT rc;
        GetClientRect(g_hwnd, &rc);
        printf("STEP_client=%ldx%ld\n", (long)(rc.right - rc.left),
               (long)(rc.bottom - rc.top));
        printf("STEP_visible=%d\n", IsWindowVisible(g_hwnd) ? 1 : 0);
    }

    DestroyWindow(g_hwnd);
    CoUninitialize();
    printf("PROBE_RESULT=%s\n",
           (g_ctl_h.called && g_ctl_h.obj) ? "CONTROLLER_CREATED" : "CONTROLLER_NOT_CREATED");
    return 0;
}
