/* probe_wv2_public.c — host WebView2 through the PUBLIC loader API.
 *
 * Unlike probe_wv2_controller (which drives the runtime's internal export and
 * must guess at internal vtable layouts), this uses WebView2Loader.dll and the
 * documented ICoreWebView2* interfaces only:
 *   env[3]  = CreateCoreWebView2Controller
 *   ctl[14] = get_CoreWebView2
 *   core[5] = Navigate
 * then pumps while a real page loads. Same binary, Windows and Wine.
 *
 * Build 32-bit:
 *   i686-w64-mingw32-gcc -O2 -o probe_wv2_public.exe probe_wv2_public.c \
 *       -lole32 -ladvapi32 -luuid -lgdi32 -luser32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef struct Handler {
    void **lpVtbl;
    LONG ref;
    volatile LONG called;
    volatile HRESULT hr;
    volatile void *obj;
    volatile DWORD thread;
    const char *tag;
} Handler;

static HRESULT WINAPI H_QI(void *iface, const void *riid, void **ppv) {
    (void)riid; *ppv = iface; return S_OK;
}
static ULONG WINAPI H_AddRef(void *iface) {
    Handler *h = (Handler *)iface;
    return (ULONG)InterlockedIncrement(&h->ref);
}
static ULONG WINAPI H_Release(void *iface) {
    Handler *h = (Handler *)iface;
    return (ULONG)InterlockedDecrement(&h->ref);
}
static HWND g_hwnd = NULL;
static Handler g_ctl_h;
static LONG g_ctl_started = 0;

static HRESULT WINAPI H_Invoke(void *iface, HRESULT error, void *obj);
static void *handler_vtbl[] = { (void *)H_QI, (void *)H_AddRef,
                                (void *)H_Release, (void *)H_Invoke };

typedef HRESULT (STDAPICALLTYPE *CreateCtl_t)(void *, HWND, void *);
typedef HRESULT (STDAPICALLTYPE *GetCore_t)(void *, void **);
typedef HRESULT (STDAPICALLTYPE *Navigate_t)(void *, const WCHAR *);

static HRESULT WINAPI H_Invoke(void *iface, HRESULT error, void *obj) {
    Handler *h = (Handler *)iface;
    h->hr = error;
    h->obj = obj;
    h->thread = GetCurrentThreadId();
    InterlockedExchange(&h->called, 1);
    printf("%s_called=1 hr=0x%08lx objnull=%d tid=%lu\n", h->tag,
           (unsigned long)error, obj == NULL, (unsigned long)h->thread);
    if (error == S_OK && obj != NULL && g_hwnd != NULL &&
        InterlockedExchange(&g_ctl_started, 1) == 0) {
        CreateCtl_t create_ctl = (CreateCtl_t)(*(void ***)obj)[3];
        memset(&g_ctl_h, 0, sizeof(g_ctl_h));
        g_ctl_h.lpVtbl = h->lpVtbl; g_ctl_h.ref = 1; g_ctl_h.tag = "CTL";
        error = create_ctl(obj, g_hwnd, (void *)&g_ctl_h);
        printf("STEP_createctl_hr=0x%08lx tid=%lu\n",
               (unsigned long)error, (unsigned long)GetCurrentThreadId());
    }
    return S_OK;
}

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

typedef HRESULT (STDAPICALLTYPE *CreateEnv_t)(const WCHAR *, const WCHAR *, void *, void *);

int main(void) {
    WCHAR loader[MAX_PATH], userdata[MAX_PATH];
    HMODULE mod;
    CreateEnv_t create_env;
    Handler env_h, ctl_h;
    void *env = NULL;
    HWND hwnd;
    WNDCLASSW wc;
    HRESULT hr;
    int i;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));
    printf("MAIN_tid=%lu\n", (unsigned long)GetCurrentThreadId());

    GetModuleFileNameW(NULL, loader, MAX_PATH);
    {
        WCHAR *s = loader + lstrlenW(loader);
        while (s > loader && *(s - 1) != L'\\') s--;
        lstrcpyW(s, L"WebView2Loader_x86.dll");
    }
    mod = LoadLibraryW(loader);
    printf("STEP_loader=%d\n", mod != NULL);
    if (!mod) { printf("PROBE_RESULT=NO_LOADER\n"); return 2; }
    create_env = (CreateEnv_t)(void *)GetProcAddress(mod, "CreateCoreWebView2EnvironmentWithOptions");
    printf("STEP_export=%d\n", create_env != NULL);
    if (!create_env) { printf("PROBE_RESULT=NO_EXPORT\n"); return 2; }

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"Wv2PubProbe";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc)) { printf("PROBE_RESULT=NO_WINDOW_CLASS\n"); return 5; }
    hwnd = CreateWindowExW(0, L"Wv2PubProbe", L"Wv2PubProbe",
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);
    printf("STEP_hwnd=%p visible=%d\n", hwnd, hwnd && IsWindowVisible(hwnd));
    if (!hwnd) { printf("PROBE_RESULT=NO_WINDOW\n"); return 5; }
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    g_hwnd = hwnd;

    GetTempPathW(MAX_PATH, userdata);
    lstrcatW(userdata, L"wv2pubprobe");

    memset(&env_h, 0, sizeof(env_h));
    env_h.lpVtbl = handler_vtbl; env_h.ref = 1; env_h.tag = "ENV";
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    printf("STEP_coininit=0x%08lx\n", (unsigned long)hr);
    hr = create_env(NULL, userdata, NULL, (void *)&env_h);
    printf("STEP_createenv_hr=0x%08lx\n", (unsigned long)hr);
    pump_until(&env_h.called, 300);
    printf("STEP_env_called=%ld hr=0x%08lx objnull=%d\n",
           (long)env_h.called, (unsigned long)env_h.hr, env_h.obj == NULL);
    if (!env_h.called || !env_h.obj) { printf("PROBE_RESULT=ENV_NOT_CREATED\n"); return 4; }
    env = env_h.obj;

    pump_until(&g_ctl_h.called, 300);
    printf("STEP_ctl_called=%ld hr=0x%08lx objnull=%d\n",
           (long)g_ctl_h.called, (unsigned long)g_ctl_h.hr, g_ctl_h.obj == NULL);
    if (!g_ctl_h.called || !g_ctl_h.obj) { printf("PROBE_RESULT=CONTROLLER_NOT_CREATED\n"); return 6; }

    {
        GetCore_t get_core = (GetCore_t)(*(void ***)g_ctl_h.obj)[14];
        void *core = NULL;
        hr = get_core(g_ctl_h.obj, &core);
        printf("STEP_getcore_hr=0x%08lx corenull=%d\n",
               (unsigned long)hr, core == NULL);
        if (SUCCEEDED(hr) && core != NULL) {
            Navigate_t nav = (Navigate_t)(*(void ***)core)[5];
            hr = nav(core, L"https://example.com/");
            printf("STEP_navigate_hr=0x%08lx\n", (unsigned long)hr);
        }
    }

    for (i = 0; i < 200; i++) {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        Sleep(100);
    }
    printf("STEP_survived=1\n");
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        printf("STEP_client=%ldx%ld\n", (long)(rc.right - rc.left),
               (long)(rc.bottom - rc.top));
    }
    DestroyWindow(hwnd);
    CoUninitialize();
    printf("PROBE_RESULT=PAGE_LOADED\n");
    return 0;
}
