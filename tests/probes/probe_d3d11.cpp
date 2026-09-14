// probe_d3d11: minimal D3D11 device creation. Same PE on Windows and Wine.
// Prints HRESULT + adapter description. Exit 0 on device, 1 otherwise.
#include <stdio.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <d3d11_4.h>

int main(void) {
    IDXGIFactory *factory = NULL;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&factory);
    printf("CreateDXGIFactory -> 0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 1;
    IDXGIAdapter *adapter = NULL;
    hr = factory->EnumAdapters(0, &adapter);
    printf("EnumAdapters(0) -> 0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr)) {
        DXGI_ADAPTER_DESC desc;
        if (SUCCEEDED(adapter->GetDesc(&desc)))
            printf("adapter=%ls vendor=%04x device=%04x\n", desc.Description,
                   (unsigned)desc.VendorId, (unsigned)desc.DeviceId);
        adapter->Release();
    }
    ID3D11Device *dev = NULL;
    D3D_FEATURE_LEVEL got = (D3D_FEATURE_LEVEL)0;
    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0,
                           D3D11_SDK_VERSION, &dev, &got, NULL);
    printf("D3D11CreateDevice(HW,BGRA) -> 0x%08lx level=0x%x\n",
           (unsigned long)hr, (unsigned)got);
    // Shared-texture round-trip (what Chromium compositing needs).
    ID3D11Texture2D *tex = NULL;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 64; td.Height = 64; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM; td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    hr = dev->CreateTexture2D(&td, NULL, &tex);
    printf("CreateTexture2D(SHARED) -> 0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr)) {
        IDXGIResource *res = NULL;
        hr = tex->QueryInterface(__uuidof(IDXGIResource), (void **)&res);
        printf("QI(IDXGIResource) -> 0x%08lx\n", (unsigned long)hr);
        if (SUCCEEDED(hr)) {
            HANDLE h = NULL;
            hr = res->GetSharedHandle(&h);
            printf("GetSharedHandle -> 0x%08lx handle=%p\n", (unsigned long)hr, h);
            HANDLE dup = NULL;
            BOOL dok = DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(), &dup, 0, FALSE, DUPLICATE_SAME_ACCESS);
            printf("DuplicateHandle(same-proc) -> %d gle=%lu dup=%p\n", dok, GetLastError(), dup);
            res->Release();
        }
    // Format matrix (which pixel formats admit shared textures here?).
    const DXGI_FORMAT fmts[] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_NV12 };
    for (int fi = 0; fi < 4; fi++) {
        D3D11_TEXTURE2D_DESC fd = {};
        fd.Format = fmts[fi]; fd.SampleDesc.Count = 1;
        fd.Usage = D3D11_USAGE_DEFAULT;
        fd.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        fd.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        ID3D11Texture2D *ft = NULL;
        HRESULT fhr = dev->CreateTexture2D(&fd, NULL, &ft);
        printf("SharedFmt[%d] -> 0x%08lx\n", fi, (unsigned long)fhr);
        if (SUCCEEDED(fhr)) ft->Release();
    }
    // Runtime use of RGBA8-shared (does creation-success hold up?).
    {
        D3D11_TEXTURE2D_DESC rd = {};
        rd.Width = 64; rd.Height = 64; rd.MipLevels = 1; rd.ArraySize = 1;
        rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM; rd.SampleDesc.Count = 1;
        rd.Usage = D3D11_USAGE_DEFAULT;
        rd.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        rd.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        ID3D11Texture2D *rt = NULL;
        HRESULT rhr = dev->CreateTexture2D(&rd, NULL, &rt);
        printf("RGBASharedCreate -> 0x%08lx\n", (unsigned long)rhr);
        if (SUCCEEDED(rhr)) {
            ID3D11RenderTargetView *rrtv = NULL;
            rhr = dev->CreateRenderTargetView(rt, NULL, &rrtv);
            printf("RGBASharedRTV -> 0x%08lx\n", (unsigned long)rhr);
            if (SUCCEEDED(rhr)) {
                ID3D11DeviceContext *ctx = NULL;
                dev->GetImmediateContext(&ctx);
                FLOAT c[4] = {1, 0, 0, 1};
                ctx->ClearRenderTargetView(rrtv, c);
                ctx->Flush();
                printf("RGBAClearFlush done\n");
                ctx->Release();
                rrtv->Release();
            }
            rt->Release();
        }
    }
    // Views on a shared texture (ANGLE wraps backing in RTV+SRV).
    ID3D11RenderTargetView *rtv = NULL;
    HRESULT vhr = dev->CreateRenderTargetView(tex, NULL, &rtv);
    printf("SharedRTV -> 0x%08lx\n", (unsigned long)vhr);
    if (SUCCEEDED(vhr)) rtv->Release();
    ID3D11ShaderResourceView *srv = NULL;
    vhr = dev->CreateShaderResourceView(tex, NULL, &srv);
    printf("SharedSRV -> 0x%08lx\n", (unsigned long)vhr);
    if (SUCCEEDED(vhr)) srv->Release();
        tex->Release();
    }
    // Keyed-mutex + NT-handle variant (ANGLE/Chromium zero-copy path).
    ID3D11Texture2D *tex2 = NULL;
    D3D11_TEXTURE2D_DESC td2 = {};
    td2.Width = 64; td2.Height = 64; td2.MipLevels = 1; td2.ArraySize = 1;
    td2.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
    td2.Usage = D3D11_USAGE_DEFAULT; td2.BindFlags = D3D11_BIND_RENDER_TARGET;
    hr = dev->CreateTexture2D(&td2, NULL, &tex2);
    printf("CreateTexture2D(SHARED_KEYEDMUTEX) -> 0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr)) {
        IDXGIKeyedMutex *km = NULL;
        hr = tex2->QueryInterface(__uuidof(IDXGIKeyedMutex), (void **)&km);
        printf("QI(IDXGIKeyedMutex) -> 0x%08lx\n", (unsigned long)hr);
        if (SUCCEEDED(hr)) {
            hr = km->AcquireSync(0, 1000);
            printf("AcquireSync -> 0x%08lx\n", (unsigned long)hr);
            if (SUCCEEDED(hr)) km->ReleaseSync(1);
            km->Release();
        }
        IDXGIResource1 *res1 = NULL;
        hr = tex2->QueryInterface(__uuidof(IDXGIResource1), (void **)&res1);
        printf("QI(IDXGIResource1) -> 0x%08lx\n", (unsigned long)hr);
        if (SUCCEEDED(hr)) {
            HANDLE h = NULL;
            hr = res1->CreateSharedHandle(NULL, DXGI_SHARED_RESOURCE_READ, NULL, &h);
            printf("CreateSharedHandle(NT) -> 0x%08lx handle=%p\n", (unsigned long)hr, h);
            if (SUCCEEDED(hr)) CloseHandle(h);
            res1->Release();
        }
        tex2->Release();
    }
    // D3D11 fences (Chromium syncs shared images through them).
    ID3D11Device5 *dev5 = NULL;
    hr = dev->QueryInterface(__uuidof(ID3D11Device5), (void **)&dev5);
    printf("QI(ID3D11Device5) -> 0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr)) {
        ID3D11Fence *fence = NULL;
        hr = dev5->CreateFence(0, D3D11_FENCE_FLAG_SHARED, __uuidof(ID3D11Fence), (void **)&fence);
        printf("CreateFence(SHARED) -> 0x%08lx\n", (unsigned long)hr);
        if (SUCCEEDED(hr)) {
            IDXGIResource1 *fres = NULL;
            hr = fence->QueryInterface(__uuidof(IDXGIResource1), (void **)&fres);
            printf("FenceQI1 -> 0x%08lx\n", (unsigned long)hr);
            if (SUCCEEDED(hr)) {
                HANDLE fh = NULL;
                hr = fres->CreateSharedHandle(NULL, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, NULL, &fh);
                printf("FenceCreateSharedHandle -> 0x%08lx handle=%p\n", (unsigned long)hr, fh);
                if (SUCCEEDED(hr)) CloseHandle(fh);
                fres->Release();
            }
            fence->Release();
        }
        dev5->Release();
    }
}
