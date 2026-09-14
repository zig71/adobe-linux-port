/* probe_webbrowser.c — host the WebBrowser control the way the installer does.
 *
 * Why: the Creative Cloud installer creates its main window, hides it
 * (SetWindowPos with SWP_HIDEWINDOW), creates an AdobeWebBrowserWindowClass
 * child, shows the browser children, and then waits. It never calls ShowWindow
 * on its main window -- established by API tracing, not inference -- so Wine is
 * not dropping a show request. The remaining question is whether the embedded
 * browser ever reports itself ready, which is the value the host branches on
 * before revealing its window and starting the install workflow.
 *
 * This probe embeds CLSID_WebBrowser (ieframe) as an installer would, drives the
 * activation sequence, loads a document, and reports the observable state:
 * which interfaces the control exposes and whether it reaches
 * READYSTATE_COMPLETE.
 *
 * Build 32-bit, matching the installer:
 *   i686-w64-mingw32-gcc -O2 -o probe_webbrowser.exe probe_webbrowser.c \
 *       -lole32 -loleaut32 -luuid -luser32 -lgdi32
 */
#include <windows.h>
#include <objbase.h>
#include <ocidl.h>
#include <docobj.h>
#include <exdisp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

static void ph(const char *k, HRESULT hr) { printf("%s=0x%08lx\n", k, (unsigned long)hr); }

/* --- a client site, an in-place site and an in-place frame ---------------- */
typedef struct Site {
    IOleClientSite site;
    IOleInPlaceSite inplace;
    IOleInPlaceFrame frame;
    LONG ref;
    HWND hwnd;
} Site;

#define SITE_FROM(field, ptr) ((Site *)((char *)(ptr) - offsetof(Site, field)))

static HRESULT WINAPI Site_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv) {
    Site *s = (Site *)iface;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleClientSite))
        *ppv = &s->site;
    else if (IsEqualIID(riid, &IID_IOleInPlaceSite))
        *ppv = &s->inplace;
    else if (IsEqualIID(riid, &IID_IOleInPlaceFrame))
        *ppv = &s->frame;
    if (*ppv) { InterlockedIncrement(&s->ref); return S_OK; }
    return E_NOINTERFACE;
}
static ULONG WINAPI Site_AddRef(IOleClientSite *iface) { return (ULONG)InterlockedIncrement(&((Site *)iface)->ref); }
static ULONG WINAPI Site_Release(IOleClientSite *iface) { return (ULONG)InterlockedDecrement(&((Site *)iface)->ref); }
static HRESULT WINAPI Site_SaveObject(IOleClientSite *i) { (void)i; return S_OK; }
static HRESULT WINAPI Site_GetMoniker(IOleClientSite *i, DWORD a, DWORD b, IMoniker **m) { (void)i;(void)a;(void)b; *m = NULL; return E_NOTIMPL; }
static HRESULT WINAPI Site_GetContainer(IOleClientSite *i, IOleContainer **c) { (void)i; *c = NULL; return E_NOINTERFACE; }
static HRESULT WINAPI Site_ShowObject(IOleClientSite *i) { (void)i; printf("CALLBACK_ShowObject=1\n"); return S_OK; }
static HRESULT WINAPI Site_OnShowWindow(IOleClientSite *i, BOOL b) { (void)i; printf("CALLBACK_OnShowWindow=1 value=%d\n", b); return S_OK; }
static HRESULT WINAPI Site_RequestNewObjectLayout(IOleClientSite *i) { (void)i; return E_NOTIMPL; }

