/*
 * DirectComposition target and visual
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

#include "dcomp.h"

#include "dcomp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dcomp);

/* The v1 methods are generated further down but the target code above them
 * refers to them, so they are declared here. */
static ULONG STDMETHODCALLTYPE dcomp_visual1_AddRef(IDCompositionVisual *iface);
static ULONG STDMETHODCALLTYPE dcomp_visual1_Release(IDCompositionVisual *iface);

/* The v1 methods are generated further down but are used by the target code
 * above them, so their references are declared here. */
static ULONG STDMETHODCALLTYPE dcomp_visual1_AddRef(IDCompositionVisual *iface);
static ULONG STDMETHODCALLTYPE dcomp_visual1_Release(IDCompositionVisual *iface);

/* --- target ------------------------------------------------------------ */

static HRESULT STDMETHODCALLTYPE target_QueryInterface(IDCompositionTarget *iface, REFIID riid, void **out)
{
    struct dcomp_target *target = target_from_IDCompositionTarget(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    *out = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDCompositionTarget))
        *out = &target->IDCompositionTarget_iface;

    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&target->ref);
    return S_OK;
}

static ULONG STDMETHODCALLTYPE target_AddRef(IDCompositionTarget *iface)
{
    struct dcomp_target *target = target_from_IDCompositionTarget(iface);
    return InterlockedIncrement(&target->ref);
}

static ULONG STDMETHODCALLTYPE target_Release(IDCompositionTarget *iface)
{
    struct dcomp_target *target = target_from_IDCompositionTarget(iface);
    ULONG ref = InterlockedDecrement(&target->ref);

    if (!ref)
    {
        if (target->root) dcomp_visual1_Release(target->root);
        (&target->device->IDCompositionDevice3_iface)->lpVtbl->Release(&target->device->IDCompositionDevice3_iface);
        free(target);
    }
    return ref;
}

static HRESULT STDMETHODCALLTYPE target_SetRoot(IDCompositionTarget *iface, IDCompositionVisual *visual)
{
    struct dcomp_target *target = target_from_IDCompositionTarget(iface);

    TRACE("iface %p, visual %p.\n", iface, visual);

    if (visual) dcomp_visual1_AddRef(visual);
    if (target->root) dcomp_visual1_Release(target->root);
    target->root = visual;

    return S_OK;
}

static const IDCompositionTargetVtbl dcomp_target_vtbl =
{
    target_QueryInterface,
    target_AddRef,
    target_Release,
    target_SetRoot,
};

HRESULT dcomp_target_create(struct dcomp_device *device, IDCompositionTarget **out)
{
    struct dcomp_target *target;

    if (!(target = calloc(1, sizeof(*target)))) return E_OUTOFMEMORY;

    target->IDCompositionTarget_iface.lpVtbl = &dcomp_target_vtbl;
    target->ref = 1;
    target->device = device;
    (&device->IDCompositionDevice3_iface)->lpVtbl->AddRef(&device->IDCompositionDevice3_iface);

    TRACE("created target %p.\n", target);
    *out = &target->IDCompositionTarget_iface;
    return S_OK;
}

/* --- visual ------------------------------------------------------------ */

static HRESULT visual_query_interface(struct dcomp_visual *visual, REFIID riid, void **out)
{
    *out = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDCompositionVisual))
        *out = &visual->IDCompositionVisual_iface;
    else if (IsEqualIID(riid, &IID_IDCompositionVisual2))
        *out = &visual->IDCompositionVisual2_iface;

    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&visual->ref);
    return S_OK;
}

