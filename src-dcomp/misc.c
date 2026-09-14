/*
 * DirectComposition animations, effect groups and clips
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

/* --- animation ---------------------------------------------------------
 *
 * The curve segments are accepted and retained only insofar as the object
 * exists with correct identity; no clock is driven, so a value is never
 * evaluated. Callers that rely on animated output will see the constant
 * default instead. */

static HRESULT STDMETHODCALLTYPE animation_QueryInterface(IDCompositionAnimation *iface, REFIID riid, void **out)
{
    struct dcomp_animation *animation = CONTAINING_RECORD(iface, struct dcomp_animation,
                                                          IDCompositionAnimation_iface);

    *out = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDCompositionAnimation))
        *out = &animation->IDCompositionAnimation_iface;
    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&animation->ref);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE animation_AddRef(IDCompositionAnimation *iface)
{
    struct dcomp_animation *animation = CONTAINING_RECORD(iface, struct dcomp_animation,
                                                          IDCompositionAnimation_iface);
    return InterlockedIncrement(&animation->ref);
}

static ULONG STDMETHODCALLTYPE animation_Release(IDCompositionAnimation *iface)
{
    struct dcomp_animation *animation = CONTAINING_RECORD(iface, struct dcomp_animation,
                                                          IDCompositionAnimation_iface);
    ULONG ref = InterlockedDecrement(&animation->ref);

    if (!ref) free(animation);
    return ref;
}

