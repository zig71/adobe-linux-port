/* probe_directwrite.c — DirectWrite / font stack oracle probe.
 *
 * Why: every Adobe text engine and the CEF-based UI depend on DirectWrite
 * font enumeration, fallback and text-layout metrics. Divergences here change
 * glyph selection and layout output.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_directwrite.exe probe_directwrite.c -ldwrite -lole32
 */
#include <windows.h>
#include <dwrite.h>
#include <dwrite_2.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static void ph(const char *k, HRESULT hr) { printf("%s=0x%08lx\n", k, (unsigned long)hr); }

static void utf16_to_ascii(const WCHAR *w, char *out, int outlen) {
    int i;
    for (i = 0; i < outlen - 1 && w[i]; i++) out[i] = (w[i] < 128) ? (char)w[i] : '?';
    out[i] = 0;
}

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

/* IIDs are declared but not necessarily exported by every mingw-w64 runtime,
 * so define the ones used here explicitly. */
static const IID probe_IID_IDWriteFactory =
    { 0xb859ee5a, 0xd838, 0x4b5b, { 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48 } };
static const IID probe_IID_IDWriteFactory2 =
    { 0x0439fc60, 0xca44, 0x4998, { 0xbe, 0x3a, 0x9a, 0xdd, 0x42, 0xc9, 0x99, 0xd0 } };
static const IID probe_IID_IDWriteTextAnalysisSource =
    { 0x688e1a58, 0x5094, 0x47c8, { 0xad, 0xc8, 0xfb, 0xce, 0x87, 0x5c, 0x90, 0xb1 } };
#define IID_IDWriteFactory probe_IID_IDWriteFactory
#define IID_IDWriteFactory2 probe_IID_IDWriteFactory2
#define IID_IDWriteTextAnalysisSource probe_IID_IDWriteTextAnalysisSource

/* --- minimal IDWriteTextAnalysisSource, needed by MapCharacters --------- */
static const WCHAR fallback_text[] = { 0x6f22, 0x5b57 };  /* CJK, no Latin */

struct analysis_source {
    IDWriteTextAnalysisSource source;
    const WCHAR *text;
    UINT32 length;
};

static HRESULT WINAPI as_GetTextAtPosition(IDWriteTextAnalysisSource *iface, UINT32 pos,
                                           const WCHAR **text, UINT32 *len) {
    struct analysis_source *s = (struct analysis_source *)iface;
    if (pos >= s->length) { *text = NULL; *len = 0; }
    else { *text = s->text + pos; *len = s->length - pos; }
    return S_OK;
}
static HRESULT WINAPI as_GetTextBeforePosition(IDWriteTextAnalysisSource *iface, UINT32 pos,
                                               const WCHAR **text, UINT32 *len) {
    struct analysis_source *s = (struct analysis_source *)iface;
    if (pos == 0 || pos > s->length) { *text = NULL; *len = 0; }
    else { *text = s->text; *len = pos; }
    return S_OK;
}
static DWRITE_READING_DIRECTION WINAPI as_GetParagraphReadingDirection(
        IDWriteTextAnalysisSource *iface) {
    (void)iface;
    return DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
}
static HRESULT WINAPI as_GetLocaleName(IDWriteTextAnalysisSource *iface, UINT32 pos,
                                       UINT32 *len, const WCHAR **locale) {
    struct analysis_source *s = (struct analysis_source *)iface;
    if (pos >= s->length) { *len = 0; *locale = NULL; }
    else { *len = s->length - pos; *locale = L"en-us"; }
    return S_OK;
}
static HRESULT WINAPI as_GetNumberSubstitution(IDWriteTextAnalysisSource *iface, UINT32 pos,
                                               UINT32 *len, IDWriteNumberSubstitution **sub) {
    struct analysis_source *s = (struct analysis_source *)iface;
    if (pos >= s->length) { *len = 0; }
    else { *len = s->length - pos; }
    *sub = NULL;
    return S_OK;
}
static HRESULT WINAPI as_QueryInterface(IDWriteTextAnalysisSource *iface, REFIID riid, void **ppv) {
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDWriteTextAnalysisSource)) {
        *ppv = iface;
        return S_OK;
    }
    return E_NOINTERFACE;
}
static ULONG WINAPI as_AddRef(IDWriteTextAnalysisSource *iface) { (void)iface; return 2; }
static ULONG WINAPI as_Release(IDWriteTextAnalysisSource *iface) { (void)iface; return 1; }

