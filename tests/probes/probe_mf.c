/* probe_mf.c — Media Foundation oracle probe.
 *
 * Why: Premiere Pro, Media Encoder and Photoshop's video import all sit on
 * Media Foundation. Codec/transform enumeration is the substrate Adobe builds
 * its media pipeline on, so enumeration differences are directly causal.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_mf.exe probe_mf.c \
 *            -lmfplat -lmfuuid -lole32 -luuid
 */
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static void ph(const char *k, HRESULT hr) { printf("%s=0x%08lx\n", k, (unsigned long)hr); }

/* The SDK helpers for size/ratio attributes are inline wrappers around the
 * packed UINT64 representation; define them explicitly so the probe does not
 * depend on toolchain-provided inline versions. */
static HRESULT set_size(IMFAttributes *a, REFGUID g, UINT32 w, UINT32 h) {
    return a->lpVtbl->SetUINT64(a, g, ((UINT64)w << 32) | h);
}
static HRESULT get_size(IMFAttributes *a, REFGUID g, UINT32 *w, UINT32 *h) {
    UINT64 v = 0;
    HRESULT hr = a->lpVtbl->GetUINT64(a, g, &v);
    if (SUCCEEDED(hr)) { *w = (UINT32)(v >> 32); *h = (UINT32)v; }
    return hr;
}
static HRESULT set_ratio(IMFAttributes *a, REFGUID g, UINT32 n, UINT32 d) {
    return a->lpVtbl->SetUINT64(a, g, ((UINT64)n << 32) | d);
}
static HRESULT get_ratio(IMFAttributes *a, REFGUID g, UINT32 *n, UINT32 *d) {
    UINT64 v = 0;
    HRESULT hr = a->lpVtbl->GetUINT64(a, g, &v);
    if (SUCCEEDED(hr)) { *n = (UINT32)(v >> 32); *d = (UINT32)v; }
    return hr;
}

typedef HRESULT (WINAPI *MFTEnumEx_t)(GUID, UINT32, const MFT_REGISTER_TYPE_INFO *,
                                      const MFT_REGISTER_TYPE_INFO *, IMFActivate ***, UINT32 *);

