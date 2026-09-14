/*
 * DirectComposition surfaces
 *
 * Copyright 2026 Adobe Wine Lab
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "dcomp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dcomp);

static void surface_destroy(struct dcomp_surface *surface)
{
    if (surface->dxgi_surface) surface->dxgi_surface->lpVtbl->Release(surface->dxgi_surface);
    if (surface->texture) surface->texture->lpVtbl->Release(surface->texture);
    (&surface->device->IDCompositionDevice3_iface)->lpVtbl->Release(&surface->device->IDCompositionDevice3_iface);
    free(surface);
}

static HRESULT surface_query_interface(struct dcomp_surface *surface, REFIID riid, void **out)
{
    *out = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDCompositionSurface))
        *out = &surface->IDCompositionSurface_iface;
    else if (IsEqualIID(riid, &IID_IDCompositionVirtualSurface))
        *out = &surface->IDCompositionVirtualSurface_iface;

    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&surface->ref);
    return S_OK;
}

/* Create the backing texture. Surfaces are only drawable when the device was
 * given a rendering device, which is how a caller obtains one on Windows too. */
static HRESULT surface_ensure_texture(struct dcomp_surface *surface)
{
    D3D11_TEXTURE2D_DESC desc;
    HRESULT hr;

    if (surface->texture) return S_OK;

    if (!surface->device->d3d_device)
    {
        WARN("no rendering device was supplied to the composition device\n");
        return E_NOTIMPL;
    }
    if (!surface->width || !surface->height) return E_INVALIDARG;

    memset(&desc, 0, sizeof(desc));
    desc.Width = surface->width;
    desc.Height = surface->height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = surface->pixel_format;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

    hr = surface->device->d3d_device->lpVtbl->CreateTexture2D(surface->device->d3d_device, &desc, NULL, &surface->texture);
    if (FAILED(hr))
    {
        WARN("CreateTexture2D failed, hr %#lx\n", hr);
        return hr;
    }

    hr = surface->texture->lpVtbl->QueryInterface(surface->texture, &IID_IDXGISurface,
                                        (void **)&surface->dxgi_surface);
    if (FAILED(hr))
    {
        WARN("failed to get IDXGISurface, hr %#lx\n", hr);
        surface->texture->lpVtbl->Release(surface->texture);
        surface->texture = NULL;
        return hr;
    }

    TRACE("created %ux%u surface texture\n", surface->width, surface->height);
    return S_OK;
}