static HRESULT WINAPI IPW_GetWindow(IOleInPlaceSite *i, HWND *h) {
    *h = SITE_FROM(inplace, i)->hwnd;
    printf("CALLBACK_InPlaceGetWindow=1\n");
    return S_OK;
}
static HRESULT WINAPI IPW_ContextSensitiveHelp(IOleInPlaceSite *i, BOOL b) { (void)i;(void)b; return E_NOTIMPL; }
static HRESULT WINAPI IPW_CanInPlaceActivate(IOleInPlaceSite *i) { (void)i; printf("CALLBACK_CanInPlaceActivate=1\n"); return S_OK; }
static HRESULT WINAPI IPW_OnInPlaceActivate(IOleInPlaceSite *i) { (void)i; printf("CALLBACK_OnInPlaceActivate=1\n"); return S_OK; }
static HRESULT WINAPI IPW_OnUIActivate(IOleInPlaceSite *i) { (void)i; printf("CALLBACK_OnUIActivate=1\n"); return S_OK; }
static HRESULT WINAPI IPW_GetWindowContext(IOleInPlaceSite *i, IOleInPlaceFrame **f,
                                           IOleInPlaceUIWindow **u, LPRECT r, LPRECT cr,
                                           LPOLEINPLACEFRAMEINFO fi) {
    Site *s = SITE_FROM(inplace, i);
    printf("CALLBACK_GetWindowContext=1\n");
    if (f) { *f = &s->frame; InterlockedIncrement(&s->ref); }
    if (u) *u = NULL;
    if (r) { r->left = 0; r->top = 0; r->right = 1024; r->bottom = 720; }
    if (cr) { cr->left = 0; cr->top = 0; cr->right = 1024; cr->bottom = 720; }
    if (fi) { fi->cb = sizeof(*fi); fi->fMDIApp = FALSE; fi->hwndFrame = s->hwnd;
              fi->haccel = NULL; fi->cAccelEntries = 0; }
    return S_OK;
}
static HRESULT WINAPI IPW_Scroll(IOleInPlaceSite *i, SIZE sz) { (void)i;(void)sz; return E_NOTIMPL; }
static HRESULT WINAPI IPW_OnUIDeactivate(IOleInPlaceSite *i, BOOL b) { (void)i;(void)b; printf("CALLBACK_OnUIDeactivate=1\n"); return S_OK; }
static HRESULT WINAPI IPW_OnInPlaceDeactivate(IOleInPlaceSite *i) { (void)i; printf("CALLBACK_OnInPlaceDeactivate=1\n"); return S_OK; }
static HRESULT WINAPI IPW_DiscardUndoState(IOleInPlaceSite *i) { (void)i; return E_NOTIMPL; }
static HRESULT WINAPI IPW_DeactivateAndUndo(IOleInPlaceSite *i) { (void)i; return E_NOTIMPL; }
static HRESULT WINAPI IPW_OnPosRectChange(IOleInPlaceSite *i, LPCRECT rc) { (void)i;(void)rc; return S_OK; }

static HRESULT WINAPI FRW_QueryInterface(IOleInPlaceFrame *i, REFIID riid, void **ppv) {
    Site *s = SITE_FROM(frame, i);
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IOleInPlaceFrame) ||
        IsEqualIID(riid, &IID_IOleInPlaceUIWindow) || IsEqualIID(riid, &IID_IOleWindow))
        *ppv = &s->frame;
    if (*ppv) { InterlockedIncrement(&s->ref); return S_OK; }
    return E_NOINTERFACE;
}
static ULONG WINAPI FRW_AddRef(IOleInPlaceFrame *i) {
    Site *s = SITE_FROM(frame, i);
    return (ULONG)InterlockedIncrement(&s->ref);
}
static ULONG WINAPI FRW_Release(IOleInPlaceFrame *i) {
    Site *s = SITE_FROM(frame, i);
    return (ULONG)InterlockedDecrement(&s->ref);
}
static HRESULT WINAPI FRW_GetWindow(IOleInPlaceFrame *i, HWND *h) { *h = SITE_FROM(frame, i)->hwnd; return S_OK; }
static HRESULT WINAPI FRW_ContextSensitiveHelp(IOleInPlaceFrame *i, BOOL b) { (void)i;(void)b; return E_NOTIMPL; }
static HRESULT WINAPI FRW_GetBorder(IOleInPlaceFrame *i, LPRECT r) { (void)i;(void)r; return E_NOTIMPL; }
static HRESULT WINAPI FRW_RequestBorderSpace(IOleInPlaceFrame *i, LPCBORDERWIDTHS b) { (void)i;(void)b; return E_NOTIMPL; }
static HRESULT WINAPI FRW_SetBorderSpace(IOleInPlaceFrame *i, LPCBORDERWIDTHS b) { (void)i;(void)b; return E_NOTIMPL; }
static HRESULT WINAPI FRW_SetActiveObject(IOleInPlaceFrame *i, IOleInPlaceActiveObject *o, LPCOLESTR n) {
    (void)i;(void)o;(void)n; printf("CALLBACK_FrameSetActiveObject=1\n"); return S_OK;
}
static HRESULT WINAPI FRW_InsertMenus(IOleInPlaceFrame *i, HMENU m, LPOLEMENUGROUPWIDTHS w) { (void)i;(void)m;(void)w; return E_NOTIMPL; }
static HRESULT WINAPI FRW_SetMenu(IOleInPlaceFrame *i, HMENU m, HOLEMENU h, HWND w) { (void)i;(void)m;(void)h;(void)w; return S_OK; }
static HRESULT WINAPI FRW_RemoveMenus(IOleInPlaceFrame *i, HMENU m) { (void)i;(void)m; return E_NOTIMPL; }
static HRESULT WINAPI FRW_SetStatusText(IOleInPlaceFrame *i, LPCOLESTR t) { (void)i;(void)t; printf("CALLBACK_SetStatusText=1\n"); return S_OK; }
static HRESULT WINAPI FRW_EnableModeless(IOleInPlaceFrame *i, BOOL b) { (void)i;(void)b; return S_OK; }
static HRESULT WINAPI FRW_TranslateAccelerator(IOleInPlaceFrame *i, LPMSG m, WORD w) { (void)i;(void)m;(void)w; return S_FALSE; }