static HRESULT STDMETHODCALLTYPE animation_Reset(IDCompositionAnimation *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE animation_SetAbsoluteBeginTime(IDCompositionAnimation *iface, LARGE_INTEGER time)
{
    FIXME("begin time %lld: evaluated animation output is not implemented\n", (long long)time.QuadPart);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE animation_AddCubic(IDCompositionAnimation *iface, double offset,
        float constant, float linear, float quadratic, float cubic)
{
    TRACE("iface %p, offset %.8e.\n", iface, offset);
    FIXME("evaluated animation output is not implemented\n");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE animation_AddSinusoidal(IDCompositionAnimation *iface, double offset,
        float bias, float amplitude, float frequency, float phase)
{
    TRACE("iface %p, offset %.8e.\n", iface, offset);
    FIXME("evaluated animation output is not implemented\n");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE animation_AddRepeat(IDCompositionAnimation *iface, double offset, double duration)
{
    TRACE("iface %p, offset %.8e.\n", iface, offset);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE animation_End(IDCompositionAnimation *iface, double offset, float value)
{
    TRACE("iface %p, offset %.8e, value %.8e.\n", iface, offset, value);
    return S_OK;
}

static const IDCompositionAnimationVtbl dcomp_animation_vtbl =
{
    animation_QueryInterface,
    animation_AddRef,
    animation_Release,
    animation_Reset,
    animation_SetAbsoluteBeginTime,
    animation_AddCubic,
    animation_AddSinusoidal,
    animation_AddRepeat,
    animation_End,
};

HRESULT dcomp_animation_create(IDCompositionAnimation **out)
{
    struct dcomp_animation *animation;

    if (!(animation = calloc(1, sizeof(*animation)))) return E_OUTOFMEMORY;

    animation->IDCompositionAnimation_iface.lpVtbl = &dcomp_animation_vtbl;
    animation->ref = 1;

    *out = &animation->IDCompositionAnimation_iface;
    return S_OK;
}

/* --- effect group ------------------------------------------------------ */

static HRESULT STDMETHODCALLTYPE effect_group_QueryInterface(IDCompositionEffectGroup *iface, REFIID riid, void **out)
{
    struct dcomp_effect_group *group = CONTAINING_RECORD(iface, struct dcomp_effect_group,
                                                         IDCompositionEffectGroup_iface);

    *out = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDCompositionEffectGroup) ||
        IsEqualIID(riid, &IID_IDCompositionEffect))
        *out = &group->IDCompositionEffectGroup_iface;
    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&group->ref);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE effect_group_AddRef(IDCompositionEffectGroup *iface)
{
    struct dcomp_effect_group *group = CONTAINING_RECORD(iface, struct dcomp_effect_group,
                                                         IDCompositionEffectGroup_iface);
    return InterlockedIncrement(&group->ref);
}

static ULONG STDMETHODCALLTYPE effect_group_Release(IDCompositionEffectGroup *iface)
{
    struct dcomp_effect_group *group = CONTAINING_RECORD(iface, struct dcomp_effect_group,
                                                         IDCompositionEffectGroup_iface);
    ULONG ref = InterlockedDecrement(&group->ref);

    if (!ref)
    {
        if (group->transform_3d) group->transform_3d->lpVtbl->Release(group->transform_3d);
        free(group);
    }
    return ref;
}

static HRESULT STDMETHODCALLTYPE effect_group_SetOpacityAnimation(IDCompositionEffectGroup *iface,
                                                                  IDCompositionAnimation *animation)
{
    FIXME("animations are not supported yet\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE effect_group_SetOpacity(IDCompositionEffectGroup *iface, float opacity)
{
    struct dcomp_effect_group *group = CONTAINING_RECORD(iface, struct dcomp_effect_group,
                                                         IDCompositionEffectGroup_iface);
    if (opacity < 0.0f || opacity > 1.0f) return E_INVALIDARG;

    group->opacity = opacity;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE effect_group_SetTransform3D(IDCompositionEffectGroup *iface,
                                                             IDCompositionTransform3D *transform)
{
    struct dcomp_effect_group *group = CONTAINING_RECORD(iface, struct dcomp_effect_group,
                                                         IDCompositionEffectGroup_iface);

    if (transform) transform->lpVtbl->AddRef(transform);
    if (group->transform_3d) group->transform_3d->lpVtbl->Release(group->transform_3d);
    group->transform_3d = transform;
    return S_OK;
}

static const IDCompositionEffectGroupVtbl dcomp_effect_group_vtbl =
{
    effect_group_QueryInterface,
    effect_group_AddRef,
    effect_group_Release,
    effect_group_SetOpacityAnimation,
    effect_group_SetOpacity,
    effect_group_SetTransform3D,
};

HRESULT dcomp_effect_group_create(IDCompositionEffectGroup **out)
{
    struct dcomp_effect_group *group;

    if (!(group = calloc(1, sizeof(*group)))) return E_OUTOFMEMORY;

    group->IDCompositionEffectGroup_iface.lpVtbl = &dcomp_effect_group_vtbl;
    group->ref = 1;
    group->opacity = 1.0f;

    *out = &group->IDCompositionEffectGroup_iface;
    return S_OK;
}

/* --- rectangle clip ---------------------------------------------------- */

static struct dcomp_rect_clip *clip_from_iface(IDCompositionRectangleClip *iface)
{
    return CONTAINING_RECORD(iface, struct dcomp_rect_clip, IDCompositionRectangleClip_iface);
}

static HRESULT STDMETHODCALLTYPE clip_QueryInterface(IDCompositionRectangleClip *iface, REFIID riid, void **out)
{
    struct dcomp_rect_clip *clip = clip_from_iface(iface);

    *out = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDCompositionRectangleClip) ||
        IsEqualIID(riid, &IID_IDCompositionClip))
        *out = &clip->IDCompositionRectangleClip_iface;
    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&clip->ref);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE clip_AddRef(IDCompositionRectangleClip *iface)
{
    return InterlockedIncrement(&clip_from_iface(iface)->ref);
}

static ULONG STDMETHODCALLTYPE clip_Release(IDCompositionRectangleClip *iface)
{
    struct dcomp_rect_clip *clip = clip_from_iface(iface);
    ULONG ref = InterlockedDecrement(&clip->ref);

    if (!ref) free(clip);
    return ref;
}

#define CLIP_ANIM(name)                                                     \
static HRESULT STDMETHODCALLTYPE clip_##name(IDCompositionRectangleClip *iface, \
        IDCompositionAnimation *animation)                              \
{                                                                       \
    FIXME("animations are not supported yet\n");                        \
    return E_NOTIMPL;                                                   \
}
#define CLIP_VALUE(name, field)                                             \
static HRESULT STDMETHODCALLTYPE clip_##name(IDCompositionRectangleClip *iface, float value) \
{                                                                       \
    clip_from_iface(iface)->field = value;                              \
    return S_OK;                                                        \
}

CLIP_ANIM(SetLeftAnimation)     CLIP_VALUE(SetLeft, left)
CLIP_ANIM(SetTopAnimation)      CLIP_VALUE(SetTop, top)
CLIP_ANIM(SetRightAnimation)    CLIP_VALUE(SetRight, right)
CLIP_ANIM(SetBottomAnimation)   CLIP_VALUE(SetBottom, bottom)

CLIP_ANIM(SetTopLeftRadiusXAnimation)      CLIP_VALUE(SetTopLeftRadiusX, tlx)
CLIP_ANIM(SetTopLeftRadiusYAnimation)      CLIP_VALUE(SetTopLeftRadiusY, tly)
CLIP_ANIM(SetTopRightRadiusXAnimation)     CLIP_VALUE(SetTopRightRadiusX, trx)
CLIP_ANIM(SetTopRightRadiusYAnimation)     CLIP_VALUE(SetTopRightRadiusY, try_)
CLIP_ANIM(SetBottomLeftRadiusXAnimation)   CLIP_VALUE(SetBottomLeftRadiusX, blx)
CLIP_ANIM(SetBottomLeftRadiusYAnimation)   CLIP_VALUE(SetBottomLeftRadiusY, bly)
CLIP_ANIM(SetBottomRightRadiusXAnimation)  CLIP_VALUE(SetBottomRightRadiusX, brx)
CLIP_ANIM(SetBottomRightRadiusYAnimation)  CLIP_VALUE(SetBottomRightRadiusY, bry)

static const IDCompositionRectangleClipVtbl dcomp_rect_clip_vtbl =
{
    clip_QueryInterface,
    clip_AddRef,
    clip_Release,
    clip_SetLeftAnimation,  clip_SetLeft,
    clip_SetTopAnimation,   clip_SetTop,
    clip_SetRightAnimation, clip_SetRight,
    clip_SetBottomAnimation, clip_SetBottom,
    clip_SetTopLeftRadiusXAnimation,     clip_SetTopLeftRadiusX,
    clip_SetTopLeftRadiusYAnimation,     clip_SetTopLeftRadiusY,
    clip_SetTopRightRadiusXAnimation,    clip_SetTopRightRadiusX,
    clip_SetTopRightRadiusYAnimation,    clip_SetTopRightRadiusY,
    clip_SetBottomLeftRadiusXAnimation,  clip_SetBottomLeftRadiusX,
    clip_SetBottomLeftRadiusYAnimation,  clip_SetBottomLeftRadiusY,
    clip_SetBottomRightRadiusXAnimation, clip_SetBottomRightRadiusX,
    clip_SetBottomRightRadiusYAnimation, clip_SetBottomRightRadiusY,
};

HRESULT dcomp_rect_clip_create(IDCompositionRectangleClip **out)
{
    struct dcomp_rect_clip *clip;

    if (!(clip = calloc(1, sizeof(*clip)))) return E_OUTOFMEMORY;

    clip->IDCompositionRectangleClip_iface.lpVtbl = &dcomp_rect_clip_vtbl;
    clip->ref = 1;
    clip->left = clip->top = 0.0f;
    clip->right = clip->bottom = 0.0f;

    *out = &clip->IDCompositionRectangleClip_iface;
    return S_OK;
}
