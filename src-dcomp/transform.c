/*
 * DirectComposition transforms
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

/* A transform object carries one vtable, chosen by kind at creation, so the
 * interface that was handed out is recovered with the matching accessor. */
static HRESULT transform_query_interface(struct dcomp_transform *transform, REFIID riid, void **out)
{
    *out = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDCompositionTransform) ||
        IsEqualIID(riid, &IID_IDCompositionTransform3D) ||
        IsEqualIID(riid, &IID_IDCompositionEffect))
    {
        /* These bases add no methods, so any of the concrete vtables serves. */
        *out = &transform->IDCompositionTranslateTransform_iface;
    }
    else
    {
        switch (transform->kind)
        {
        case DCOMP_TRANSFORM_TRANSLATE:
            if (IsEqualIID(riid, &IID_IDCompositionTranslateTransform))
                *out = &transform->IDCompositionTranslateTransform_iface;
            break;
        case DCOMP_TRANSFORM_SCALE:
            if (IsEqualIID(riid, &IID_IDCompositionScaleTransform))
                *out = &transform->IDCompositionScaleTransform_iface;
            break;
        case DCOMP_TRANSFORM_MATRIX:
            if (IsEqualIID(riid, &IID_IDCompositionMatrixTransform))
                *out = &transform->IDCompositionMatrixTransform_iface;
            break;
        case DCOMP_TRANSFORM_SKEW:
            if (IsEqualIID(riid, &IID_IDCompositionSkewTransform))
                *out = &transform->IDCompositionSkewTransform_iface;
            break;
        case DCOMP_TRANSFORM_ROTATE:
            if (IsEqualIID(riid, &IID_IDCompositionRotateTransform))
                *out = &transform->IDCompositionRotateTransform_iface;
            break;
        default:
            break;
        }
    }

    if (!*out) return E_NOINTERFACE;

    InterlockedIncrement(&transform->ref);
    return S_OK;
}

static HRESULT transform_release(struct dcomp_transform *transform)
{
    ULONG ref = InterlockedDecrement(&transform->ref);

    if (!ref)
    {
        UINT i;
        for (i = 0; i < transform->member_count; i++)
            transform->members[i]->lpVtbl->Release(transform->members[i]);
        free(transform->members);
        free(transform);
    }
    return ref;
}

#define GEN_TRANSFORM_COMMON(NAME, IF, CONV)                                             \
static HRESULT STDMETHODCALLTYPE NAME##_QueryInterface(IF *iface, REFIID riid, void **out) \
{                                                                                        \
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);              \
    return transform_query_interface(CONV(iface), riid, out);                            \
}                                                                                        \
static ULONG STDMETHODCALLTYPE NAME##_AddRef(IF *iface)                                  \
{                                                                                        \
    return InterlockedIncrement(&CONV(iface)->ref);                                      \
}                                                                                        \
static ULONG STDMETHODCALLTYPE NAME##_Release(IF *iface)                                 \
{                                                                                        \
    return transform_release(CONV(iface));                                               \
}

#define ANIM_SETTER(NAME, IF)                                                            \
static HRESULT STDMETHODCALLTYPE NAME(IF *iface, IDCompositionAnimation *animation)      \
{                                                                                        \
    FIXME("animations are not supported yet\n");                                         \
    return E_NOTIMPL;                                                                    \
}