int main(void) {
    HRESULT hr;
    HMODULE mfplat;

    mfplat = LoadLibraryA("mfplat.dll");
    if (!mfplat) mfplat = LoadLibraryA("mf.dll");
    printf("mfplat_load=%d\n", mfplat != NULL);
    if (!mfplat) { printf("PROBE_RESULT=NO_MFPLAT\n"); return 2; }

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ph("MFStartup", hr);
    if (FAILED(hr)) { printf("PROBE_RESULT=MFSTARTUP_FAILED\n"); return 3; }

    /* --- transform enumeration for the codecs Adobe cares about --- */
    {
        MFTEnumEx_t pMFTEnumEx = (MFTEnumEx_t)(void *)GetProcAddress(mfplat, "MFTEnumEx");
        printf("MFTEnumEx_present=%d\n", pMFTEnumEx != NULL);
        if (pMFTEnumEx) {
            struct {
                const char *name;
                GUID cat;
                MFT_REGISTER_TYPE_INFO in;
                MFT_REGISTER_TYPE_INFO out;
                int has_media;
            } cases[] = {
                { "VideoDecoder_H264", MFT_CATEGORY_VIDEO_DECODER,
                  { MFMediaType_Video, MFVideoFormat_H264 }, { MFMediaType_Video, MFVideoFormat_NV12 }, 1 },
                { "VideoEncoder_H264", MFT_CATEGORY_VIDEO_ENCODER,
                  { MFMediaType_Video, MFVideoFormat_NV12 }, { MFMediaType_Video, MFVideoFormat_H264 }, 1 },
                { "VideoDecoder_HEVC", MFT_CATEGORY_VIDEO_DECODER,
                  { MFMediaType_Video, MFVideoFormat_HEVC }, { MFMediaType_Video, MFVideoFormat_NV12 }, 1 },
                { "AudioDecoder_AAC", MFT_CATEGORY_AUDIO_DECODER,
                  { MFMediaType_Audio, MFAudioFormat_AAC }, { MFMediaType_Audio, MFAudioFormat_PCM }, 1 },
                { "AudioEncoder_AAC", MFT_CATEGORY_AUDIO_ENCODER,
                  { MFMediaType_Audio, MFAudioFormat_PCM }, { MFMediaType_Audio, MFAudioFormat_AAC }, 1 },
                { "VideoProcessor", MFT_CATEGORY_VIDEO_PROCESSOR, { {0}, 0 }, { {0}, 0 }, 0 },
                { "VideoEffect", MFT_CATEGORY_VIDEO_EFFECT, { {0}, 0 }, { {0}, 0 }, 0 },
                { "AudioEffect", MFT_CATEGORY_AUDIO_EFFECT, { {0}, 0 }, { {0}, 0 }, 0 },
                { "Muxer", MFT_CATEGORY_MULTIPLEXER, { {0}, 0 }, { {0}, 0 }, 0 },
                { "Demuxer", MFT_CATEGORY_DEMULTIPLEXER, { {0}, 0 }, { {0}, 0 }, 0 },
            };
            int i;
            for (i = 0; i < (int)(sizeof(cases) / sizeof(cases[0])); i++) {
                IMFActivate **acts = NULL;
                UINT32 n = 0;
                hr = pMFTEnumEx(cases[i].cat, MFT_ENUM_FLAG_ALL,
                                cases[i].has_media ? &cases[i].in : NULL,
                                cases[i].has_media ? &cases[i].out : NULL,
                                &acts, &n);
                printf("MFT_%s=0x%08lx count=%lu\n", cases[i].name,
                       (unsigned long)hr, (unsigned long)n);
                if (acts) {
                    UINT32 k;
                    for (k = 0; k < n; k++) if (acts[k]) acts[k]->lpVtbl->Release(acts[k]);
                    CoTaskMemFree(acts);
                }
            }
        }
    }

    /* --- media type attribute plumbing --- */
    {
        IMFMediaType *mt = NULL;
        hr = MFCreateMediaType(&mt);
        ph("MFCreateMediaType", hr);
        if (SUCCEEDED(hr) && mt) {
            UINT32 gw = 0, gh = 0, rn = 0, rd = 0;
            GUID g;
            mt->lpVtbl->SetGUID(mt, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
            mt->lpVtbl->SetGUID(mt, &MF_MT_SUBTYPE, &MFVideoFormat_NV12);
            set_size((IMFAttributes *)mt, &MF_MT_FRAME_SIZE, 1920, 1080);
            set_ratio((IMFAttributes *)mt, &MF_MT_FRAME_RATE, 30000, 1001);
            hr = get_size((IMFAttributes *)mt, &MF_MT_FRAME_SIZE, &gw, &gh);
            printf("MF_FRAME_SIZE=0x%08lx %lux%lu\n", (unsigned long)hr,
                   (unsigned long)gw, (unsigned long)gh);
            hr = get_ratio((IMFAttributes *)mt, &MF_MT_FRAME_RATE, &rn, &rd);
            printf("MF_FRAME_RATE=0x%08lx %lu/%lu\n", (unsigned long)hr,
                   (unsigned long)rn, (unsigned long)rd);
            memset(&g, 0, sizeof(g));
            hr = mt->lpVtbl->GetGUID(mt, &MF_MT_SUBTYPE, &g);
            printf("MF_MT_SUBTYPE=0x%08lx %08lx-%04x-%04x\n", (unsigned long)hr,
                   (unsigned long)g.Data1, g.Data2, g.Data3);
            /* duplicate type: documented to copy attributes */
            {
                IMFMediaType *mt2 = NULL;
                hr = MFCreateMediaType(&mt2);
                if (SUCCEEDED(hr) && mt2) {
                    hr = mt->lpVtbl->CopyAllItems(mt, (IMFAttributes *)mt2);
                    printf("MF_CopyAllItems=0x%08lx\n", (unsigned long)hr);
                    mt2->lpVtbl->Release(mt2);
                }
            }
            mt->lpVtbl->Release(mt);
        }
    }

    /* --- byte stream / buffer primitives --- */
    {
        IMFMediaBuffer *buf = NULL;
        hr = MFCreateMemoryBuffer(1024, &buf);
        ph("MFCreateMemoryBuffer", hr);
        if (SUCCEEDED(hr) && buf) {
            BYTE *data = NULL;
            DWORD maxlen = 0, curlen = 0;
            hr = buf->lpVtbl->Lock(buf, &data, &maxlen, &curlen);
            printf("MFBuffer_Lock=0x%08lx maxlen=%lu curlen=%lu\n", (unsigned long)hr,
                   (unsigned long)maxlen, (unsigned long)curlen);
            if (SUCCEEDED(hr)) buf->lpVtbl->Unlock(buf);
            buf->lpVtbl->Release(buf);
        }
    }

    /* --- DXGI device manager (GPU interop, the Premiere GPU path) --- */
    {
        IMFDXGIDeviceManager *mgr = NULL;
        UINT32 token = 0xdeadbeef;
        hr = MFCreateDXGIDeviceManager(&token, &mgr);
        printf("MFCreateDXGIDeviceManager=0x%08lx token=%lu\n",
               (unsigned long)hr, (unsigned long)token);
        if (mgr) {
            HRESULT hr2 = mgr->lpVtbl->ResetDevice(mgr, NULL, 0);
            printf("MFDXGI_ResetDevice_null=0x%08lx\n", (unsigned long)hr2);
            mgr->lpVtbl->Release(mgr);
        }
    }

    /* --- sample / clock primitives --- */
    {
        IMFSample *sample = NULL;
        hr = MFCreateSample(&sample);
        ph("MFCreateSample", hr);
        if (sample) {
            LONGLONG t = 0;
            sample->lpVtbl->SetSampleTime(sample, 123456789);
            sample->lpVtbl->GetSampleTime(sample, &t);
            printf("MFSample_time=%lld\n", (long long)t);
            sample->lpVtbl->SetSampleDuration(sample, 333667);
            sample->lpVtbl->GetSampleDuration(sample, &t);
            printf("MFSample_duration=%lld\n", (long long)t);
            sample->lpVtbl->Release(sample);
        }
    }
    {
        typedef HRESULT (WINAPI *MFCreatePresentationClock_t)(IMFPresentationClock **);
        MFCreatePresentationClock_t f = (MFCreatePresentationClock_t)(void *)
            GetProcAddress(mfplat, "MFCreatePresentationClock");
        IMFPresentationClock *clock = NULL;
        printf("MFCreatePresentationClock_present=%d\n", f != NULL);
        if (f) {
            hr = f(&clock);
            ph("MFCreatePresentationClock", hr);
            if (clock) clock->lpVtbl->Release(clock);
        }
    }

    MFShutdown();
    printf("PROBE_RESULT=OK\n");
    return 0;
}
