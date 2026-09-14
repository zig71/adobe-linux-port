/* probe_dxgi.c — DXGI / D3D11 capability oracle probe.
 *
 * Why: Photoshop and Premiere select GPU paths from DXGI adapter enumeration,
 * feature levels and output descriptions. If Wine reports a different adapter
 * capability surface, applications take different code paths.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_dxgi.exe probe_dxgi.c -ldxgi -ld3d11 -lole32
 */
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <stdio.h>
#include <string.h>

static void ph(const char *k, HRESULT hr) { printf("%s=0x%08lx\n", k, (unsigned long)hr); }

static const char *fl_name(D3D_FEATURE_LEVEL fl) {
    switch (fl) {
    case D3D_FEATURE_LEVEL_1_0_CORE: return "1_0_CORE";
    case D3D_FEATURE_LEVEL_9_1: return "9_1";
    case D3D_FEATURE_LEVEL_9_2: return "9_2";
    case D3D_FEATURE_LEVEL_9_3: return "9_3";
    case D3D_FEATURE_LEVEL_10_0: return "10_0";
    case D3D_FEATURE_LEVEL_10_1: return "10_1";
    case D3D_FEATURE_LEVEL_11_0: return "11_0";
    case D3D_FEATURE_LEVEL_11_1: return "11_1";
    case D3D_FEATURE_LEVEL_12_0: return "12_0";
    case D3D_FEATURE_LEVEL_12_1: return "12_1";
    case D3D_FEATURE_LEVEL_12_2: return "12_2";
    default: return "unknown";
    }
}