#define GEN_SURFACE(NAME, IF, CONV)                                                                     \
static HRESULT STDMETHODCALLTYPE NAME##_QueryInterface(IF *iface, REFIID riid, void **out)          \
{                                                                                                 \
    struct dcomp_surface *surface = CONV(iface);                                                  \
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);                       \
    return surface_query_interface(surface, riid, out);                                           \
}                                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_AddRef(IF *iface)                                             \
{                                                                                                 \
    return InterlockedIncrement(&CONV(iface)->ref);                                               \
}                                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_Release(IF *iface)                                            \
{                                                                                                 \
    struct dcomp_surface *surface = CONV(iface);                                                  \
    ULONG ref = InterlockedDecrement(&surface->ref);                                              \
    if (!ref) surface_destroy(surface);                                                           \
    return ref;                                                                                   \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_BeginDraw(IF *iface, const RECT *rect, REFIID iid,          \
        void **object, POINT *offset)                                                             \
{                                                                                                 \
    struct dcomp_surface *surface = CONV(iface);                                                  \
    HRESULT hr;                                                                                   \
    TRACE("iface %p, rect %p, iid %s, object %p, offset %p.\n", iface, rect,                      \
          debugstr_guid(iid), object, offset);                                                    \
    if (!object || !offset) return E_INVALIDARG;                                                  \
    if (surface->drawing) return E_NOT_VALID_STATE;                                               \
    if (FAILED(hr = surface_ensure_texture(surface))) return hr;                                  \
    if (FAILED(hr = surface->texture->lpVtbl->QueryInterface(surface->texture, iid, object))) return hr;    \
    offset->x = 0;                                                                                \
    offset->y = 0;                                                                                \
    surface->drawing = TRUE;                                                                      \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_EndDraw(IF *iface)                                          \
{                                                                                                 \
    struct dcomp_surface *surface = CONV(iface);                                                  \
    TRACE("iface %p.\n", iface);                                                                  \
    if (!surface->drawing) return E_NOT_VALID_STATE;                                              \
    surface->drawing = FALSE;                                                                     \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SuspendDraw(IF *iface)                                      \
{                                                                                                 \
    struct dcomp_surface *surface = CONV(iface);                                                  \
    TRACE("iface %p.\n", iface);                                                                  \
    if (!surface->drawing) return E_NOT_VALID_STATE;                                              \
    surface->drawing = FALSE;                                                                     \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_ResumeDraw(IF *iface)                                       \
{                                                                                                 \
    struct dcomp_surface *surface = CONV(iface);                                                  \
    TRACE("iface %p.\n", iface);                                                                  \
    if (!surface->texture) return E_NOT_VALID_STATE;                                              \
    surface->drawing = TRUE;                                                                      \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_Scroll(IF *iface, const RECT *scroll, const RECT *clip,     \
        int offset_x, int offset_y)                                                               \
{                                                                                                 \
    FIXME("surface scrolling is not supported yet\n");                                            \
    return E_NOTIMPL;                                                                             \
}

GEN_SURFACE(dcomp_surface1, IDCompositionSurface, surface_from_IDCompositionSurface)

#define GEN_VIRTUAL_SURFACE(NAME, IF, CONV)                                                             \
static HRESULT STDMETHODCALLTYPE NAME##_QueryInterface(IF *iface, REFIID riid, void **out)          \
{                                                                                                 \
    return surface_query_interface(CONV(iface), riid, out);                                       \
}                                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_AddRef(IF *iface)                                             \
{                                                                                                 \
    return InterlockedIncrement(&CONV(iface)->ref);                                               \
}                                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_Release(IF *iface)                                            \
{                                                                                                 \
    struct dcomp_surface *surface = CONV(iface);                                                  \
    ULONG ref = InterlockedDecrement(&surface->ref);                                              \
    if (!ref) surface_destroy(surface);                                                           \
    return ref;                                                                                   \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_BeginDraw(IF *iface, const RECT *rect, REFIID iid,          \
        void **object, POINT *offset)                                                             \
{                                                                                                 \
    return dcomp_surface1_BeginDraw(&CONV(iface)->IDCompositionSurface_iface, rect, iid,    \
                                          object, offset);                                        \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_EndDraw(IF *iface)                                          \
{                                                                                                 \
    return dcomp_surface1_EndDraw(&CONV(iface)->IDCompositionSurface_iface);                \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SuspendDraw(IF *iface)                                      \
{                                                                                                 \
    return dcomp_surface1_SuspendDraw(&CONV(iface)->IDCompositionSurface_iface);            \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_ResumeDraw(IF *iface)                                       \
{                                                                                                 \
    return dcomp_surface1_ResumeDraw(&CONV(iface)->IDCompositionSurface_iface);             \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_Scroll(IF *iface, const RECT *scroll, const RECT *clip,     \
        int offset_x, int offset_y)                                                               \
{                                                                                                 \
    return dcomp_surface1_Scroll(&CONV(iface)->IDCompositionSurface_iface, scroll, clip,    \
                                       offset_x, offset_y);                                       \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_Resize(IF *iface, UINT width, UINT height)                  \
{                                                                                                 \
    struct dcomp_surface *surface = CONV(iface);                                                  \
    TRACE("iface %p, width %u, height %u.\n", iface, width, height);                              \
    if (!width || !height) return E_INVALIDARG;                                                   \
    if (surface->drawing) return E_NOT_VALID_STATE;                                               \
    if (width == surface->width && height == surface->height) return S_OK;                        \
    if (surface->dxgi_surface) { surface->dxgi_surface->lpVtbl->Release(surface->dxgi_surface); surface->dxgi_surface = NULL; } \
    if (surface->texture) { surface->texture->lpVtbl->Release(surface->texture); surface->texture = NULL; } \
    surface->width = width;                                                                       \
    surface->height = height;                                                                     \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_Trim(IF *iface, const RECT *rectangles, UINT count)         \
{                                                                                                 \
    FIXME("virtual surface trimming is not supported yet\n");                                     \
    return E_NOTIMPL;                                                                             \
}

GEN_VIRTUAL_SURFACE(dcomp_vsurface, IDCompositionVirtualSurface, surface_from_IDCompositionVirtualSurface)

static const IDCompositionSurfaceVtbl dcomp_surface_vtbl =
{
    dcomp_surface1_QueryInterface,
    dcomp_surface1_AddRef,
    dcomp_surface1_Release,
    dcomp_surface1_BeginDraw,
    dcomp_surface1_EndDraw,
    dcomp_surface1_SuspendDraw,
    dcomp_surface1_ResumeDraw,
    dcomp_surface1_Scroll,
};

static const IDCompositionVirtualSurfaceVtbl dcomp_virtual_surface_vtbl =
{
    dcomp_vsurface_QueryInterface,
    dcomp_vsurface_AddRef,
    dcomp_vsurface_Release,
    dcomp_vsurface_BeginDraw,
    dcomp_vsurface_EndDraw,
    dcomp_vsurface_SuspendDraw,
    dcomp_vsurface_ResumeDraw,
    dcomp_vsurface_Scroll,
    dcomp_vsurface_Resize,
    dcomp_vsurface_Trim,
};

HRESULT dcomp_surface_create(struct dcomp_device *device, UINT width, UINT height,
                             DXGI_FORMAT format, DXGI_ALPHA_MODE alpha_mode,
                             BOOL virtual_surface, void **out)
{
    struct dcomp_surface *surface;

    if (!out) return E_INVALIDARG;
    if (!width || !height) return E_INVALIDARG;

    if (!(surface = calloc(1, sizeof(*surface)))) return E_OUTOFMEMORY;

    surface->IDCompositionSurface_iface.lpVtbl = &dcomp_surface_vtbl;
    surface->IDCompositionVirtualSurface_iface.lpVtbl = &dcomp_virtual_surface_vtbl;
    surface->ref = 1;
    surface->device = device;
    surface->width = width;
    surface->height = height;
    surface->pixel_format = format;
    surface->alpha_mode = alpha_mode;
    (&device->IDCompositionDevice3_iface)->lpVtbl->AddRef(&device->IDCompositionDevice3_iface);

    *out = virtual_surface ? (void *)&surface->IDCompositionVirtualSurface_iface
                           : (void *)&surface->IDCompositionSurface_iface;

    TRACE("created surface %p, %ux%u, format %u, virtual %d\n",
          surface, width, height, format, virtual_surface);
    return S_OK;
}

/* --- surface factory --------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE factory_QueryInterface(IDCompositionSurfaceFactory *iface, REFIID riid, void **out)
{
    struct dcomp_surface_factory *factory = CONTAINING_RECORD(iface, struct dcomp_surface_factory,
                                                              IDCompositionSurfaceFactory_iface);

    *out = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDCompositionSurfaceFactory))
        *out = &factory->IDCompositionSurfaceFactory_iface;
    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&factory->ref);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE factory_AddRef(IDCompositionSurfaceFactory *iface)
{
    struct dcomp_surface_factory *factory = CONTAINING_RECORD(iface, struct dcomp_surface_factory,
                                                              IDCompositionSurfaceFactory_iface);
    return InterlockedIncrement(&factory->ref);
}

static ULONG STDMETHODCALLTYPE factory_Release(IDCompositionSurfaceFactory *iface)
{
    struct dcomp_surface_factory *factory = CONTAINING_RECORD(iface, struct dcomp_surface_factory,
                                                              IDCompositionSurfaceFactory_iface);
    ULONG ref = InterlockedDecrement(&factory->ref);

    if (!ref)
    {
        (&factory->device->IDCompositionDevice3_iface)->lpVtbl->Release(&factory->device->IDCompositionDevice3_iface);
        free(factory);
    }
    return ref;
}

static HRESULT STDMETHODCALLTYPE factory_CreateSurface(IDCompositionSurfaceFactory *iface, UINT width,
        UINT height, DXGI_FORMAT pixel_format, DXGI_ALPHA_MODE alpha_mode, IDCompositionSurface **surface)
{
    struct dcomp_surface_factory *factory = CONTAINING_RECORD(iface, struct dcomp_surface_factory,
                                                              IDCompositionSurfaceFactory_iface);
    return dcomp_surface_create(factory->device, width, height, pixel_format, alpha_mode, FALSE,
                                (void **)surface);
}

static HRESULT STDMETHODCALLTYPE factory_CreateVirtualSurface(IDCompositionSurfaceFactory *iface,
        UINT width, UINT height, DXGI_FORMAT pixel_format, DXGI_ALPHA_MODE alpha_mode,
        IDCompositionVirtualSurface **surface)
{
    struct dcomp_surface_factory *factory = CONTAINING_RECORD(iface, struct dcomp_surface_factory,
                                                              IDCompositionSurfaceFactory_iface);
    return dcomp_surface_create(factory->device, width, height, pixel_format, alpha_mode, TRUE,
                                (void **)surface);
}

static const IDCompositionSurfaceFactoryVtbl dcomp_surface_factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    factory_CreateSurface,
    factory_CreateVirtualSurface,
};

HRESULT dcomp_surface_factory_create(struct dcomp_device *device, IDCompositionSurfaceFactory **out)
{
    struct dcomp_surface_factory *factory;

    if (!(factory = calloc(1, sizeof(*factory)))) return E_OUTOFMEMORY;

    factory->IDCompositionSurfaceFactory_iface.lpVtbl = &dcomp_surface_factory_vtbl;
    factory->ref = 1;
    factory->device = device;
    (&device->IDCompositionDevice3_iface)->lpVtbl->AddRef(&device->IDCompositionDevice3_iface);

    *out = &factory->IDCompositionSurfaceFactory_iface;
    return S_OK;
}
