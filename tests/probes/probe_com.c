/* probe_com.c — COM/OLE behavioral oracle probe.
 *
 * Why: Adobe applications are COM-heavy (app object model, Type Library
 * marshalling, CEF integration, plugin hosts). Divergences in apartment
 * behaviour, class registration or marshalling are load-bearing.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_com.exe probe_com.c \
 *            -lole32 -loleaut32 -luuid -ladvapi32
 */
#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <shobjidl.h>
#include <stdio.h>
#include <string.h>

static void ph(const char *k, HRESULT hr) { printf("%s=0x%08lx\n", k, (unsigned long)hr); }

static const char *hresult_name(HRESULT hr) {
    switch ((unsigned long)hr) {
    case 0x00000000: return "S_OK";
    case 0x80004001: return "E_NOTIMPL";
    case 0x80004002: return "E_NOINTERFACE";
    case 0x80004005: return "E_FAIL";
    case 0x80040154: return "REGDB_E_CLASSNOTREG";
    case 0x800401F0: return "CO_E_NOTINITIALIZED";
    case 0x80040155: return "REGDB_E_IIDNOTREG";
    case 0x80070005: return "E_ACCESSDENIED";
    case 0x80070057: return "E_INVALIDARG";
    case 0x80080005: return "CO_E_SERVER_EXEC_FAILURE";
    case 0x80010106: return "RPC_E_CHANGED_MODE";
    case 0x8001010D: return "RPC_E_CALL_REJECTED";
    case 0x80010108: return "RPC_E_DISCONNECTED";
    case 0x80029C4A: return "TYPE_E_CANTLOADLIBRARY";
    default: return "";
    }
}
static void phex(const char *k, HRESULT hr) {
    printf("%s=0x%08lx %s\n", k, (unsigned long)hr, hresult_name(hr));
}

/* --- minimal in-process COM object used to exercise marshalling --------- */
typedef struct TestObj {
    IUnknownVtbl *lpVtbl;
    LONG ref;
} TestObj;

static HRESULT WINAPI TO_QueryInterface(IUnknown *iface, REFIID riid, void **ppv) {
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)) {
        *ppv = iface;
        iface->lpVtbl->AddRef(iface);
        return S_OK;
    }
    return E_NOINTERFACE;
}
static ULONG WINAPI TO_AddRef(IUnknown *iface) {
    TestObj *o = (TestObj *)iface;
    return (ULONG)InterlockedIncrement(&o->ref);
}
static ULONG WINAPI TO_Release(IUnknown *iface) {
    TestObj *o = (TestObj *)iface;
    return (ULONG)InterlockedDecrement(&o->ref);
}
static IUnknownVtbl to_vtbl = { TO_QueryInterface, TO_AddRef, TO_Release };