static IDWriteTextAnalysisSourceVtbl as_vtbl = {
    as_QueryInterface,
    as_AddRef,
    as_Release,
    as_GetTextAtPosition,
    as_GetTextBeforePosition,
    as_GetParagraphReadingDirection,
    as_GetLocaleName,
    as_GetNumberSubstitution
};

static void init_analysis_source(struct analysis_source *s, const WCHAR *text, UINT32 length) {
    s->source.lpVtbl = &as_vtbl;
    s->text = text;
    s->length = length;
}

int main(void) {
    IDWriteFactory *dw = NULL;
    IDWriteFontCollection *coll = NULL;
    HRESULT hr;

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory, (IUnknown **)&dw);
    ph("DWriteCreateFactory", hr);
    if (FAILED(hr) || !dw) { printf("PROBE_RESULT=NO_FACTORY\n"); return 2; }

    hr = dw->lpVtbl->GetSystemFontCollection(dw, &coll, FALSE);
    ph("GetSystemFontCollection", hr);
    if (SUCCEEDED(hr) && coll) {
        UINT32 n = coll->lpVtbl->GetFontFamilyCount(coll);
        printf("FONT_FAMILY_COUNT=%lu\n", (unsigned long)n);

        /* deterministic probes for fonts an Adobe app would ask for */
        static const WCHAR *want[] = { L"Arial", L"Segoe UI", L"Tahoma", L"Times New Roman",
                                       L"Courier New", L"Microsoft Sans Serif", L"Calibri",
                                       L"Wingdings", L"Symbol" };
        int i;
        for (i = 0; i < (int)(sizeof(want) / sizeof(want[0])); i++) {
            UINT32 idx = 0;
            BOOL found = FALSE;
            char nm[64];
            utf16_to_ascii(want[i], nm, sizeof(nm));
            if (SUCCEEDED(coll->lpVtbl->FindFamilyName(coll, want[i], &idx, &found)))
                printf("FONT_PRESENT_%s=%d\n", nm, found ? 1 : 0);
            else
                printf("FONT_PRESENT_%s=err\n", nm);
        }

        /* first family name, deterministically (index 0) */
        if (n > 0) {
            IDWriteFontFamily *fam = NULL;
            if (SUCCEEDED(coll->lpVtbl->GetFontFamily(coll, 0, &fam)) && fam) {
                IDWriteLocalizedStrings *names = NULL;
                if (SUCCEEDED(fam->lpVtbl->GetFamilyNames(fam, &names)) && names) {
                    UINT32 cnt = names->lpVtbl->GetCount(names);
                    printf("FAMILY0_NAME_COUNT=%lu\n", (unsigned long)cnt);
                    if (cnt) {
                        WCHAR w[128];
                        UINT32 len = 0;
                        if (SUCCEEDED(names->lpVtbl->GetString(names, 0, w, 128))) {
                            names->lpVtbl->GetStringLength(names, 0, &len);
                            char nm[128];
                            utf16_to_ascii(w, nm, sizeof(nm));
                            printf("FAMILY0_STRING0=%s\n", nm);
                            printf("FAMILY0_STRING0_LEN=%lu\n", (unsigned long)len);
                        }
                    }
                    names->lpVtbl->Release(names);
                }
                fam->lpVtbl->Release(fam);
            }
        }
    }

    /* --- text layout metrics for a fixed string --- */
    {
        static const WCHAR txt[] = L"Adobe Wine probe 1234";
        IDWriteTextFormat *fmt = NULL;
        hr = dw->lpVtbl->CreateTextFormat(dw, L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_NORMAL,
                                          DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                          12.0f, L"en-us", &fmt);
        ph("CreateTextFormat_SegoeUI", hr);
        if (FAILED(hr) || !fmt) {
            hr = dw->lpVtbl->CreateTextFormat(dw, L"Arial", NULL, DWRITE_FONT_WEIGHT_NORMAL,
                                              DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                              12.0f, L"en-us", &fmt);
            ph("CreateTextFormat_Arial", hr);
        }
        if (SUCCEEDED(hr) && fmt) {
            IDWriteTextLayout *lay = NULL;
            hr = dw->lpVtbl->CreateTextLayout(dw, txt, (UINT32)(wcslen(txt)), fmt, 1000.0f, 1000.0f, &lay);
            ph("CreateTextLayout", hr);
            if (SUCCEEDED(hr) && lay) {
                DWRITE_TEXT_METRICS tm;
                DWRITE_LINE_METRICS lm;
                UINT32 lc = 0;
                memset(&tm, 0, sizeof(tm));
                memset(&lm, 0, sizeof(lm));
                if (SUCCEEDED(lay->lpVtbl->GetMetrics(lay, &tm))) {
                    printf("LAYOUT_WIDTH=%.3f\n", tm.width);
                    printf("LAYOUT_HEIGHT=%.3f\n", tm.height);
                    printf("LAYOUT_LINE_COUNT=%lu\n", (unsigned long)tm.lineCount);
                }
                if (SUCCEEDED(lay->lpVtbl->GetLineMetrics(lay, &lm, 1, &lc))) {
                    printf("LAYOUT_FIRSTLINE_LENGTH=%lu\n", (unsigned long)lm.length);
                    printf("LAYOUT_FIRSTLINE_HEIGHT=%.3f\n", lm.height);
                    printf("LAYOUT_FIRSTLINE_BASELINE=%.3f\n", lm.baseline);
                }
                lay->lpVtbl->Release(lay);
            }
            fmt->lpVtbl->Release(fmt);
        }
    }

    /* --- font fallback for a CJK codepoint (IDWriteFactory2) --- */
    {
        IDWriteFactory2 *dw2 = NULL;
        hr = dw->lpVtbl->QueryInterface(dw, &IID_IDWriteFactory2, (void **)&dw2);
        ph("QueryInterface_IDWriteFactory2", hr);
        if (SUCCEEDED(hr) && dw2) {
            IDWriteFontFallback *fb = NULL;
            hr = dw2->lpVtbl->GetSystemFontFallback(dw2, &fb);
            ph("GetSystemFontFallback", hr);
            if (SUCCEEDED(hr) && fb) {
                struct analysis_source src;
                UINT32 mapped = 0;
                IDWriteFont *font = NULL;
                FLOAT scale = 0.0f;
                init_analysis_source(&src, fallback_text, ARRAY_LEN(fallback_text));
                hr = fb->lpVtbl->MapCharacters(fb, &src.source, 0, src.length, NULL, NULL,
                                               DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                               DWRITE_FONT_STRETCH_NORMAL, &mapped, &font, &scale);
                printf("MapCharacters_CJK=0x%08lx mapped=%lu font_nonnull=%d\n",
                       (unsigned long)hr, (unsigned long)mapped, font != NULL);
                if (font) {
                    IDWriteFontFamily *fam = NULL;
                    if (SUCCEEDED(font->lpVtbl->GetFontFamily(font, &fam)) && fam) {
                        IDWriteLocalizedStrings *names = NULL;
                        if (SUCCEEDED(fam->lpVtbl->GetFamilyNames(fam, &names)) && names &&
                            names->lpVtbl->GetCount(names)) {
                            WCHAR w[128];
                            if (SUCCEEDED(names->lpVtbl->GetString(names, 0, w, 128))) {
                                char nm[128];
                                utf16_to_ascii(w, nm, sizeof(nm));
                                printf("MapCharacters_CJK_FAMILY=%s\n", nm);
                            }
                            names->lpVtbl->Release(names);
                        }
                        fam->lpVtbl->Release(fam);
                    }
                    font->lpVtbl->Release(font);
                }
                fb->lpVtbl->Release(fb);
            }
            dw2->lpVtbl->Release(dw2);
        }
    }

    (void)coll;
    dw->lpVtbl->Release(dw);
    printf("PROBE_RESULT=OK\n");
    return 0;
}