/* Vendor-neutral vtbl layout comes straight from the interface declarations,
 * so use the compiler's own structs and assign field by field. */
static IOleClientSiteVtbl site_vtbl = {
    Site_QueryInterface, Site_AddRef, Site_Release,
    Site_SaveObject, Site_GetMoniker, Site_GetContainer,
    Site_ShowObject, Site_OnShowWindow, Site_RequestNewObjectLayout
};
static IOleInPlaceSiteVtbl inplace_vtbl = {
    (HRESULT (WINAPI *)(IOleInPlaceSite *, REFIID, void **))Site_QueryInterface,
    (ULONG (WINAPI *)(IOleInPlaceSite *))Site_AddRef,
    (ULONG (WINAPI *)(IOleInPlaceSite *))Site_Release,
    IPW_GetWindow, IPW_ContextSensitiveHelp,
    IPW_CanInPlaceActivate, IPW_OnInPlaceActivate, IPW_OnUIActivate,
    IPW_GetWindowContext, IPW_Scroll, IPW_OnUIDeactivate,
    IPW_OnInPlaceDeactivate, IPW_DiscardUndoState,
    IPW_DeactivateAndUndo, IPW_OnPosRectChange
};
static IOleInPlaceFrameVtbl frame_vtbl = {
    FRW_QueryInterface, FRW_AddRef, FRW_Release,
    FRW_GetWindow, FRW_ContextSensitiveHelp,
    FRW_GetBorder, FRW_RequestBorderSpace, FRW_SetBorderSpace,
    FRW_SetActiveObject, FRW_InsertMenus, FRW_SetMenu, FRW_RemoveMenus,
    FRW_SetStatusText, FRW_EnableModeless, FRW_TranslateAccelerator
};

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE) return 0;
    return DefWindowProcA(h, m, w, l);
}