#define GEN_VISUAL(NAME, IF, CONV)                                                                     \
static HRESULT STDMETHODCALLTYPE NAME##_QueryInterface(IF *iface, REFIID riid, void **out)          \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);                       \
    return visual_query_interface(visual, riid, out);                                             \
}                                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_AddRef(IF *iface)                                             \
{                                                                                                 \
    return InterlockedIncrement(&CONV(iface)->ref);                                               \
}                                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_Release(IF *iface)                                            \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    ULONG ref = InterlockedDecrement(&visual->ref);                                               \
    if (!ref)                                                                                     \
    {                                                                                             \
        struct dcomp_visual_child *child, *next;                                                  \
        LIST_FOR_EACH_ENTRY_SAFE(child, next, &visual->children, struct dcomp_visual_child, entry) \
        {                                                                                         \
            list_remove(&child->entry);                                                           \
            dcomp_visual1_Release(child->visual);                                           \
            free(child);                                                                          \
        }                                                                                         \
        if (visual->content) visual->content->lpVtbl->Release(visual->content);                                   \
        if (visual->transform) visual->transform->lpVtbl->Release(visual->transform);                 \
        if (visual->transform_parent) dcomp_visual1_Release(visual->transform_parent);      \
        if (visual->effect) visual->effect->lpVtbl->Release(visual->effect);                          \
        if (visual->clip) visual->clip->lpVtbl->Release(visual->clip);                                \
        free(visual);                                                                             \
    }                                                                                             \
    return ref;                                                                                   \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetOffsetXAnimation(IF *iface, IDCompositionAnimation *animation) \
{                                                                                                 \
    FIXME("animations are not supported yet\n");                                                  \
    return E_NOTIMPL;                                                                             \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetOffsetX(IF *iface, float offset_x)                       \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, offset_x %.8e.\n", iface, offset_x);                                         \
    visual->offset_x = offset_x;                                                                  \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetOffsetYAnimation(IF *iface, IDCompositionAnimation *animation) \
{                                                                                                 \
    FIXME("animations are not supported yet\n");                                                  \
    return E_NOTIMPL;                                                                             \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetOffsetY(IF *iface, float offset_y)                       \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, offset_y %.8e.\n", iface, offset_y);                                         \
    visual->offset_y = offset_y;                                                                  \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetTransformObject(IF *iface, IDCompositionTransform *transform) \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, transform %p.\n", iface, transform);                                         \
    if (transform) transform->lpVtbl->AddRef(transform);                                      \
    if (visual->transform) visual->transform->lpVtbl->Release(visual->transform);                     \
    visual->transform = transform;                                                                \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetTransform(IF *iface, const D2D_MATRIX_3X2_F *matrix)     \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, matrix %p.\n", iface, matrix);                                               \
    if (!matrix) return E_INVALIDARG;                                                             \
    visual->matrix = *matrix;                                                                     \
    visual->has_matrix = TRUE;                                                                    \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetTransformParent(IF *iface, IDCompositionVisual *parent)  \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, parent %p.\n", iface, parent);                                               \
    if (parent) dcomp_visual1_AddRef(parent);                                               \
    if (visual->transform_parent) dcomp_visual1_Release(visual->transform_parent);          \
    visual->transform_parent = parent;                                                            \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetEffect(IF *iface, IDCompositionEffect *effect)           \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, effect %p.\n", iface, effect);                                               \
    if (effect) effect->lpVtbl->AddRef(effect);                                               \
    if (visual->effect) visual->effect->lpVtbl->Release(visual->effect);                              \
    visual->effect = effect;                                                                      \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetBitmapInterpolationMode(IF *iface,                       \
        enum DCOMPOSITION_BITMAP_INTERPOLATION_MODE mode)                                         \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, mode %u.\n", iface, mode);                                                   \
    visual->interpolation_mode = mode;                                                            \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetBorderMode(IF *iface, enum DCOMPOSITION_BORDER_MODE mode) \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, mode %u.\n", iface, mode);                                                   \
    visual->border_mode = mode;                                                                   \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetClipObject(IF *iface, IDCompositionClip *clip)           \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, clip %p.\n", iface, clip);                                                   \
    if (clip) clip->lpVtbl->AddRef(clip);                                                     \
    if (visual->clip) visual->clip->lpVtbl->Release(visual->clip);                                    \
    visual->clip = clip;                                                                          \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetClip(IF *iface, const D2D_RECT_F *rect)                  \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, rect %p.\n", iface, rect);                                                   \
    if (!rect) return E_INVALIDARG;                                                               \
    visual->clip_rect = *rect;                                                                    \
    visual->has_clip_rect = TRUE;                                                                 \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetContent(IF *iface, IUnknown *content)                    \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, content %p.\n", iface, content);                                             \
    if (content) content->lpVtbl->AddRef(content);                                                        \
    if (visual->content) visual->content->lpVtbl->Release(visual->content);                                       \
    visual->content = content;                                                                    \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_AddVisual(IF *iface, IDCompositionVisual *child, BOOL insert_above, \
        IDCompositionVisual *reference)                                                           \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    struct dcomp_visual_child *entry;                                                             \
    TRACE("iface %p, child %p, insert_above %d, reference %p.\n", iface, child, insert_above, reference); \
    if (!child) return E_INVALIDARG;                                                              \
    if (child == iface) return E_INVALIDARG;                                                      \
    if (!(entry = calloc(1, sizeof(*entry)))) return E_OUTOFMEMORY;                               \
    dcomp_visual1_AddRef(child);                                                            \
    entry->visual = child;                                                                        \
    list_add_tail(&visual->children, &entry->entry);                                              \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_RemoveVisual(IF *iface, IDCompositionVisual *child)         \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    struct dcomp_visual_child *entry;                                                             \
    TRACE("iface %p, child %p.\n", iface, child);                                                 \
    LIST_FOR_EACH_ENTRY(entry, &visual->children, struct dcomp_visual_child, entry)               \
    {                                                                                             \
        if (entry->visual == child)                                                               \
        {                                                                                         \
            list_remove(&entry->entry);                                                           \
            dcomp_visual1_Release(entry->visual);                                           \
            free(entry);                                                                          \
            return S_OK;                                                                          \
        }                                                                                         \
    }                                                                                             \
    return E_INVALIDARG;                                                                          \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_RemoveAllVisuals(IF *iface)                                 \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    struct dcomp_visual_child *entry, *next;                                                      \
    TRACE("iface %p.\n", iface);                                                                  \
    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &visual->children, struct dcomp_visual_child, entry)     \
    {                                                                                             \
        list_remove(&entry->entry);                                                               \
        dcomp_visual1_Release(entry->visual);                                               \
        free(entry);                                                                              \
    }                                                                                             \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetCompositeMode(IF *iface, enum DCOMPOSITION_COMPOSITE_MODE mode) \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, mode %u.\n", iface, mode);                                                   \
    visual->composite_mode = mode;                                                                \
    return S_OK;                                                                                  \
}