int main(void) {
    HRESULT hr;

    /* --- initialisation / apartment model --- */
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    phex("CoInitializeEx_STA", hr);

    {
        APTTYPE at = APTTYPE_CURRENT;
        APTTYPEQUALIFIER q = APTTYPEQUALIFIER_NONE;
        hr = CoGetApartmentType(&at, &q);
        printf("CoGetApartmentType=0x%08lx aptype=%d qualifier=%d\n",
               (unsigned long)hr, (int)at, (int)q);
    }
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    phex("CoInitializeEx_MTA_on_STA_thread", hr);
    {
        ULONG_PTR cookie = 0;
        hr = CoGetContextToken(&cookie);
        printf("CoGetContextToken=0x%08lx nonnull=%d\n", (unsigned long)hr, cookie != 0);
    }

    /* --- standard object creation --- */
    {
        IUnknown *unk = NULL;
        hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
        phex("CoCreateInstance_ShellLink", hr);
        if (unk) unk->lpVtbl->Release(unk);
    }
    {
        static const CLSID clsid_stdpicture = { 0x7bf80980, 0xbf32, 0x101a, { 0x8b, 0xbb, 0x00, 0xaa, 0x00, 0x30, 0x0c, 0xab } };
        IUnknown *unk = NULL;
        hr = CoCreateInstance(&clsid_stdpicture, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
        phex("CoCreateInstance_StdPicture", hr);
        if (unk) unk->lpVtbl->Release(unk);
    }
    {
        /* CLSID_FreeThreadedMarshaler */
        static const CLSID clsid_ftm = { 0x0000033a, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
        IUnknown *unk = NULL;
        hr = CoCreateInstance(&clsid_ftm, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
        phex("CoCreateInstance_FreeThreadedMarshaler", hr);
        if (unk) unk->lpVtbl->Release(unk);
    }
    {
        /* CLSID_XMLHTTP (MSXML6) */
        static const CLSID clsid_xmlhttp = { 0x88d96a05, 0xf192, 0x11d4, { 0xa6, 0x5f, 0x00, 0x40, 0x96, 0x32, 0x51, 0xe5 } };
        IUnknown *unk = NULL;
        hr = CoCreateInstance(&clsid_xmlhttp, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
        phex("CoCreateInstance_XMLHTTP", hr);
        if (unk) unk->lpVtbl->Release(unk);
    }

    /* --- ProgID / CLSID mapping --- */
    {
        CLSID clsid;
        memset(&clsid, 0, sizeof(clsid));
        hr = CLSIDFromProgID(L"Shell.Application", &clsid);
        printf("CLSIDFromProgID_Shell.Application=0x%08lx {%08lx-%04x-%04x}\n",
               (unsigned long)hr, (unsigned long)clsid.Data1, clsid.Data2, clsid.Data3);
        hr = CLSIDFromProgID(L"Word.Application", &clsid);
        phex("CLSIDFromProgID_Word.Application", hr);
        hr = CLSIDFromProgID(L"ThisDoesNotExist.AdobeProbe", &clsid);
        phex("CLSIDFromProgID_missing", hr);
    }
    {
        static const CLSID clsid_shelllink = { 0x00021401, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
        LPOLESTR s = NULL;
        hr = StringFromCLSID(&clsid_shelllink, &s);
        if (SUCCEEDED(hr) && s) {
            printf("StringFromCLSID_ShellLink=%ls\n", s);
            CoTaskMemFree(s);
        } else phex("StringFromCLSID_ShellLink", hr);
    }

    /* --- marshalling of an in-process object --- */
    {
        TestObj obj;
        obj.lpVtbl = &to_vtbl;
        obj.ref = 1;
        {
            IStream *stm = NULL;
            hr = CoMarshalInterThreadInterfaceInStream(&IID_IUnknown, (IUnknown *)&obj, &stm);
            phex("CoMarshalInterThreadInterfaceInStream", hr);
            if (stm) {
                IUnknown *unm = NULL;
                hr = CoGetInterfaceAndReleaseStream(stm, &IID_IUnknown, (void **)&unm);
                phex("CoGetInterfaceAndReleaseStream", hr);
                if (unm) unm->lpVtbl->Release(unm);
            }
        }
        {
            IStream *stm = NULL;
            hr = CreateStreamOnHGlobal(NULL, TRUE, &stm);
            phex("CreateStreamOnHGlobal", hr);
            if (stm) {
                hr = CoMarshalInterface(stm, &IID_IUnknown, (IUnknown *)&obj, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
                phex("CoMarshalInterface_INPROC", hr);
                if (SUCCEEDED(hr)) {
                    STATSTG st;
                    memset(&st, 0, sizeof(st));
                    if (SUCCEEDED(stm->lpVtbl->Stat(stm, &st, STATFLAG_NONAME)))
                        printf("MarshalledSize_INPROC=%llu\n", (unsigned long long)st.cbSize.QuadPart);
                }
                stm->lpVtbl->Release(stm);
            }
        }
    }

    /* --- memory allocators --- */
    {
        IMalloc *m = NULL;
        hr = CoGetMalloc(1, &m);
        phex("CoGetMalloc", hr);
        if (m) {
            void *a = m->lpVtbl->Alloc(m, 16);
            printf("IMalloc_Alloc_nonnull=%d\n", a != NULL);
            m->lpVtbl->Free(m, a);
            m->lpVtbl->Release(m);
        }
    }

    /* --- HKCR view of the class database --- */
    {
        HKEY hk;
        LONG lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID", 0, KEY_READ, &hk);
        printf("RegOpen_HKCR_CLSID=%ld\n", (long)lr);
        if (lr == ERROR_SUCCESS) {
            DWORD sub = 0, maxsub = 0, maxval = 0;
            if (RegQueryInfoKeyA(hk, NULL, NULL, NULL, &sub, &maxsub, NULL, &maxval, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                printf("HKCR_CLSID_subkeys_ge_1=%d\n", sub >= 1);
                printf("HKCR_CLSID_maxsubkeylen_gt_0=%d\n", maxsub > 0);
                printf("HKCR_CLSID_valuename_maxlen_ge_0=%d\n", maxval >= 0);
            }
            RegCloseKey(hk);
        }
    }

    CoUninitialize();
    {
        APTTYPE at;
        APTTYPEQUALIFIER q;
        hr = CoGetApartmentType(&at, &q);
        printf("CoGetApartmentType_after_uninit=0x%08lx\n", (unsigned long)hr);
    }

    printf("PROBE_RESULT=OK\n");
    return 0;
}
