/*
 * DirectComposition private definitions
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

#ifndef __DCOMP_PRIVATE_H
#define __DCOMP_PRIVATE_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "d3d11.h"
#include "dxgi.h"
/* shobjidl.h, pulled in by the generated headers, includes msxml.h. Its two
 * interface GUIDs would then be defined in every translation unit of this
 * module as well as in libuuid, which the linker rejects, so those two blocks
 * are skipped. */
#define __IXMLError_INTERFACE_DEFINED__
#define __IXMLDOMParseError_INTERFACE_DEFINED__
#include "dcomp.h"

#include "wine/debug.h"
#include "wine/list.h"

struct dcomp_device;

struct dcomp_visual_child
{
    struct list entry;
    IDCompositionVisual *visual;
};

struct dcomp_target
{
    IDCompositionTarget IDCompositionTarget_iface;
    LONG ref;
    struct dcomp_device *device;
    IDCompositionVisual *root;
};

struct dcomp_visual
{
    IDCompositionVisual IDCompositionVisual_iface;
    IDCompositionVisual2 IDCompositionVisual2_iface;
    LONG ref;
    struct dcomp_device *device;
    float offset_x, offset_y;
    IDCompositionTransform *transform;
    IDCompositionVisual *transform_parent;
    IDCompositionEffect *effect;
    IDCompositionClip *clip;
    enum DCOMPOSITION_BITMAP_INTERPOLATION_MODE interpolation_mode;
    enum DCOMPOSITION_BORDER_MODE border_mode;
    enum DCOMPOSITION_COMPOSITE_MODE composite_mode;
    enum DCOMPOSITION_OPACITY_MODE opacity_mode;
    enum DCOMPOSITION_BACKFACE_VISIBILITY backface_visibility;
    IUnknown *content;
    D2D_MATRIX_3X2_F matrix;
    BOOL has_matrix;
    D2D_RECT_F clip_rect;
    BOOL has_clip_rect;
    struct list children;
};

struct dcomp_surface
{
    IDCompositionSurface IDCompositionSurface_iface;
    IDCompositionVirtualSurface IDCompositionVirtualSurface_iface;
    LONG ref;
    struct dcomp_device *device;
    UINT width, height;
    DXGI_FORMAT pixel_format;
    DXGI_ALPHA_MODE alpha_mode;
    ID3D11Texture2D *texture;
    IDXGISurface *dxgi_surface;
    BOOL drawing;
};

struct dcomp_surface_factory
{
    IDCompositionSurfaceFactory IDCompositionSurfaceFactory_iface;
    LONG ref;
    struct dcomp_device *device;
};

struct dcomp_animation
{
    IDCompositionAnimation IDCompositionAnimation_iface;
    LONG ref;
};

struct dcomp_effect_group
{
    IDCompositionEffectGroup IDCompositionEffectGroup_iface;
    LONG ref;
    float opacity;
    IDCompositionTransform3D *transform_3d;
};

struct dcomp_rect_clip
{
    IDCompositionRectangleClip IDCompositionRectangleClip_iface;
    LONG ref;
    float left, top, right, bottom;
    float tlx, tly, trx, try_, blx, bly, brx, bry;
};

/* Transform kinds, shared between the device entry points and transform.c. */
enum dcomp_transform_kind
{
    DCOMP_TRANSFORM_TRANSLATE = 1,
    DCOMP_TRANSFORM_SCALE,
    DCOMP_TRANSFORM_MATRIX,
    DCOMP_TRANSFORM_SKEW,
    DCOMP_TRANSFORM_ROTATE,
    DCOMP_TRANSFORM_TRANSLATE_3D,
    DCOMP_TRANSFORM_SCALE_3D,
    DCOMP_TRANSFORM_MATRIX_3D,
    DCOMP_TRANSFORM_GROUP,
};