/* --- translate --------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE dcomp_translate_SetOffsetX(IDCompositionTranslateTransform *iface, float offset)
{
    transform_from_IDCompositionTranslateTransform(iface)->off_x = offset;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_translate_SetOffsetY(IDCompositionTranslateTransform *iface, float offset)
{
    transform_from_IDCompositionTranslateTransform(iface)->off_y = offset;
    return S_OK;
}
GEN_TRANSFORM_COMMON(dcomp_translate, IDCompositionTranslateTransform, transform_from_IDCompositionTranslateTransform)
ANIM_SETTER(dcomp_translate_SetOffsetXAnimation, IDCompositionTranslateTransform)
ANIM_SETTER(dcomp_translate_SetOffsetYAnimation, IDCompositionTranslateTransform)

static const IDCompositionTranslateTransformVtbl translate_vtbl =
{
    dcomp_translate_QueryInterface,
    dcomp_translate_AddRef,
    dcomp_translate_Release,
    dcomp_translate_SetOffsetXAnimation,
    dcomp_translate_SetOffsetX,
    dcomp_translate_SetOffsetYAnimation,
    dcomp_translate_SetOffsetY,
};

/* --- scale ------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE dcomp_scale_SetScaleX(IDCompositionScaleTransform *iface, float scale)
{
    transform_from_IDCompositionScaleTransform(iface)->scale_x = scale;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_scale_SetScaleY(IDCompositionScaleTransform *iface, float scale)
{
    transform_from_IDCompositionScaleTransform(iface)->scale_y = scale;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_scale_SetCenterX(IDCompositionScaleTransform *iface, float center)
{
    transform_from_IDCompositionScaleTransform(iface)->center_x = center;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_scale_SetCenterY(IDCompositionScaleTransform *iface, float center)
{
    transform_from_IDCompositionScaleTransform(iface)->center_y = center;
    return S_OK;
}
GEN_TRANSFORM_COMMON(dcomp_scale, IDCompositionScaleTransform, transform_from_IDCompositionScaleTransform)
ANIM_SETTER(dcomp_scale_SetScaleXAnimation, IDCompositionScaleTransform)
ANIM_SETTER(dcomp_scale_SetScaleYAnimation, IDCompositionScaleTransform)
ANIM_SETTER(dcomp_scale_SetCenterXAnimation, IDCompositionScaleTransform)
ANIM_SETTER(dcomp_scale_SetCenterYAnimation, IDCompositionScaleTransform)

static const IDCompositionScaleTransformVtbl scale_vtbl =
{
    dcomp_scale_QueryInterface,
    dcomp_scale_AddRef,
    dcomp_scale_Release,
    dcomp_scale_SetScaleXAnimation,
    dcomp_scale_SetScaleX,
    dcomp_scale_SetScaleYAnimation,
    dcomp_scale_SetScaleY,
    dcomp_scale_SetCenterXAnimation,
    dcomp_scale_SetCenterX,
    dcomp_scale_SetCenterYAnimation,
    dcomp_scale_SetCenterY,
};

/* --- matrix ------------------------------------------------------------ */

