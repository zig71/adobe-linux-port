/* probe_mime.c — MIME-type to COM object resolution.
 *
 * Why: the Creative Cloud bootstrapper fails its UI with
 *     urlmon:create_object Could not find object for MIME L"text/html"
 *     ieframe:bind_to_object BindToObject failed: 80040154 (REGDB_E_CLASSNOTREG)
 *
 * urlmon's get_mime_clsid() resolves a content type by reading
 *     HKEY_CLASSES_ROOT\MIME\Database\Content Type\<mime>  ->  CLSID
 * and then instantiating that CLSID. This probe performs the same two steps,
 * reporting each separately so the failure can be attributed to the registry
 * data or to object creation.
 *
 * Build 32-bit, matching the installer:
 *   i686-w64-mingw32-gcc -O2 -o probe_mime.exe probe_mime.c -lole32 -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static void wide_to_ascii(const WCHAR *w, char *out, int n) {
    int i;
    for (i = 0; i < n - 1 && w[i]; i++) out[i] = (w[i] < 128) ? (char)w[i] : '?';
    out[i] = 0;
}

static void check_mime(const char *mime) {
    WCHAR key[512], clsid_str[128];
    char label[64], ascii[256];
    HKEY hk;
    LONG r;
    DWORD type = 0, size = sizeof(clsid_str);
    HRESULT hr;
    CLSID clsid;
    int i;

    MultiByteToWideChar(CP_ACP, 0, mime, -1, key, 512);
    /* prefix with the database path, in place */
    {
        WCHAR full[512];
        static const WCHAR prefix[] = L"MIME\\Database\\Content Type\\";
        lstrcpyW(full, prefix);
        lstrcatW(full, key);
        lstrcpyW(key, full);
    }

    snprintf(label, sizeof(label), "MIME_%s", mime);
    for (i = 0; label[i]; i++) if (label[i] == '/') label[i] = '_';

    r = RegOpenKeyExW(HKEY_CLASSES_ROOT, key, 0, KEY_READ, &hk);
    printf("%s_open=%s (%ld)\n", label, r == ERROR_SUCCESS ? "found" : "missing", (long)r);
    if (r != ERROR_SUCCESS) {
        printf("%s_clsid=(no key)\n", label);
        return;
    }

    memset(clsid_str, 0, sizeof(clsid_str));
    r = RegQueryValueExW(hk, L"CLSID", NULL, &type, (BYTE *)clsid_str, &size);
    if (r == ERROR_SUCCESS) {
        wide_to_ascii(clsid_str, ascii, sizeof(ascii));
        printf("%s_clsid=%s\n", label, ascii);
        printf("%s_clsid_type=%lu\n", label, (unsigned long)type);
    } else {
        printf("%s_clsid=(missing) (%ld)\n", label, (long)r);
        RegCloseKey(hk);
        return;
    }
    RegCloseKey(hk);

    /* Step 2: can the CLSID actually be turned into an object? */
    memset(&clsid, 0, sizeof(clsid));
    hr = CLSIDFromString(clsid_str, &clsid);
    printf("%s_parse=0x%08lx\n", label, (unsigned long)hr);
    if (FAILED(hr)) return;

    {
        IUnknown *unk = NULL;
        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
        printf("%s_cocreate=0x%08lx\n", label, (unsigned long)hr);
        if (unk) unk->lpVtbl->Release(unk);
    }
}

int main(void) {
    HRESULT hr;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    printf("CoInitializeEx=0x%08lx\n", (unsigned long)hr);

    check_mime("text/html");
    check_mime("text/plain");
    check_mime("text/xml");
    check_mime("application/xhtml+xml");

    /* The class database entries those MIME handlers depend on. */
    {
        static const char *classes[] = {
            "{25336920-03F9-11CF-8FD0-00AA00686F13}",  /* HTML Document (mshtml) */
            "{25336921-03F9-11CF-8FD0-00AA00686F13}",  /* HTML Document, IE */
            "{0002DF01-0000-0000-C000-000000000046}",  /* InternetExplorer.Application */
            "{8856F961-340A-11D0-A96B-00C04FD705A2}",  /* WebBrowser (ieframe Shell.Explorer) */
        };
        int i;
        for (i = 0; i < 4; i++) {
            WCHAR path[256];
            HKEY hk;
            LONG r;
            MultiByteToWideChar(CP_ACP, 0, classes[i], -1, path, 256);
            {
                WCHAR full[320];
                lstrcpyW(full, L"CLSID\\");
                lstrcatW(full, path);
                lstrcpyW(path, full);
            }
            r = RegOpenKeyExW(HKEY_CLASSES_ROOT, path, 0, KEY_READ, &hk);
            printf("CLSIDKEY_%s=%s\n", classes[i], r == ERROR_SUCCESS ? "present" : "absent");
            if (r == ERROR_SUCCESS) RegCloseKey(hk);
        }
    }

    /* Is the MIME database populated at all? */
    {
        HKEY hk;
        LONG r = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"MIME\\Database\\Content Type", 0, KEY_READ, &hk);
        printf("MIME_DB_open=%s (%ld)\n", r == ERROR_SUCCESS ? "found" : "missing", (long)r);
        if (r == ERROR_SUCCESS) {
            DWORD sub = 0;
            if (RegQueryInfoKeyW(hk, NULL, NULL, NULL, &sub, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                printf("MIME_DB_subkeys=%lu\n", (unsigned long)sub);
            RegCloseKey(hk);
        }
        r = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"MIME\\Database", 0, KEY_READ, &hk);
        printf("MIME_DATABASE_open=%s (%ld)\n", r == ERROR_SUCCESS ? "found" : "missing", (long)r);
        if (r == ERROR_SUCCESS) RegCloseKey(hk);
    }

    CoUninitialize();
    printf("PROBE_RESULT=OK\n");
    return 0;
}
