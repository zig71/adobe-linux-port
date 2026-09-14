// probe_swapchain: DXGI flip-model swapchain on a hidden window.
// Same PE on Windows and Wine. Exit 0 if Present succeeds.
#include <stdio.h>
#include <dxgi1_2.h>
#include <d3d11.h>

int main(void) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "probeswap";
    RegisterClassA(&wc);
    // NOTE: never shown - like the app's hidden host window.
    HWND w = CreateWindowExA(0, "probeswap", "probe", 0,
                             0, 0, 800, 600, NULL, NULL, wc.hInstance, NULL);
    printf("window=%p\n", w);
    IDXGIFactory2 *fac = NULL;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory2), (void **)&fac);
    printf("Factory2 -> 0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 1;
    ID3D11Device *dev = NULL;
    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0,
                           D3D11_SDK_VERSION, &dev, NULL, NULL);
    printf("Device -> 0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 1;
    DXGI_SWAP_CHAIN_DESC1 sd = {0};
    sd.Width = 800; sd.Height = 600;
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.SampleDesc.Count = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    IDXGISwapChain1 *sc = NULL;
    hr = fac->CreateSwapChainForHwnd((IUnknown *)dev, w, &sd, NULL, NULL, &sc);
    printf("CreateSwapChainForHwnd(hidden) -> 0x%08lx\n", (unsigned long)hr);
    if (sc) {
        hr = sc->Present(0, 0);
        printf("Present -> 0x%08lx\n", (unsigned long)hr);
        ID3D11Texture2D *buf = NULL;
        hr = sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&buf);
        printf("GetBuffer -> 0x%08lx\n", (unsigned long)hr);
        if (SUCCEEDED(hr)) {
            IDXGIResource *res = NULL;
            hr = buf->QueryInterface(__uuidof(IDXGIResource), (void **)&res);
            printf("BufQI(IDXGIResource) -> 0x%08lx\n", (unsigned long)hr);
            if (SUCCEEDED(hr)) {
                HANDLE h = NULL;
                hr = res->GetSharedHandle(&h);
                printf("BufGetSharedHandle -> 0x%08lx handle=%p\n", (unsigned long)hr, h);
                res->Release();
            }
            buf->Release();
        }
    }
    if (sc) { sc->Release(); sc = NULL; }
    // Phase 2: shown window (both platforms should succeed here).
    ShowWindow(w, SW_SHOW);
    UpdateWindow(w);
    MSG m; int spins = 0;
    while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE) && spins++ < 20) { TranslateMessage(&m); DispatchMessageA(&m); }
    IDXGISwapChain1 *sc2 = NULL;
    hr = fac->CreateSwapChainForHwnd((IUnknown *)dev, w, &sd, NULL, NULL, &sc2);
    printf("CreateSwapChainForHwnd(shown) -> 0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr)) {
        ID3D11Texture2D *buf2 = NULL;
        hr = sc2->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&buf2);
        printf("ShownGetBuffer -> 0x%08lx\n", (unsigned long)hr);
        if (SUCCEEDED(hr)) {
            IDXGIResource1 *res2 = NULL;
            hr = buf2->QueryInterface(__uuidof(IDXGIResource1), (void **)&res2);
            printf("ShownBufQI1 -> 0x%08lx\n", (unsigned long)hr);
            if (SUCCEEDED(hr)) {
                HANDLE h2 = NULL;
                hr = res2->CreateSharedHandle(NULL, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, NULL, &h2);
                printf("ShownBufCreateSharedHandle -> 0x%08lx handle=%p\n", (unsigned long)hr, h2);
                if (SUCCEEDED(hr)) CloseHandle(h2);
                res2->Release();
            }
            buf2->Release();
        }
        sc2->Release();
    }
    dev->Release();
    fac->Release();
    return 0;
}