struct dcomp_transform
{
    /* One member per interface this object can expose. Only the member matching
     * `kind` has a meaningful vtable. */
    IDCompositionTranslateTransform IDCompositionTranslateTransform_iface;
    IDCompositionScaleTransform IDCompositionScaleTransform_iface;
    IDCompositionMatrixTransform IDCompositionMatrixTransform_iface;
    IDCompositionSkewTransform IDCompositionSkewTransform_iface;
    IDCompositionRotateTransform IDCompositionRotateTransform_iface;
    IDCompositionTranslateTransform3D IDCompositionTranslateTransform3D_iface;
    IDCompositionScaleTransform3D IDCompositionScaleTransform3D_iface;
    IDCompositionMatrixTransform3D IDCompositionMatrixTransform3D_iface;
    LONG ref;
    enum dcomp_transform_kind kind;
    float off_x, off_y, off_z;
    float scale_x, scale_y, scale_z, center_x, center_y, center_z;
    float angle, angle_x, angle_y, skew_x, skew_y;
    D2D_MATRIX_3X2_F matrix;
    D2D_MATRIX_4X4_F matrix_3d;
    IDCompositionTransform **members;
    UINT member_count;
};

struct dcomp_device
{
    IDCompositionDevice IDCompositionDevice_iface;
    IDCompositionDevice3 IDCompositionDevice3_iface;
    IDCompositionDesktopDevice IDCompositionDesktopDevice_iface;
    LONG ref;

    /* The rendering device the composition device was created with; may be
     * NULL. A D3D11 device is cached for surface creation. */
    IUnknown *rendering_device;
    ID3D11Device *d3d_device;
    BOOL valid;
};

static inline struct dcomp_device *device_from_IDCompositionDevice(IDCompositionDevice *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_device, IDCompositionDevice_iface);
}

static inline struct dcomp_device *device_from_IDCompositionDevice3(IDCompositionDevice3 *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_device, IDCompositionDevice3_iface);
}

static inline struct dcomp_device *device_from_IDCompositionDesktopDevice(IDCompositionDesktopDevice *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_device, IDCompositionDesktopDevice_iface);
}

static inline struct dcomp_target *target_from_IDCompositionTarget(IDCompositionTarget *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_target, IDCompositionTarget_iface);
}

static inline struct dcomp_visual *visual_from_IDCompositionVisual(IDCompositionVisual *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_visual, IDCompositionVisual_iface);
}

static inline struct dcomp_visual *visual_from_IDCompositionVisual2(IDCompositionVisual2 *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_visual, IDCompositionVisual2_iface);
}

static inline struct dcomp_surface *surface_from_IDCompositionSurface(IDCompositionSurface *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_surface, IDCompositionSurface_iface);
}

static inline struct dcomp_surface *surface_from_IDCompositionVirtualSurface(IDCompositionVirtualSurface *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_surface, IDCompositionVirtualSurface_iface);
}

static inline struct dcomp_transform *transform_from_IDCompositionTranslateTransform(IDCompositionTranslateTransform *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_transform, IDCompositionTranslateTransform_iface);
}

static inline struct dcomp_transform *transform_from_IDCompositionScaleTransform(IDCompositionScaleTransform *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_transform, IDCompositionScaleTransform_iface);
}

static inline struct dcomp_transform *transform_from_IDCompositionMatrixTransform(IDCompositionMatrixTransform *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_transform, IDCompositionMatrixTransform_iface);
}

static inline struct dcomp_transform *transform_from_IDCompositionSkewTransform(IDCompositionSkewTransform *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_transform, IDCompositionSkewTransform_iface);
}

static inline struct dcomp_transform *transform_from_IDCompositionRotateTransform(IDCompositionRotateTransform *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_transform, IDCompositionRotateTransform_iface);
}

/* Object constructors. */
HRESULT dcomp_target_create(struct dcomp_device *device, IDCompositionTarget **out);
HRESULT dcomp_visual_create(struct dcomp_device *device, BOOL want_v2, void **out);
HRESULT dcomp_surface_create(struct dcomp_device *device, UINT width, UINT height,
                             DXGI_FORMAT format, DXGI_ALPHA_MODE alpha_mode,
                             BOOL virtual_surface, void **out);
HRESULT dcomp_surface_factory_create(struct dcomp_device *device, IDCompositionSurfaceFactory **out);
HRESULT dcomp_animation_create(IDCompositionAnimation **out);
HRESULT dcomp_effect_group_create(IDCompositionEffectGroup **out);
HRESULT dcomp_rect_clip_create(IDCompositionRectangleClip **out);
HRESULT dcomp_transform_create(enum dcomp_transform_kind kind, void **out);

#endif /* __DCOMP_PRIVATE_H */
