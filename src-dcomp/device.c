/*
 * DirectComposition device
 *
 * Copyright 2020 Nikolay Sivov for CodeWeavers
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

/* --- shared implementations, taking the device directly --------------- */

static HRESULT device_commit(struct dcomp_device *device)
{
    device->valid = TRUE;
    return S_OK;
}

static HRESULT device_get_frame_statistics(struct dcomp_device *device,
                                           DCOMPOSITION_FRAME_STATISTICS *statistics)
{
    if (!statistics) return E_INVALIDARG;
    memset(statistics, 0, sizeof(*statistics));
    return S_OK;
}

static HRESULT device_create_target_for_hwnd(struct dcomp_device *device, HWND hwnd,
                                             BOOL topmost, IDCompositionTarget **target)
{
    if (!target) return E_INVALIDARG;
    if (!hwnd || !IsWindow(hwnd)) return E_INVALIDARG;
    if (topmost) FIXME("topmost is not supported yet\n");
    return dcomp_target_create(device, target);
}

static HRESULT device_create_visual2(struct dcomp_device *device, IDCompositionVisual2 **visual)
{
    if (!visual) return E_INVALIDARG;
    return dcomp_visual_create(device, TRUE, (void **)visual);
}

static HRESULT device_create_visual1(struct dcomp_device *device, IDCompositionVisual **visual)
{
    if (!visual) return E_INVALIDARG;
    return dcomp_visual_create(device, FALSE, (void **)visual);
}

static HRESULT device_create_surface_factory(struct dcomp_device *device,
                                             IDCompositionSurfaceFactory **factory)
{
    if (!factory) return E_INVALIDARG;
    return dcomp_surface_factory_create(device, factory);
}

static HRESULT device_create_surface(struct dcomp_device *device, UINT width, UINT height,
                                     DXGI_FORMAT format, DXGI_ALPHA_MODE alpha_mode,
                                     IDCompositionSurface **surface)
{
    if (!surface) return E_INVALIDARG;
    return dcomp_surface_create(device, width, height, format, alpha_mode, FALSE, (void **)surface);
}

static HRESULT device_create_virtual_surface(struct dcomp_device *device, UINT width, UINT height,
                                             DXGI_FORMAT format, DXGI_ALPHA_MODE alpha_mode,
                                             IDCompositionVirtualSurface **surface)
{
    if (!surface) return E_INVALIDARG;
    return dcomp_surface_create(device, width, height, format, alpha_mode, TRUE, (void **)surface);
}

static HRESULT device_create_animation(struct dcomp_device *device, IDCompositionAnimation **animation)
{
    if (!animation) return E_INVALIDARG;
    return dcomp_animation_create(animation);
}

static HRESULT device_create_effect_group(struct dcomp_device *device, IDCompositionEffectGroup **effect_group)
{
    if (!effect_group) return E_INVALIDARG;
    return dcomp_effect_group_create(effect_group);
}

static HRESULT device_create_rectangle_clip(struct dcomp_device *device, IDCompositionRectangleClip **clip)
{
    if (!clip) return E_INVALIDARG;
    return dcomp_rect_clip_create(clip);
}

static HRESULT device_create_transform(struct dcomp_device *device, enum dcomp_transform_kind kind,
                                       void **transform)
{
    if (!transform) return E_INVALIDARG;
    return dcomp_transform_create(kind, transform);
}

static HRESULT device_check_device_state(struct dcomp_device *device, BOOL *valid)
{
    if (!valid) return E_INVALIDARG;
    *valid = device->valid;
    return S_OK;
}

static HRESULT device_create_surface_from_handle(struct dcomp_device *device, HANDLE handle,
                                                 IUnknown **surface)
{
    if (!surface) return E_INVALIDARG;
    FIXME("handle %p: cross-process surfaces are not supported yet\n", handle);
    return E_NOTIMPL;
}

static HRESULT device_create_surface_from_hwnd(struct dcomp_device *device, HWND hwnd,
                                               IUnknown **surface)
{
    if (!surface) return E_INVALIDARG;
    FIXME("hwnd %p: window content surfaces are not supported yet\n", hwnd);
    return E_NOTIMPL;
}