int main(void) {
    static const char *kPage =
        "<html><head><title>lab</title></head><body><h1>Adobe Wine Lab</h1></body></html>";
    char html_path[MAX_PATH], url[MAX_PATH + 16];
    WCHAR wurl[MAX_PATH + 16];
    WNDCLASSA wc;
    HWND hwnd;
    Site site;
    IOleObject *ole = NULL;
    IWebBrowser2 *wb = NULL;
    IPersistStreamInit *psi = NULL;
    IViewObject *vo = NULL;
    IViewObjectEx *vox = NULL;
    IOleInPlaceObject *ipo = NULL;
    IOleCommandTarget *oct = NULL;
    HGLOBAL mem = NULL;
    IStream *stream = NULL;
    HRESULT hr;
    int i;
    READYSTATE rs = (READYSTATE)-1;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ph("CoInitializeEx", hr);

    memset(&site, 0, sizeof(site));
    site.site.lpVtbl = &site_vtbl;
    site.inplace.lpVtbl = &inplace_vtbl;
    site.frame.lpVtbl = &frame_vtbl;
    site.ref = 1;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "LabHostWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);
    hwnd = CreateWindowExA(0, "LabHostWindow", "Lab Host", WS_OVERLAPPEDWINDOW,
                           0, 0, 1024, 720, NULL, NULL, GetModuleHandleA(NULL), NULL);
    site.hwnd = hwnd;
    printf("host_window_nonnull=%d\n", hwnd != NULL);

    hr = CoCreateInstance(&CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IOleObject, (void **)&ole);
    ph("CoCreateInstance_WebBrowser", hr);
    if (!ole) { printf("PROBE_RESULT=NO_CONTROL\n"); return 2; }

    {
        DWORD misc = 0;
        hr = ole->lpVtbl->GetMiscStatus(ole, DVASPECT_CONTENT, &misc);
        printf("OleObject_GetMiscStatus=0x%08lx value=0x%08lx\n",
               (unsigned long)hr, (unsigned long)misc);
    }

    hr = ole->lpVtbl->SetClientSite(ole, &site.site);
    ph("OleObject_SetClientSite", hr);

    hr = ole->lpVtbl->QueryInterface(ole, &IID_IWebBrowser2, (void **)&wb);
    ph("QI_IWebBrowser2", hr);
    hr = ole->lpVtbl->QueryInterface(ole, &IID_IPersistStreamInit, (void **)&psi);
    ph("QI_IPersistStreamInit", hr);
    hr = ole->lpVtbl->QueryInterface(ole, &IID_IViewObject, (void **)&vo);
    ph("QI_IViewObject", hr);
    hr = ole->lpVtbl->QueryInterface(ole, &IID_IViewObjectEx, (void **)&vox);
    ph("QI_IViewObjectEx", hr);
    hr = ole->lpVtbl->QueryInterface(ole, &IID_IOleInPlaceObject, (void **)&ipo);
    ph("QI_IOleInPlaceObject", hr);
    hr = ole->lpVtbl->QueryInterface(ole, &IID_IOleCommandTarget, (void **)&oct);
    ph("QI_IOleCommandTarget", hr);

    hr = ole->lpVtbl->DoVerb(ole, OLEIVERB_SHOW, NULL, &site.site, -1, hwnd, NULL);
    ph("OleObject_DoVerb_OLEIVERB_SHOW", hr);
    hr = ole->lpVtbl->SetHostNames(ole, L"LabHost", L"LabBrowser");
    ph("OleObject_SetHostNames", hr);
    if (ipo) {
        RECT r = {0, 0, 1024, 720};
        hr = ipo->lpVtbl->SetObjectRects(ipo, &r, &r);
        ph("OleInPlaceObject_SetObjectRects", hr);
    }

    if (psi) {
        mem = GlobalAlloc(GMEM_MOVEABLE, strlen(kPage) + 1);
        if (mem) {
            void *p = GlobalLock(mem);
            memcpy(p, kPage, strlen(kPage) + 1);
            GlobalUnlock(mem);
            if (CreateStreamOnHGlobal(mem, TRUE, &stream) == S_OK) {
                hr = psi->lpVtbl->Load(psi, stream);
                ph("IPersistStreamInit_Load", hr);
            }
        }
        hr = psi->lpVtbl->InitNew(psi);
        ph("IPersistStreamInit_InitNew", hr);
    }

    GetTempPathA(MAX_PATH, html_path);
    strcat(html_path, "labprobe.html");
    {
        FILE *f = fopen(html_path, "w");
        if (f) { fputs(kPage, f); fclose(f); }
    }
    sprintf(url, "file:///%s", html_path);
    for (i = 0; url[i]; i++) if (url[i] == '\\') url[i] = '/';
    MultiByteToWideChar(CP_ACP, 0, url, -1, wurl, MAX_PATH + 16);

    if (wb) {
        hr = wb->lpVtbl->Navigate(wb, wurl, NULL, NULL, NULL, NULL);
        ph("IWebBrowser2_Navigate", hr);
    }

    printf("--- readiness sampling ---\n");
    for (i = 0; i < 60; i++) {
        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (wb) {
            READYSTATE cur = (READYSTATE)-1;
            if (SUCCEEDED(wb->lpVtbl->get_ReadyState(wb, &cur)) && cur != rs) {
                rs = cur;
                printf("readystate_t%02d=%ld\n", i * 100, (long)rs);
            }
        }
        Sleep(100);
    }
    printf("READYSTATE_final=%ld\n", (long)rs);
    printf("READYSTATE_COMPLETE_is_4=%d\n", rs == 4 ? 1 : 0);

    if (wb) {
        BSTR loc = NULL;
        hr = wb->lpVtbl->get_LocationURL(wb, &loc);
        printf("get_LocationURL_hr=0x%08lx\n", (unsigned long)hr);
        printf("LocationURL_nonempty=%d\n", (loc && SysStringLen(loc) > 0) ? 1 : 0);
        if (loc) SysFreeString(loc);
    }
    {
        RECT r;
        if (hwnd && GetClientRect(hwnd, &r))
            printf("host_client=%ldx%ld\n", (long)(r.right - r.left), (long)(r.bottom - r.top));
    }

    if (oct) oct->lpVtbl->Release(oct);
    if (ipo) ipo->lpVtbl->Release(ipo);
    if (vox) vox->lpVtbl->Release(vox);
    if (vo) vo->lpVtbl->Release(vo);
    if (psi) psi->lpVtbl->Release(psi);
    if (wb) wb->lpVtbl->Release(wb);
    ole->lpVtbl->Close(ole, OLECLOSE_NOSAVE);
    ole->lpVtbl->Release(ole);
    if (stream) stream->lpVtbl->Release(stream);
    if (hwnd) DestroyWindow(hwnd);
    CoUninitialize();
    printf("PROBE_RESULT=OK\n");
    return 0;
}
