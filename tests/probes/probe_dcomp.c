/* probe_dcomp.c — DirectComposition availability.
 *
 * Why: Wine bug 58921 attributes WebView2's failure to DirectComposition being
 * unimplemented -- "DCompositionCreateDevice failed: Not implemented
 * (0x80004001)" -- and bug 59370 (blank WebView2 windows, Chromium's D3D11
 * shared-texture compositor path) was closed as a duplicate of it.
 *
 * This reports whether the DirectComposition entry points can actually be
 * called in the current Wine, which decides whether that is still the blocker.
 *
 * dcomp.h is deliberately NOT included: mingw-w64's copy declares the Vtbl
 * structs but not the matching interface typedefs, so it does not compile. The
 * entry points are declared locally and resolved dynamically instead.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_dcomp.exe probe_dcomp.c -lole32 -luuid
 */
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

static void ph(const char *k, HRESULT hr) { printf("%s=0x%08lx\n", k, (unsigned long)hr); }

typedef HRESULT (WINAPI *create1_t)(IUnknown *, REFIID, void **);
typedef HRESULT (WINAPI *createN_t)(IUnknown *, REFIID, void **);
typedef HRESULT (WINAPI *surface_t)(DWORD, SECURITY_ATTRIBUTES *, HANDLE *);

int main(void) {
    HMODULE mod;
    create1_t create1;
    createN_t create2, create3;
    surface_t surface;
    IUnknown *unk = NULL;
    HRESULT hr;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ph("CoInitializeEx", hr);

    mod = LoadLibraryW(L"dcomp.dll");
    printf("dcomp_dll_load=%d\n", mod != NULL);
    if (!mod) { printf("PROBE_RESULT=NO_DCOMP\n"); return 2; }

    create1 = (create1_t)(void *)GetProcAddress(mod, "DCompositionCreateDevice");
    create2 = (createN_t)(void *)GetProcAddress(mod, "DCompositionCreateDevice2");
    create3 = (createN_t)(void *)GetProcAddress(mod, "DCompositionCreateDevice3");
    surface = (surface_t)(void *)GetProcAddress(mod, "DCompositionCreateSurfaceHandle");

    printf("has_DCompositionCreateDevice=%d\n", create1 != NULL);
    printf("has_DCompositionCreateDevice2=%d\n", create2 != NULL);
    printf("has_DCompositionCreateDevice3=%d\n", create3 != NULL);
    printf("has_DCompositionCreateSurfaceHandle=%d\n", surface != NULL);

    /* The call bug 58921 blames. E_NOTIMPL (0x80004001) is the recorded result. */
    if (create1) {
        hr = create1(NULL, &IID_IUnknown, (void **)&unk);
        ph("DCompositionCreateDevice", hr);
        printf("DCompositionCreateDevice_nonnull=%d\n", unk != NULL);
        if (unk) { unk->lpVtbl->Release(unk); unk = NULL; }
    }
    if (create2) {
        hr = create2(NULL, &IID_IUnknown, (void **)&unk);
        ph("DCompositionCreateDevice2", hr);
        printf("DCompositionCreateDevice2_nonnull=%d\n", unk != NULL);
        if (unk) { unk->lpVtbl->Release(unk); unk = NULL; }
    }
    if (create3) {
        hr = create3(NULL, &IID_IUnknown, (void **)&unk);
        ph("DCompositionCreateDevice3", hr);
        printf("DCompositionCreateDevice3_nonnull=%d\n", unk != NULL);
        if (unk) { unk->lpVtbl->Release(unk); unk = NULL; }
    }
    if (surface) {
        HANDLE h = NULL;
        hr = surface(0, NULL, &h);
        ph("DCompositionCreateSurfaceHandle", hr);
        printf("surface_handle_nonnull=%d\n", h != NULL);
        if (h) CloseHandle(h);
    }

    /* Same question through the high-level API Chromium is likely to use:
     * creating a device for a DXGI swapchain. */
    {
        HRESULT (WINAPI *cds)(IUnknown *, REFIID, void **) = create1;
        if (cds) {
            HMODULE dxgi = LoadLibraryW(L"dxgi.dll");
            printf("dxgi_load=%d\n", dxgi != NULL);
        }
    }

    CoUninitialize();
    printf("PROBE_RESULT=OK\n");
    return 0;
}
