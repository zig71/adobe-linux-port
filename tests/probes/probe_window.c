/* probe_window.c — top-level window geometry and state.
 *
 * Why: the Creative Cloud installer creates a window ("AdobeInstallerWindowClass",
 * title "Creative Cloud Installer") and then waits for its onWindowResize
 * callback. On Windows that fires; under Wine it never does, and the install
 * workflow never starts.
 *
 * This probe reports the geometry and state of its own top-level window, both
 * immediately after creation and after it has been shown and sized, so the
 * numbers Windows produces can be compared with Wine's directly. It also
 * counts the WM_SIZE / WM_WINDOWPOSCHANGED / WM_SHOWWINDOW messages it
 * receives, which is the quantity the installer is actually waiting on.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_window.exe probe_window.c -luser32 -lgdi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static int n_create, n_size, n_windowposchanged, n_showwindow, n_move, n_paint, n_nccalcsize;
static int n_getminmaxinfo, n_activate;
static RECT first_size_client;
static int have_first_size;

static void report_state(const char *tag, HWND hwnd) {
    RECT wr, cr;
    WINDOWPLACEMENT wp;
    int vis;
    char cls[128], title[128], text[64];

    GetClassNameA(hwnd, cls, sizeof(cls));
    GetWindowTextA(hwnd, title, sizeof(title));
    GetWindowRect(hwnd, &wr);
    GetClientRect(hwnd, &cr);
    vis = IsWindowVisible(hwnd) ? 1 : 0;
    memset(&wp, 0, sizeof(wp));
    wp.length = sizeof(wp);
    GetWindowPlacement(hwnd, &wp);
    sprintf(text, "%s", vis ? "visible" : "hidden");

    printf("WND_%s_class=%s\n", tag, cls);
    printf("WND_%s_title=%s\n", tag, title);
    printf("WND_%s_window_rect=%ld,%ld,%ld,%ld\n", tag,
           (long)wr.left, (long)wr.top, (long)wr.right, (long)wr.bottom);
    printf("WND_%s_window_size=%ldx%ld\n", tag,
           (long)(wr.right - wr.left), (long)(wr.bottom - wr.top));
    printf("WND_%s_client_size=%ldx%ld\n", tag,
           (long)(cr.right - cr.left), (long)(cr.bottom - cr.top));
    printf("WND_%s_visible=%s\n", tag, text);
    printf("WND_%s_showcmd=%u\n", tag, (unsigned)wp.showCmd);
    printf("WND_%s_style=0x%08lx\n", tag, (unsigned long)GetWindowLongA(hwnd, GWL_STYLE));
    printf("WND_%s_exstyle=0x%08lx\n", tag, (unsigned long)GetWindowLongA(hwnd, GWL_EXSTYLE));
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: n_create++; break;
    case WM_SIZE:
        n_size++;
        if (!have_first_size) {
            GetClientRect(hwnd, &first_size_client);
            have_first_size = 1;
        }
        break;
    case WM_WINDOWPOSCHANGED: n_windowposchanged++; break;
    case WM_SHOWWINDOW: n_showwindow++; break;
    case WM_MOVE: n_move++; break;
    case WM_PAINT: n_paint++; ValidateRect(hwnd, NULL); break;
    case WM_NCCALCSIZE: n_nccalcsize++; break;
    case WM_GETMINMAXINFO: n_getminmaxinfo++; break;
    case WM_ACTIVATE: n_activate++; break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProcA(hwnd, msg, wp, lp);
    }
    return 0;
}

/* --- metrics the installer is likely to size itself against ------------- */
static void report_metrics(void) {
    printf("METRIC_screen=%dx%d\n",
           GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    printf("METRIC_workarea=%dx%d\n",
           GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN));
    printf("METRIC_caption=%d\n", GetSystemMetrics(SM_CYCAPTION));
    printf("METRIC_border=%d\n", GetSystemMetrics(SM_CXSIZEFRAME));
    printf("METRIC_frame=%d\n", GetSystemMetrics(SM_CYFRAME));
    printf("METRIC_paddedborder=%d\n", GetSystemMetrics(SM_CXPADDEDBORDER));
    printf("METRIC_monitors=%d\n", GetSystemMetrics(SM_CMONITORS));
    printf("METRIC_minmaxinfo=%d\n", GetSystemMetrics(SM_CXMINTRACK));
    printf("METRIC_dpi=%u\n", (unsigned)GetDpiForSystem());
}

int main(void) {
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    RECT want;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));
    report_metrics();

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "AdobeWineLabWindowClass";
    if (!RegisterClassA(&wc)) {
        printf("RegisterClass_failed=%lu\n", (unsigned long)GetLastError());
        return 2;
    }

    /* Create with an explicit size, the way a normal application does. */
    want.left = 2; want.top = 24; want.right = 1022; want.bottom = 689;
    hwnd = CreateWindowExA(0x00000100 /* WS_EX_APPWINDOW */, "AdobeWineLabWindowClass",
                           "Adobe Wine Lab Window",
                           0x00cf0000 /* WS_OVERLAPPEDWINDOW */,
                           want.left, want.top,
                           want.right - want.left, want.bottom - want.top,
                           NULL, NULL, GetModuleHandleA(NULL), NULL);
    printf("CreateWindow_nonnull=%d\n", hwnd != NULL);
    if (!hwnd) { printf("CreateWindow_err=%lu\n", (unsigned long)GetLastError()); return 3; }

    printf("--- counts immediately after CreateWindowEx ---\n");
    printf("AFTER_CREATE_size=%d\n", n_size);
    printf("AFTER_CREATE_windowposchanged=%d\n", n_windowposchanged);
    printf("AFTER_CREATE_showwindow=%d\n", n_showwindow);
    printf("AFTER_CREATE_create=%d\n", n_create);
    printf("AFTER_CREATE_move=%d\n", n_move);
    printf("AFTER_CREATE_nccalcsize=%d\n", n_nccalcsize);
    printf("AFTER_CREATE_getminmaxinfo=%d\n", n_getminmaxinfo);
    report_state("after_create", hwnd);

    /* Show it, the way an application that wants a UI does. */
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    printf("--- counts after ShowWindow(SW_SHOWNORMAL) ---\n");
    printf("AFTER_SHOW_size=%d\n", n_size);
    printf("AFTER_SHOW_windowposchanged=%d\n", n_windowposchanged);
    printf("AFTER_SHOW_showwindow=%d\n", n_showwindow);
    printf("AFTER_SHOW_move=%d\n", n_move);
    printf("AFTER_SHOW_activate=%d\n", n_activate);
    if (have_first_size)
        printf("FIRST_SIZE_CLIENT=%ldx%ld\n",
               (long)(first_size_client.right - first_size_client.left),
               (long)(first_size_client.bottom - first_size_client.top));
    report_state("after_show", hwnd);

    /* Pump briefly so any posted work settles, then report again. */
    for (int i = 0; i < 200; i++) {
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        Sleep(5);
    }
    printf("--- counts after message pump ---\n");
    printf("AFTER_PUMP_size=%d\n", n_size);
    printf("AFTER_PUMP_windowposchanged=%d\n", n_windowposchanged);
    printf("AFTER_PUMP_showwindow=%d\n", n_showwindow);
    printf("AFTER_PUMP_paint=%d\n", n_paint);
    report_state("after_pump", hwnd);

    DestroyWindow(hwnd);
    printf("PROBE_RESULT=OK\n");
    return 0;
}
