// probe_d3d11: minimal D3D11 device creation. Same PE on Windows and Wine.
// Prints HRESULT + adapter description. Exit 0 on device, 1 otherwise.
#include <stdio.h>
#include <d3d11.h>
#include <dxgi.h>

int main(void) {
    IDXGIFactory *factory = NULL;
    HRESULT hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
    printf("CreateDXGIFactory -> 0x%08lx\n", (unsigned long)hr);
    if (FAILED(hr)) return 1;
    IDXGIAdapter *adapter = NULL;
    hr = IDXGIFactory_EnumAdapters(factory, 0, &adapter);
    printf("EnumAdapters(0) -> 0x%08lx\n", (unsigned long)hr);
    if (SUCCEEDED(hr)) {
        DXGI_ADAPTER_DESC desc;
        if (SUCCEEDED(IDXGIAdapter_GetDesc(adapter, &desc)))
            printf("adapter=%ls vendor=%04x device=%04x\n", desc.Description,
                   desc.VendorId, desc.DeviceId);
        IDXGIAdapter_Release(adapter);
    }
    ID3D11Device *dev = NULL;
    D3D_FEATURE_LEVEL got = 0;
    hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, NULL, 0,
                           D3D11_SDK_VERSION, &dev, &got, NULL);
    printf("D3D11CreateDevice(HW,BGRA) -> 0x%08lx level=0x%x\n",
           (unsigned long)hr, got);
    if (dev) ID3D11Device_Release(dev);
    IDXGIFactory_Release(factory);
    return SUCCEEDED(hr) ? 0 : 1;
}