/* --- method generation -------------------------------------------------
 *
 * A derived interface's vtable declares its inherited methods with the
 * derived interface type, so the three exposed interfaces cannot share
 * function pointers and are generated from a common body instead. */

#define GEN_DEVICE2_CORE(NAME, IF, CONV)                                                              \
static HRESULT STDMETHODCALLTYPE NAME##_Commit(IF *iface)                                         \
{                                                                                               \
    struct dcomp_device *device = CONV(iface);                                                  \
    TRACE("iface %p.\n", iface);                                                                \
    return device_commit(device);                                                               \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_WaitForCommitCompletion(IF *iface)                        \
{                                                                                               \
    TRACE("iface %p.\n", iface);                                                                \
    return S_OK;                                                                                \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_GetFrameStatistics(IF *iface,                             \
        DCOMPOSITION_FRAME_STATISTICS *statistics)                                              \
{                                                                                               \
    return device_get_frame_statistics(CONV(iface), statistics);                                \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateVisual(IF *iface, IDCompositionVisual2 **visual)    \
{                                                                                               \
    return device_create_visual2(CONV(iface), visual);                                          \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSurfaceFactory(IF *iface, IUnknown *rendering_device, \
        IDCompositionSurfaceFactory **surface_factory)                                          \
{                                                                                               \
    return device_create_surface_factory(CONV(iface), surface_factory);                         \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSurface(IF *iface, UINT width, UINT height,         \
        DXGI_FORMAT pixel_format, DXGI_ALPHA_MODE alpha_mode, IDCompositionSurface **surface)   \
{                                                                                               \
    return device_create_surface(CONV(iface), width, height, pixel_format, alpha_mode, surface);\
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateVirtualSurface(IF *iface, UINT width, UINT height,   \
        DXGI_FORMAT pixel_format, DXGI_ALPHA_MODE alpha_mode, IDCompositionVirtualSurface **surface) \
{                                                                                               \
    return device_create_virtual_surface(CONV(iface), width, height, pixel_format, alpha_mode,   \
                                         surface);                                              \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTranslateTransform(IF *iface,                       \
        IDCompositionTranslateTransform **transform)                                            \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_TRANSLATE, (void **)transform);  \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateScaleTransform(IF *iface,                           \
        IDCompositionScaleTransform **transform)                                                \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_SCALE, (void **)transform);      \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateRotateTransform(IF *iface,                          \
        IDCompositionRotateTransform **transform)                                               \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_ROTATE, (void **)transform);     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSkewTransform(IF *iface,                            \
        IDCompositionSkewTransform **transform)                                                 \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_SKEW, (void **)transform);       \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateMatrixTransform(IF *iface,                          \
        IDCompositionMatrixTransform **transform)                                               \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_MATRIX, (void **)transform);     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTransformGroup(IF *iface,                           \
        IDCompositionTransform **transforms, UINT elements, IDCompositionTransform **transform_group) \
{                                                                                               \
    FIXME("transform groups are not supported yet\n");                                          \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTranslateTransform3D(IF *iface,                     \
        IDCompositionTranslateTransform3D **transform_3d)                                       \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateScaleTransform3D(IF *iface,                         \
        IDCompositionScaleTransform3D **transform_3d)                                           \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateRotateTransform3D(IF *iface,                        \
        IDCompositionRotateTransform3D **transform_3d)                                          \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateMatrixTransform3D(IF *iface,                        \
        IDCompositionMatrixTransform3D **transform_3d)                                          \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTransform3DGroup(IF *iface,                         \
        IDCompositionTransform3D **transforms_3d, UINT elements, IDCompositionTransform3D **transform_3d_group) \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateEffectGroup(IF *iface,                              \
        IDCompositionEffectGroup **effect_group)                                                \
{                                                                                               \
    return device_create_effect_group(CONV(iface), effect_group);                               \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateRectangleClip(IF *iface,                            \
        IDCompositionRectangleClip **clip)                                                      \
{                                                                                               \
    return device_create_rectangle_clip(CONV(iface), clip);                                     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateAnimation(IF *iface, IDCompositionAnimation **animation) \
{                                                                                               \
    return device_create_animation(CONV(iface), animation);                                     \
}

#define GEN_DEVICE1(NAME, IF, CONV)                                                                   \
static HRESULT STDMETHODCALLTYPE NAME##_Commit(IF *iface)                                         \
{                                                                                               \
    return device_commit(CONV(iface));                                                          \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_WaitForCommitCompletion(IF *iface)                        \
{                                                                                               \
    return S_OK;                                                                                \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_GetFrameStatistics(IF *iface,                             \
        DCOMPOSITION_FRAME_STATISTICS *statistics)                                              \
{                                                                                               \
    return device_get_frame_statistics(CONV(iface), statistics);                                \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTargetForHwnd(IF *iface, HWND hwnd, BOOL topmost,   \
        IDCompositionTarget **target)                                                           \
{                                                                                               \
    return device_create_target_for_hwnd(CONV(iface), hwnd, topmost, target);                   \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateVisual(IF *iface, IDCompositionVisual **visual)     \
{                                                                                               \
    return device_create_visual1(CONV(iface), visual);                                          \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSurface(IF *iface, UINT width, UINT height,         \
        DXGI_FORMAT pixel_format, DXGI_ALPHA_MODE alpha_mode, IDCompositionSurface **surface)   \
{                                                                                               \
    return device_create_surface(CONV(iface), width, height, pixel_format, alpha_mode, surface);\
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateVirtualSurface(IF *iface, UINT width, UINT height,   \
        DXGI_FORMAT pixel_format, DXGI_ALPHA_MODE alpha_mode, IDCompositionVirtualSurface **surface) \
{                                                                                               \
    return device_create_virtual_surface(CONV(iface), width, height, pixel_format, alpha_mode,   \
                                         surface);                                              \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSurfaceFromHandle(IF *iface, HANDLE handle,         \
        IUnknown **surface)                                                                     \
{                                                                                               \
    return device_create_surface_from_handle(CONV(iface), handle, surface);                     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSurfaceFromHwnd(IF *iface, HWND hwnd, IUnknown **surface) \
{                                                                                               \
    return device_create_surface_from_hwnd(CONV(iface), hwnd, surface);                         \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTranslateTransform(IF *iface,                       \
        IDCompositionTranslateTransform **transform)                                            \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_TRANSLATE, (void **)transform);  \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateScaleTransform(IF *iface,                           \
        IDCompositionScaleTransform **transform)                                                \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_SCALE, (void **)transform);      \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateRotateTransform(IF *iface,                          \
        IDCompositionRotateTransform **transform)                                               \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_ROTATE, (void **)transform);     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSkewTransform(IF *iface,                            \
        IDCompositionSkewTransform **transform)                                                 \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_SKEW, (void **)transform);       \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateMatrixTransform(IF *iface,                          \
        IDCompositionMatrixTransform **transform)                                               \
{                                                                                               \
    return device_create_transform(CONV(iface), DCOMP_TRANSFORM_MATRIX, (void **)transform);     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTransformGroup(IF *iface,                           \
        IDCompositionTransform **transforms, UINT elements, IDCompositionTransform **transform_group) \
{                                                                                               \
    FIXME("transform groups are not supported yet\n");                                          \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTranslateTransform3D(IF *iface,                     \
        IDCompositionTranslateTransform3D **transform_3d)                                       \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateScaleTransform3D(IF *iface,                         \
        IDCompositionScaleTransform3D **transform_3d)                                           \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateRotateTransform3D(IF *iface,                        \
        IDCompositionRotateTransform3D **transform_3d)                                          \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateMatrixTransform3D(IF *iface,                        \
        IDCompositionMatrixTransform3D **transform_3d)                                          \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTransform3DGroup(IF *iface,                         \
        IDCompositionTransform3D **transforms_3d, UINT elements, IDCompositionTransform3D **transform_3d_group) \
{                                                                                               \
    FIXME("3D transforms are not supported yet\n");                                             \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateEffectGroup(IF *iface,                              \
        IDCompositionEffectGroup **effect_group)                                                \
{                                                                                               \
    return device_create_effect_group(CONV(iface), effect_group);                               \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateRectangleClip(IF *iface,                            \
        IDCompositionRectangleClip **clip)                                                      \
{                                                                                               \
    return device_create_rectangle_clip(CONV(iface), clip);                                     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateAnimation(IF *iface, IDCompositionAnimation **animation) \
{                                                                                               \
    return device_create_animation(CONV(iface), animation);                                     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CheckDeviceState(IF *iface, BOOL *valid)                  \
{                                                                                               \
    return device_check_device_state(CONV(iface), valid);                                       \
}

#define GEN_DESKTOP_EXTRA(NAME, IF, CONV)                                                             \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTargetForHwnd(IF *iface, HWND hwnd, BOOL topmost,   \
        IDCompositionTarget **target)                                                           \
{                                                                                               \
    return device_create_target_for_hwnd(CONV(iface), hwnd, topmost, target);                   \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSurfaceFromHandle(IF *iface, HANDLE handle,         \
        IUnknown **surface)                                                                     \
{                                                                                               \
    return device_create_surface_from_handle(CONV(iface), handle, surface);                     \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSurfaceFromHwnd(IF *iface, HWND hwnd, IUnknown **surface) \
{                                                                                               \
    return device_create_surface_from_hwnd(CONV(iface), hwnd, surface);                         \
}

#define GEN_DEVICE3_EXTRA(NAME, IF)                                                                   \
static HRESULT STDMETHODCALLTYPE NAME##_CreateGaussianBlurEffect(IF *iface,                       \
        IDCompositionGaussianBlurEffect **effect)                                               \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateBrightnessEffect(IF *iface,                         \
        IDCompositionBrightnessEffect **effect)                                                 \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateColorMatrixEffect(IF *iface,                        \
        IDCompositionColorMatrixEffect **effect)                                                \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateShadowEffect(IF *iface,                             \
        IDCompositionShadowEffect **effect)                                                     \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateHueRotationEffect(IF *iface,                        \
        IDCompositionHueRotationEffect **effect)                                                \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateSaturationEffect(IF *iface,                         \
        IDCompositionSaturationEffect **effect)                                                 \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTurbulenceEffect(IF *iface,                         \
        IDCompositionTurbulenceEffect **effect)                                                 \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateLinearTransferEffect(IF *iface,                     \
        IDCompositionLinearTransferEffect **effect)                                             \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateTableTransferEffect(IF *iface,                      \
        IDCompositionTableTransferEffect **effect)                                              \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateCompositeEffect(IF *iface,                          \
        IDCompositionCompositeEffect **effect)                                                  \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateBlendEffect(IF *iface,                              \
        IDCompositionBlendEffect **effect)                                                      \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateArithmeticCompositeEffect(IF *iface,                \
        IDCompositionArithmeticCompositeEffect **effect)                                        \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}                                                                                               \
static HRESULT STDMETHODCALLTYPE NAME##_CreateAffineTransform2DEffect(IF *iface,                  \
        IDCompositionAffineTransform2DEffect **effect)                                          \
{                                                                                               \
    FIXME("filter effects are not supported yet\n");                                            \
    return E_NOTIMPL;                                                                           \
}

GEN_DEVICE1(dcomp_device1, IDCompositionDevice, device_from_IDCompositionDevice)
GEN_DEVICE2_CORE(dcomp_device3, IDCompositionDevice3, device_from_IDCompositionDevice3)
GEN_DEVICE3_EXTRA(dcomp_device3, IDCompositionDevice3)
GEN_DEVICE2_CORE(dcomp_desktop, IDCompositionDesktopDevice, device_from_IDCompositionDesktopDevice)
GEN_DESKTOP_EXTRA(dcomp_desktop, IDCompositionDesktopDevice, device_from_IDCompositionDesktopDevice)

/* --- IUnknown, one set per exposed interface --------------------------- */

static HRESULT device_query_interface(struct dcomp_device *device, REFIID riid, void **out)
{
    *out = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDCompositionDevice))
        *out = &device->IDCompositionDevice_iface;
    else if (IsEqualIID(riid, &IID_IDCompositionDevice3))
        *out = &device->IDCompositionDevice3_iface;
    else if (IsEqualIID(riid, &IID_IDCompositionDesktopDevice) ||
             IsEqualIID(riid, &IID_IDCompositionDevice2))
        *out = &device->IDCompositionDesktopDevice_iface;

    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&device->ref);
    return S_OK;
}

#define GEN_IUNKNOWN(NAME, IF, CONV)                                                    \
static HRESULT STDMETHODCALLTYPE NAME##_QueryInterface(IF *iface, REFIID riid, void **out) \
{                                                                                 \
    struct dcomp_device *device = CONV(iface);                                    \
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);       \
    return device_query_interface(device, riid, out);                             \
}                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_AddRef(IF *iface)                             \
{                                                                                 \
    struct dcomp_device *device = CONV(iface);                                    \
    ULONG ref = InterlockedIncrement(&device->ref);                               \
    TRACE("iface %p, ref %lu.\n", iface, ref);                                     \
    return ref;                                                                   \
}                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_Release(IF *iface)                            \
{                                                                                 \
    struct dcomp_device *device = CONV(iface);                                    \
    ULONG ref = InterlockedDecrement(&device->ref);                               \
    TRACE("iface %p, ref %lu.\n", iface, ref);                                     \
    if (!ref)                                                                     \
    {                                                                             \
        if (device->d3d_device) device->d3d_device->lpVtbl->Release(device->d3d_device);         \
        if (device->rendering_device) device->rendering_device->lpVtbl->Release(device->rendering_device); \
        free(device);                                                             \
    }                                                                             \
    return ref;                                                                   \
}

GEN_IUNKNOWN(dcomp_device1, IDCompositionDevice, device_from_IDCompositionDevice)
GEN_IUNKNOWN(dcomp_device3, IDCompositionDevice3, device_from_IDCompositionDevice3)
GEN_IUNKNOWN(dcomp_desktop, IDCompositionDesktopDevice, device_from_IDCompositionDesktopDevice)

/* --- vtables ----------------------------------------------------------- */

static const IDCompositionDeviceVtbl dcomp_device_vtbl =
{
    dcomp_device1_QueryInterface,
    dcomp_device1_AddRef,
    dcomp_device1_Release,
    dcomp_device1_Commit,
    dcomp_device1_WaitForCommitCompletion,
    dcomp_device1_GetFrameStatistics,
    dcomp_device1_CreateTargetForHwnd,
    dcomp_device1_CreateVisual,
    dcomp_device1_CreateSurface,
    dcomp_device1_CreateVirtualSurface,
    dcomp_device1_CreateSurfaceFromHandle,
    dcomp_device1_CreateSurfaceFromHwnd,
    dcomp_device1_CreateTranslateTransform,
    dcomp_device1_CreateScaleTransform,
    dcomp_device1_CreateRotateTransform,
    dcomp_device1_CreateSkewTransform,
    dcomp_device1_CreateMatrixTransform,
    dcomp_device1_CreateTransformGroup,
    dcomp_device1_CreateTranslateTransform3D,
    dcomp_device1_CreateScaleTransform3D,
    dcomp_device1_CreateRotateTransform3D,
    dcomp_device1_CreateMatrixTransform3D,
    dcomp_device1_CreateTransform3DGroup,
    dcomp_device1_CreateEffectGroup,
    dcomp_device1_CreateRectangleClip,
    dcomp_device1_CreateAnimation,
    dcomp_device1_CheckDeviceState,
};

static const IDCompositionDevice3Vtbl dcomp_device3_vtbl =
{
    dcomp_device3_QueryInterface,
    dcomp_device3_AddRef,
    dcomp_device3_Release,
    dcomp_device3_Commit,
    dcomp_device3_WaitForCommitCompletion,
    dcomp_device3_GetFrameStatistics,
    dcomp_device3_CreateVisual,
    dcomp_device3_CreateSurfaceFactory,
    dcomp_device3_CreateSurface,
    dcomp_device3_CreateVirtualSurface,
    dcomp_device3_CreateTranslateTransform,
    dcomp_device3_CreateScaleTransform,
    dcomp_device3_CreateRotateTransform,
    dcomp_device3_CreateSkewTransform,
    dcomp_device3_CreateMatrixTransform,
    dcomp_device3_CreateTransformGroup,
    dcomp_device3_CreateTranslateTransform3D,
    dcomp_device3_CreateScaleTransform3D,
    dcomp_device3_CreateRotateTransform3D,
    dcomp_device3_CreateMatrixTransform3D,
    dcomp_device3_CreateTransform3DGroup,
    dcomp_device3_CreateEffectGroup,
    dcomp_device3_CreateRectangleClip,
    dcomp_device3_CreateAnimation,
    dcomp_device3_CreateGaussianBlurEffect,
    dcomp_device3_CreateBrightnessEffect,
    dcomp_device3_CreateColorMatrixEffect,
    dcomp_device3_CreateShadowEffect,
    dcomp_device3_CreateHueRotationEffect,
    dcomp_device3_CreateSaturationEffect,
    dcomp_device3_CreateTurbulenceEffect,
    dcomp_device3_CreateLinearTransferEffect,
    dcomp_device3_CreateTableTransferEffect,
    dcomp_device3_CreateCompositeEffect,
    dcomp_device3_CreateBlendEffect,
    dcomp_device3_CreateArithmeticCompositeEffect,
    dcomp_device3_CreateAffineTransform2DEffect,
};

static const IDCompositionDesktopDeviceVtbl dcomp_desktop_device_vtbl =
{
    dcomp_desktop_QueryInterface,
    dcomp_desktop_AddRef,
    dcomp_desktop_Release,
    dcomp_desktop_Commit,
    dcomp_desktop_WaitForCommitCompletion,
    dcomp_desktop_GetFrameStatistics,
    dcomp_desktop_CreateVisual,
    dcomp_desktop_CreateSurfaceFactory,
    dcomp_desktop_CreateSurface,
    dcomp_desktop_CreateVirtualSurface,
    dcomp_desktop_CreateTranslateTransform,
    dcomp_desktop_CreateScaleTransform,
    dcomp_desktop_CreateRotateTransform,
    dcomp_desktop_CreateSkewTransform,
    dcomp_desktop_CreateMatrixTransform,
    dcomp_desktop_CreateTransformGroup,
    dcomp_desktop_CreateTranslateTransform3D,
    dcomp_desktop_CreateScaleTransform3D,
    dcomp_desktop_CreateRotateTransform3D,
    dcomp_desktop_CreateMatrixTransform3D,
    dcomp_desktop_CreateTransform3DGroup,
    dcomp_desktop_CreateEffectGroup,
    dcomp_desktop_CreateRectangleClip,
    dcomp_desktop_CreateAnimation,
    dcomp_desktop_CreateTargetForHwnd,
    dcomp_desktop_CreateSurfaceFromHandle,
    dcomp_desktop_CreateSurfaceFromHwnd,
};

/* --- creation ---------------------------------------------------------- */

static HRESULT dcomp_device_create(IUnknown *rendering_device, REFIID iid, void **out)
{
    struct dcomp_device *device;
    HRESULT hr;

    if (!out) return E_INVALIDARG;
    *out = NULL;

    if (!(device = calloc(1, sizeof(*device)))) return E_OUTOFMEMORY;

    device->IDCompositionDevice_iface.lpVtbl = &dcomp_device_vtbl;
    device->IDCompositionDevice3_iface.lpVtbl = &dcomp_device3_vtbl;
    device->IDCompositionDesktopDevice_iface.lpVtbl = &dcomp_desktop_device_vtbl;
    device->ref = 1;
    device->valid = TRUE;

    if (rendering_device)
    {
        device->rendering_device = rendering_device;
        rendering_device->lpVtbl->AddRef(rendering_device);

        /* Cache a D3D11 device so surfaces can be created; a rendering device
         * may be the D3D11 device itself or a DXGI device whose parent it is. */
        if (FAILED(rendering_device->lpVtbl->QueryInterface(rendering_device, &IID_ID3D11Device,
                                           (void **)&device->d3d_device)))
        {
            IDXGIDevice *dxgi_device;
            if (SUCCEEDED(rendering_device->lpVtbl->QueryInterface(rendering_device, &IID_IDXGIDevice,
                                                  (void **)&dxgi_device)))
            {
                dxgi_device->lpVtbl->GetParent(dxgi_device, &IID_ID3D11Device, (void **)&device->d3d_device);
                dxgi_device->lpVtbl->Release(dxgi_device);
            }
        }
    }

    hr = device_query_interface(device, iid, out);
    dcomp_device1_Release(&device->IDCompositionDevice_iface);
    if (FAILED(hr))
    {
        WARN("unsupported interface %s\n", debugstr_guid(iid));
        return hr;
    }

    TRACE("created device %p for iid %s\n", device, debugstr_guid(iid));
    return S_OK;
}

/***********************************************************************
 *           DCompositionCreateDevice (dcomp.@)
 */
HRESULT WINAPI DCompositionCreateDevice(IDXGIDevice *dxgi_device, REFIID iid, void **device)
{
    TRACE("dxgi_device %p, iid %s, device %p.\n", dxgi_device, debugstr_guid(iid), device);

    return dcomp_device_create((IUnknown *)dxgi_device, iid, device);
}

/***********************************************************************
 *           DCompositionCreateDevice2 (dcomp.@)
 */
HRESULT WINAPI DCompositionCreateDevice2(IUnknown *rendering_device, REFIID iid, void **device)
{
    TRACE("rendering_device %p, iid %s, device %p.\n", rendering_device, debugstr_guid(iid), device);

    return dcomp_device_create(rendering_device, iid, device);
}

/***********************************************************************
 *           DCompositionCreateDevice3 (dcomp.@)
 */
HRESULT WINAPI DCompositionCreateDevice3(IUnknown *rendering_device, REFIID iid, void **device)
{
    TRACE("rendering_device %p, iid %s, device %p.\n", rendering_device, debugstr_guid(iid), device);

    return dcomp_device_create(rendering_device, iid, device);
}

/***********************************************************************
 *           DCompositionCreateSurfaceHandle (dcomp.@)
 *
 * Cross-process composition surfaces are not implemented; returning a handle
 * that no other entry point can consume would be worse than failing, so this
 * reports the gap explicitly.
 */
HRESULT WINAPI DCompositionCreateSurfaceHandle(DWORD desired_access,
                                               SECURITY_ATTRIBUTES *security_attributes,
                                               HANDLE *surface_handle)
{
    FIXME("desired_access %#lx, security_attributes %p, surface_handle %p: not supported yet\n",
          desired_access, security_attributes, surface_handle);

    if (!surface_handle) return E_INVALIDARG;
    *surface_handle = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *           DCompositionWaitForCompositorClock (dcomp.@)
 *
 * Waits for the next compositor clock tick, or for one of the supplied
 * handles. Without a compositor clock to synchronise with, the wait is
 * performed on the caller's handles alone, which preserves the contract that
 * the call does not return before one of its inputs is ready.
 */
DWORD WINAPI DCompositionWaitForCompositorClock(UINT count, const HANDLE *handles, DWORD timeout)
{
    TRACE("count %u, handles %p, timeout %lu.\n", count, handles, timeout);

    if (count && !handles) return 0;
    if (!count)
    {
        if (timeout == INFINITE) return 0;
        Sleep(timeout);
        return 0;
    }

    return WaitForMultipleObjects(count, handles, FALSE, timeout);
}