GEN_VISUAL(dcomp_visual1, IDCompositionVisual, visual_from_IDCompositionVisual)

#define GEN_VISUAL2_EXTRA(NAME, IF, CONV)                                                              \
static HRESULT STDMETHODCALLTYPE NAME##_SetOpacityMode(IF *iface, enum DCOMPOSITION_OPACITY_MODE mode) \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, mode %u.\n", iface, mode);                                                   \
    visual->opacity_mode = mode;                                                                  \
    return S_OK;                                                                                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetBackFaceVisibility(IF *iface,                            \
        enum DCOMPOSITION_BACKFACE_VISIBILITY visibility)                                         \
{                                                                                                 \
    struct dcomp_visual *visual = CONV(iface);                                                    \
    TRACE("iface %p, visibility %u.\n", iface, visibility);                                       \
    visual->backface_visibility = visibility;                                                     \
    return S_OK;                                                                                  \
}

/* The v2 vtable repeats every v1 method with the derived interface type, so
 * the v1 set is generated again for it. */
#define GEN_VISUAL_V2(NAME, IF, CONV)                                                                  \
static HRESULT STDMETHODCALLTYPE NAME##_QueryInterface(IF *iface, REFIID riid, void **out)          \
{                                                                                                 \
    return visual_query_interface(CONV(iface), riid, out);                                        \
}                                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_AddRef(IF *iface)                                             \
{                                                                                                 \
    return InterlockedIncrement(&CONV(iface)->ref);                                               \
}                                                                                                 \
static ULONG STDMETHODCALLTYPE NAME##_Release(IF *iface)                                            \
{                                                                                                 \
    return dcomp_visual1_Release(&CONV(iface)->IDCompositionVisual_iface);                  \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetOffsetXAnimation(IF *iface, IDCompositionAnimation *animation) \
{                                                                                                 \
    return dcomp_visual1_SetOffsetXAnimation(&CONV(iface)->IDCompositionVisual_iface, animation); \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetOffsetX(IF *iface, float v)                              \
{                                                                                                 \
    return dcomp_visual1_SetOffsetX(&CONV(iface)->IDCompositionVisual_iface, v);            \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetOffsetYAnimation(IF *iface, IDCompositionAnimation *animation) \
{                                                                                                 \
    return dcomp_visual1_SetOffsetYAnimation(&CONV(iface)->IDCompositionVisual_iface, animation); \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetOffsetY(IF *iface, float v)                              \
{                                                                                                 \
    return dcomp_visual1_SetOffsetY(&CONV(iface)->IDCompositionVisual_iface, v);            \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetTransformObject(IF *iface, IDCompositionTransform *transform) \
{                                                                                                 \
    return dcomp_visual1_SetTransformObject(&CONV(iface)->IDCompositionVisual_iface, transform); \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetTransform(IF *iface, const D2D_MATRIX_3X2_F *matrix)     \
{                                                                                                 \
    return dcomp_visual1_SetTransform(&CONV(iface)->IDCompositionVisual_iface, matrix);    \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetTransformParent(IF *iface, IDCompositionVisual *parent)  \
{                                                                                                 \
    return dcomp_visual1_SetTransformParent(&CONV(iface)->IDCompositionVisual_iface, parent); \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetEffect(IF *iface, IDCompositionEffect *effect)           \
{                                                                                                 \
    return dcomp_visual1_SetEffect(&CONV(iface)->IDCompositionVisual_iface, effect);        \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetBitmapInterpolationMode(IF *iface,                       \
        enum DCOMPOSITION_BITMAP_INTERPOLATION_MODE mode)                                         \
{                                                                                                 \
    return dcomp_visual1_SetBitmapInterpolationMode(&CONV(iface)->IDCompositionVisual_iface, mode); \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetBorderMode(IF *iface, enum DCOMPOSITION_BORDER_MODE mode) \
{                                                                                                 \
    return dcomp_visual1_SetBorderMode(&CONV(iface)->IDCompositionVisual_iface, mode);      \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetClipObject(IF *iface, IDCompositionClip *clip)           \
{                                                                                                 \
    return dcomp_visual1_SetClipObject(&CONV(iface)->IDCompositionVisual_iface, clip);      \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetClip(IF *iface, const D2D_RECT_F *rect)                  \
{                                                                                                 \
    return dcomp_visual1_SetClip(&CONV(iface)->IDCompositionVisual_iface, rect);           \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetContent(IF *iface, IUnknown *content)                    \
{                                                                                                 \
    return dcomp_visual1_SetContent(&CONV(iface)->IDCompositionVisual_iface, content);      \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_AddVisual(IF *iface, IDCompositionVisual *child, BOOL insert_above, \
        IDCompositionVisual *reference)                                                           \
{                                                                                                 \
    return dcomp_visual1_AddVisual(&CONV(iface)->IDCompositionVisual_iface, child, insert_above, reference); \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_RemoveVisual(IF *iface, IDCompositionVisual *child)         \
{                                                                                                 \
    return dcomp_visual1_RemoveVisual(&CONV(iface)->IDCompositionVisual_iface, child);      \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_RemoveAllVisuals(IF *iface)                                 \
{                                                                                                 \
    return dcomp_visual1_RemoveAllVisuals(&CONV(iface)->IDCompositionVisual_iface);         \
}                                                                                                 \
static HRESULT STDMETHODCALLTYPE NAME##_SetCompositeMode(IF *iface, enum DCOMPOSITION_COMPOSITE_MODE mode) \
{                                                                                                 \
    return dcomp_visual1_SetCompositeMode(&CONV(iface)->IDCompositionVisual_iface, mode);   \
}

GEN_VISUAL_V2(dcomp_visual2, IDCompositionVisual2, visual_from_IDCompositionVisual2)
GEN_VISUAL2_EXTRA(dcomp_visual2, IDCompositionVisual2, visual_from_IDCompositionVisual2)

static const IDCompositionVisualVtbl dcomp_visual_vtbl =
{
    dcomp_visual1_QueryInterface,
    dcomp_visual1_AddRef,
    dcomp_visual1_Release,
    dcomp_visual1_SetOffsetXAnimation,
    dcomp_visual1_SetOffsetX,
    dcomp_visual1_SetOffsetYAnimation,
    dcomp_visual1_SetOffsetY,
    dcomp_visual1_SetTransformObject,
    dcomp_visual1_SetTransform,
    dcomp_visual1_SetTransformParent,
    dcomp_visual1_SetEffect,
    dcomp_visual1_SetBitmapInterpolationMode,
    dcomp_visual1_SetBorderMode,
    dcomp_visual1_SetClipObject,
    dcomp_visual1_SetClip,
    dcomp_visual1_SetContent,
    dcomp_visual1_AddVisual,
    dcomp_visual1_RemoveVisual,
    dcomp_visual1_RemoveAllVisuals,
    dcomp_visual1_SetCompositeMode,
};

static const IDCompositionVisual2Vtbl dcomp_visual2_vtbl =
{
    dcomp_visual2_QueryInterface,
    dcomp_visual2_AddRef,
    dcomp_visual2_Release,
    dcomp_visual2_SetOffsetXAnimation,
    dcomp_visual2_SetOffsetX,
    dcomp_visual2_SetOffsetYAnimation,
    dcomp_visual2_SetOffsetY,
    dcomp_visual2_SetTransformObject,
    dcomp_visual2_SetTransform,
    dcomp_visual2_SetTransformParent,
    dcomp_visual2_SetEffect,
    dcomp_visual2_SetBitmapInterpolationMode,
    dcomp_visual2_SetBorderMode,
    dcomp_visual2_SetClipObject,
    dcomp_visual2_SetClip,
    dcomp_visual2_SetContent,
    dcomp_visual2_AddVisual,
    dcomp_visual2_RemoveVisual,
    dcomp_visual2_RemoveAllVisuals,
    dcomp_visual2_SetCompositeMode,
    dcomp_visual2_SetOpacityMode,
    dcomp_visual2_SetBackFaceVisibility,
};

HRESULT dcomp_visual_create(struct dcomp_device *device, BOOL want_v2, void **out)
{
    struct dcomp_visual *visual;

    if (!(visual = calloc(1, sizeof(*visual)))) return E_OUTOFMEMORY;

    visual->IDCompositionVisual_iface.lpVtbl = &dcomp_visual_vtbl;
    visual->IDCompositionVisual2_iface.lpVtbl = &dcomp_visual2_vtbl;
    visual->ref = 1;
    visual->device = device;
    list_init(&visual->children);

    *out = want_v2 ? (void *)&visual->IDCompositionVisual2_iface : (void *)&visual->IDCompositionVisual_iface;

    TRACE("created visual %p (v2 %d).\n", visual, want_v2);
    return S_OK;
}