static HRESULT STDMETHODCALLTYPE dcomp_matrix_SetMatrix(IDCompositionMatrixTransform *iface, const D2D_MATRIX_3X2_F *matrix)
{
    if (!matrix) return E_INVALIDARG;

    transform_from_IDCompositionMatrixTransform(iface)->matrix = *matrix;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_matrix_SetMatrixElement(IDCompositionMatrixTransform *iface,
                                                               int row, int column, float value)
{
    struct dcomp_transform *transform = transform_from_IDCompositionMatrixTransform(iface);

    if (row < 0 || row > 1 || column < 0 || column > 2) return E_INVALIDARG;

    (&transform->matrix.m11)[row * 3 + column] = value;
    return S_OK;
}
GEN_TRANSFORM_COMMON(dcomp_matrix, IDCompositionMatrixTransform, transform_from_IDCompositionMatrixTransform)
ANIM_SETTER(dcomp_matrix_SetMatrixElementAnimation, IDCompositionMatrixTransform)

static const IDCompositionMatrixTransformVtbl matrix_vtbl =
{
    dcomp_matrix_QueryInterface,
    dcomp_matrix_AddRef,
    dcomp_matrix_Release,
    dcomp_matrix_SetMatrix,
    dcomp_matrix_SetMatrixElementAnimation,
    dcomp_matrix_SetMatrixElement,
};

/* --- skew -------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE dcomp_skew_SetAngleX(IDCompositionSkewTransform *iface, float angle)
{
    transform_from_IDCompositionSkewTransform(iface)->skew_x = angle;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_skew_SetAngleY(IDCompositionSkewTransform *iface, float angle)
{
    transform_from_IDCompositionSkewTransform(iface)->skew_y = angle;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_skew_SetCenterX(IDCompositionSkewTransform *iface, float center)
{
    transform_from_IDCompositionSkewTransform(iface)->center_x = center;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_skew_SetCenterY(IDCompositionSkewTransform *iface, float center)
{
    transform_from_IDCompositionSkewTransform(iface)->center_y = center;
    return S_OK;
}
GEN_TRANSFORM_COMMON(dcomp_skew, IDCompositionSkewTransform, transform_from_IDCompositionSkewTransform)
ANIM_SETTER(dcomp_skew_SetAngleXAnimation, IDCompositionSkewTransform)
ANIM_SETTER(dcomp_skew_SetAngleYAnimation, IDCompositionSkewTransform)
ANIM_SETTER(dcomp_skew_SetCenterXAnimation, IDCompositionSkewTransform)
ANIM_SETTER(dcomp_skew_SetCenterYAnimation, IDCompositionSkewTransform)

static const IDCompositionSkewTransformVtbl skew_vtbl =
{
    dcomp_skew_QueryInterface,
    dcomp_skew_AddRef,
    dcomp_skew_Release,
    dcomp_skew_SetAngleXAnimation,
    dcomp_skew_SetAngleX,
    dcomp_skew_SetAngleYAnimation,
    dcomp_skew_SetAngleY,
    dcomp_skew_SetCenterXAnimation,
    dcomp_skew_SetCenterX,
    dcomp_skew_SetCenterYAnimation,
    dcomp_skew_SetCenterY,
};

/* --- rotate ------------------------------------------------------------ */

static HRESULT STDMETHODCALLTYPE dcomp_rotate_SetAngle(IDCompositionRotateTransform *iface, float angle)
{
    transform_from_IDCompositionRotateTransform(iface)->angle = angle;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_rotate_SetCenterX(IDCompositionRotateTransform *iface, float center)
{
    transform_from_IDCompositionRotateTransform(iface)->center_x = center;
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE dcomp_rotate_SetCenterY(IDCompositionRotateTransform *iface, float center)
{
    transform_from_IDCompositionRotateTransform(iface)->center_y = center;
    return S_OK;
}
GEN_TRANSFORM_COMMON(dcomp_rotate, IDCompositionRotateTransform, transform_from_IDCompositionRotateTransform)
ANIM_SETTER(dcomp_rotate_SetAngleAnimation, IDCompositionRotateTransform)
ANIM_SETTER(dcomp_rotate_SetCenterXAnimation, IDCompositionRotateTransform)
ANIM_SETTER(dcomp_rotate_SetCenterYAnimation, IDCompositionRotateTransform)

static const IDCompositionRotateTransformVtbl rotate_vtbl =
{
    dcomp_rotate_QueryInterface,
    dcomp_rotate_AddRef,
    dcomp_rotate_Release,
    dcomp_rotate_SetAngleAnimation,
    dcomp_rotate_SetAngle,
    dcomp_rotate_SetCenterXAnimation,
    dcomp_rotate_SetCenterX,
    dcomp_rotate_SetCenterYAnimation,
    dcomp_rotate_SetCenterY,
};

HRESULT dcomp_transform_create(enum dcomp_transform_kind kind, void **out)
{
    struct dcomp_transform *transform;

    if (!out) return E_INVALIDARG;

    if (!(transform = calloc(1, sizeof(*transform)))) return E_OUTOFMEMORY;

    transform->ref = 1;
    transform->kind = kind;
    transform->matrix.m11 = 1.0f;
    transform->matrix.m22 = 1.0f;
    transform->scale_x = 1.0f;
    transform->scale_y = 1.0f;
    transform->scale_z = 1.0f;

    switch (kind)
    {
    case DCOMP_TRANSFORM_TRANSLATE:
        transform->IDCompositionTranslateTransform_iface.lpVtbl = &translate_vtbl;
        *out = &transform->IDCompositionTranslateTransform_iface;
        break;
    case DCOMP_TRANSFORM_SCALE:
        transform->IDCompositionScaleTransform_iface.lpVtbl = &scale_vtbl;
        *out = &transform->IDCompositionScaleTransform_iface;
        break;
    case DCOMP_TRANSFORM_MATRIX:
        transform->IDCompositionMatrixTransform_iface.lpVtbl = &matrix_vtbl;
        *out = &transform->IDCompositionMatrixTransform_iface;
        break;
    case DCOMP_TRANSFORM_SKEW:
        transform->IDCompositionSkewTransform_iface.lpVtbl = &skew_vtbl;
        *out = &transform->IDCompositionSkewTransform_iface;
        break;
    case DCOMP_TRANSFORM_ROTATE:
        transform->IDCompositionRotateTransform_iface.lpVtbl = &rotate_vtbl;
        *out = &transform->IDCompositionRotateTransform_iface;
        break;
    default:
        free(transform);
        return E_NOTIMPL;
    }

    return S_OK;
}