int main(void) {
    IDXGIFactory1 *factory = NULL;
    HRESULT hr;
    UINT i;

    hr = CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory);
    ph("CreateDXGIFactory1", hr);
    if (FAILED(hr) || !factory) { printf("PROBE_RESULT=NO_FACTORY\n"); return 2; }

    {
        UINT count = 0;
        for (i = 0;; i++) {
            IDXGIAdapter1 *ad = NULL;
            if (factory->lpVtbl->EnumAdapters1(factory, i, &ad) != S_OK) break;
            count++;
            ad->lpVtbl->Release(ad);
        }
        printf("ADAPTER_COUNT=%u\n", count);
    }

    for (i = 0; i < 4; i++) {
        IDXGIAdapter1 *ad = NULL;
        DXGI_ADAPTER_DESC1 desc;
        char name[256];
        int j;
        if (factory->lpVtbl->EnumAdapters1(factory, i, &ad) != S_OK) break;
        memset(&desc, 0, sizeof(desc));
        if (SUCCEEDED(ad->lpVtbl->GetDesc1(ad, &desc))) {
            for (j = 0; j < 127 && desc.Description[j]; j++) name[j] = (char)desc.Description[j];
            name[j] = 0;
            printf("ADAPTER%u_NAME=%s\n", i, name);
            printf("ADAPTER%u_VENDOR=0x%08lx\n", i, (unsigned long)desc.VendorId);
            printf("ADAPTER%u_DEVICE=0x%08lx\n", i, (unsigned long)desc.DeviceId);
            printf("ADAPTER%u_SUBSYS=0x%08lx\n", i, (unsigned long)desc.SubSysId);
            printf("ADAPTER%u_DEDICATED_VRAM=%llu\n", i, (unsigned long long)desc.DedicatedVideoMemory);
            printf("ADAPTER%u_DEDICATED_SYS=%llu\n", i, (unsigned long long)desc.DedicatedSystemMemory);
            printf("ADAPTER%u_SHARED_SYS=%llu\n", i, (unsigned long long)desc.SharedSystemMemory);
            printf("ADAPTER%u_FLAGS=0x%08lx\n", i, (unsigned long)desc.Flags);
        }
        {
            UINT o;
            UINT outs = 0;
            for (o = 0; o < 4; o++) {
                IDXGIOutput *out = NULL;
                if (ad->lpVtbl->EnumOutputs(ad, o, &out) != S_OK) break;
                outs++;
                {
                    DXGI_OUTPUT_DESC od;
                    memset(&od, 0, sizeof(od));
                    if (SUCCEEDED(out->lpVtbl->GetDesc(out, &od))) {
                        RECT r = od.DesktopCoordinates;
                        printf("ADAPTER%u_OUTPUT%u_DESKTOP=%ld,%ld,%ld,%ld\n", i, o,
                               (long)r.left, (long)r.top, (long)r.right, (long)r.bottom);
                        printf("ADAPTER%u_OUTPUT%u_ATTACHED=%d\n", i, o, od.AttachedToDesktop ? 1 : 0);
                        printf("ADAPTER%u_OUTPUT%u_ROTATION=%d\n", i, o, (int)od.Rotation);
                    }
                }
                out->lpVtbl->Release(out);
            }
            printf("ADAPTER%u_OUTPUT_COUNT=%u\n", i, outs);
        }
        {
            LARGE_INTEGER umd;
            memset(&umd, 0, sizeof(umd));
            hr = ad->lpVtbl->CheckInterfaceSupport(ad, &IID_IDXGIDevice, &umd);
            printf("ADAPTER%u_CHECK_IDXGIDevice=0x%08lx umd=%lld\n", i, (unsigned long)hr,
                   (long long)umd.QuadPart);
        }
        ad->lpVtbl->Release(ad);
    }

    /* --- D3D11 device creation across feature levels --- */
    {
        static const D3D_FEATURE_LEVEL want[] = {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3
        };
        D3D_FEATURE_LEVEL got = (D3D_FEATURE_LEVEL)0;
        ID3D11Device *dev = NULL;
        ID3D11DeviceContext *ctx = NULL;
        UINT flags = 0;

        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, want,
                               (UINT)(sizeof(want) / sizeof(want[0])), D3D11_SDK_VERSION,
                               &dev, &got, &ctx);
        printf("D3D11_HARDWARE=0x%08lx level=%s\n", (unsigned long)hr, fl_name(got));
        if (dev) {
            D3D11_FEATURE_DATA_THREADING th;
            memset(&th, 0, sizeof(th));
            if (SUCCEEDED(dev->lpVtbl->CheckFeatureSupport(dev, D3D11_FEATURE_THREADING, &th, sizeof(th)))) {
                printf("D3D11_DRIVER_CONCURRENT=%d\n", th.DriverConcurrentCreates ? 1 : 0);
                printf("D3D11_COMMAND_LISTS=%d\n", th.DriverCommandLists ? 1 : 0);
            }
            {
                D3D11_FEATURE_DATA_DOUBLES d;
                memset(&d, 0, sizeof(d));
                if (SUCCEEDED(dev->lpVtbl->CheckFeatureSupport(dev, D3D11_FEATURE_DOUBLES, &d, sizeof(d))))
                    printf("D3D11_DOUBLE_PRECISION=%d\n", d.DoublePrecisionFloatShaderOps ? 1 : 0);
            }
            {
                D3D11_FEATURE_DATA_FORMAT_SUPPORT fs;
                memset(&fs, 0, sizeof(fs));
                fs.InFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                if (SUCCEEDED(dev->lpVtbl->CheckFeatureSupport(dev, D3D11_FEATURE_FORMAT_SUPPORT, &fs, sizeof(fs))))
                    printf("D3D11_RGBA8_SUPPORT=0x%08lx\n", (unsigned long)fs.OutFormatSupport);
            }
            {
                UINT mq = dev->lpVtbl->GetCreationFlags(dev);
                printf("D3D11_CREATION_FLAGS=0x%08lx\n", (unsigned long)mq);
            }
            ctx->lpVtbl->Release(ctx);
            dev->lpVtbl->Release(dev);
        }
    }
    {
        D3D_FEATURE_LEVEL got = (D3D_FEATURE_LEVEL)0;
        ID3D11Device *dev = NULL;
        ID3D11DeviceContext *ctx = NULL;
        static const D3D_FEATURE_LEVEL want[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0, want, 1, D3D11_SDK_VERSION, &dev, &got, &ctx);
        printf("D3D11_WARP=0x%08lx level=%s\n", (unsigned long)hr, fl_name(got));
        if (dev) { ctx->lpVtbl->Release(ctx); dev->lpVtbl->Release(dev); }
    }
    {
        D3D_FEATURE_LEVEL got = (D3D_FEATURE_LEVEL)0;
        ID3D11Device *dev = NULL;
        ID3D11DeviceContext *ctx = NULL;
        static const D3D_FEATURE_LEVEL want[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_NULL, NULL, 0, want, 1, D3D11_SDK_VERSION, &dev, &got, &ctx);
        printf("D3D11_NULL=0x%08lx level=%s\n", (unsigned long)hr, fl_name(got));
        if (dev) { ctx->lpVtbl->Release(ctx); dev->lpVtbl->Release(dev); }
    }

    factory->lpVtbl->Release(factory);
    printf("PROBE_RESULT=OK\n");
    return 0;
}
