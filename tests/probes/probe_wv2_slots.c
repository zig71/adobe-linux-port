/* probe_wv2_slots.c — find get_CoreWebView2 by SEH-guarded vtbl scan.
 *
 * The controller object may not lay out like the public
 * ICoreWebView2Controller (see probe_wv2_public fault). Scan slots 3..24,
 * calling each as GetCore(obj, &out) under __try/__except, and report which
 * return S_OK with a non-null object. Same binary, Windows and Wine.
 *
 * Build 32-bit:
 *   i686-w64-mingw32-gcc -O2 -o probe_wv2_slots.exe probe_wv2_slots.c \
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

static HWND g_hwnd = NULL;
static Handler g_ctl_h;
static LONG g_ctl_started = 0;

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
typedef HRESULT (STDAPICALLTYPE *CreateCtl_t)(void *, HWND, void *);
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
static void *handler_vtbl[] = { (void *)H_QI, (void *)H_AddRef,
                                (void *)H_Release, (void *)H_Invoke };

typedef HRESULT (STDAPICALLTYPE *CreateEnv_t)(const WCHAR *, const WCHAR *, void *, void *);
typedef HRESULT (STDAPICALLTYPE *SlotFn_t)(void *, void **);

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

static void *resume_addr = NULL;

static LONG CALLBACK VehHandler(EXCEPTION_POINTERS *ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code == EXCEPTION_ACCESS_VIOLATION ||
        code == EXCEPTION_ILLEGAL_INSTRUCTION ||
        code == EXCEPTION_PRIV_INSTRUCTION ||
        code == EXCEPTION_INT_DIVIDE_BY_ZERO ||
        code == EXCEPTION_STACK_OVERFLOW) {
        ep->ContextRecord->Eip = (DWORD)resume_addr;
        ep->ContextRecord->Eax = (DWORD)E_FAIL;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main(void) {
    WCHAR loader[MAX_PATH], userdata[MAX_PATH];
    HMODULE mod;
    CreateEnv_t create_env;
    Handler env_h;
    HWND hwnd;
    WNDCLASSW wc;
    HRESULT hr;
    int i;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));

    GetModuleFileNameW(NULL, loader, MAX_PATH);
    {
        WCHAR *s = loader + lstrlenW(loader);
        while (s > loader && *(s - 1) != L'\\') s--;
        lstrcpyW(s, L"WebView2Loader_x86.dll");
    }
    mod = LoadLibraryW(loader);
    if (!mod) { printf("PROBE_RESULT=NO_LOADER\n"); return 2; }
    create_env = (CreateEnv_t)(void *)GetProcAddress(mod, "CreateCoreWebView2EnvironmentWithOptions");
    if (!create_env) { printf("PROBE_RESULT=NO_EXPORT\n"); return 2; }

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"Wv2SlotProbe";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    if (!RegisterClassW(&wc)) { printf("PROBE_RESULT=NO_WINDOW_CLASS\n"); return 5; }
    hwnd = CreateWindowExW(0, L"Wv2SlotProbe", L"Wv2SlotProbe",
                           WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);
    if (!hwnd) { printf("PROBE_RESULT=NO_WINDOW\n"); return 5; }
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    g_hwnd = hwnd;

    GetTempPathW(MAX_PATH, userdata);
    lstrcatW(userdata, L"wv2slotprobe");

    memset(&env_h, 0, sizeof(env_h));
    env_h.lpVtbl = handler_vtbl; env_h.ref = 1; env_h.tag = "ENV";
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = create_env(NULL, userdata, NULL, (void *)&env_h);
    printf("STEP_createenv_hr=0x%08lx\n", (unsigned long)hr);
    pump_until(&g_ctl_h.called, 300);
    if (!g_ctl_h.called || !g_ctl_h.obj) {
        printf("PROBE_RESULT=CONTROLLER_NOT_CREATED\n");
        return 6;
    }
    printf("STEP_controller_ok=1\n");
    AddVectoredExceptionHandler(1, VehHandler);

    /* VEH-guarded scan for the slot that yields a Core object. A wrong slot
     * faults; the handler resumes past the call with E_FAIL. */
    {
        void **vtbl = *(void ***)g_ctl_h.obj;
        int s;
        for (s = 3; s <= 24; s++) {
            void *out = (void *)0xDEADBEEF;
            resume_addr = &&after_slot;
            {
                SlotFn_t fn = (SlotFn_t)vtbl[s];
                hr = fn(g_ctl_h.obj, &out);
            }
after_slot:
            printf("SLOT_%d hr=0x%08lx outnull=%d\n", s,
                   (unsigned long)hr, out == NULL);
        }
    }
    printf("PROBE_RESULT=SLOTS_SCANNED\n");
    return 0;
}
