/* probe_winhttp.c — WinHTTP / WinINet oracle probe.
 *
 * Why: Creative Cloud's installer and the apps' update/entitlement flow use
 * WinHTTP/WinINet. Session option handling, proxy detection and error mapping
 * differences change how an application reacts to network conditions.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_winhttp.exe probe_winhttp.c \
 *            -lwinhttp -lwininet -lole32
 */
#include <windows.h>
#include <winhttp.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

static void ph(const char *k, HRESULT hr) { printf("%s=0x%08lx\n", k, (unsigned long)hr); }
static void pe(const char *k, DWORD e) { printf("%s=%lu\n", k, (unsigned long)e); }

int main(void) {
    HINTERNET session, connect, request;
    DWORD size, value, err;

    /* --- session --- */
    session = WinHttpOpen(L"AdobeWineLab/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    printf("WinHttpOpen_nonnull=%d\n", session != NULL);
    if (!session) { pe("WinHttpOpen_err", GetLastError()); printf("PROBE_RESULT=NO_SESSION\n"); return 2; }

    {
        value = 0xdeadbeef; size = sizeof(value);
        if (WinHttpQueryOption(session, WINHTTP_OPTION_SECURE_PROTOCOLS, &value, &size)) {
            printf("SECURE_PROTOCOLS=0x%08lx\n", (unsigned long)value);
            printf("PROTOCOL_TLS12=%d\n", (value & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2) ? 1 : 0);
            printf("PROTOCOL_TLS13=%d\n", (value & WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3) ? 1 : 0);
        } else pe("SECURE_PROTOCOLS_err", GetLastError());

        value = 0xdeadbeef; size = sizeof(value);
        if (WinHttpQueryOption(session, WINHTTP_OPTION_CONNECT_TIMEOUT, &value, &size))
            printf("CONNECT_TIMEOUT=%lu\n", (unsigned long)value);
        else pe("CONNECT_TIMEOUT_err", GetLastError());

        value = 0xdeadbeef; size = sizeof(value);
        if (WinHttpQueryOption(session, WINHTTP_OPTION_DECOMPRESSION, &value, &size))
            printf("DECOMPRESSION=0x%08lx\n", (unsigned long)value);
        else pe("DECOMPRESSION_err", GetLastError());

        /* security flags for a request */
        value = 0xdeadbeef; size = sizeof(value);
        if (WinHttpQueryOption(session, WINHTTP_OPTION_SECURITY_FLAGS, &value, &size))
            printf("SECURITY_FLAGS=0x%08lx\n", (unsigned long)value);
        else pe("SECURITY_FLAGS_err", GetLastError());
    }

    /* --- option round-trips (set then get) --- */
    {
        DWORD v = WINHTTP_DISABLE_REDIRECTS;
        if (WinHttpSetOption(session, WINHTTP_OPTION_DISABLE_FEATURE, &v, sizeof(v)))
            printf("Set_DISABLE_REDIRECTS=1\n");
        else pe("Set_DISABLE_REDIRECTS", GetLastError());
        v = 1234;
        if (WinHttpSetOption(session, WINHTTP_OPTION_CONNECT_RETRIES, &v, sizeof(v)))
            printf("Set_CONNECT_RETRIES=1\n");
        else pe("Set_CONNECT_RETRIES", GetLastError());
        v = 0xdeadbeef; size = sizeof(v);
        if (WinHttpQueryOption(session, WINHTTP_OPTION_CONNECT_RETRIES, &v, &size))
            printf("Get_CONNECT_RETRIES=%lu\n", (unsigned long)v);
        else pe("Get_CONNECT_RETRIES_err", GetLastError());
    }

    /* --- error surface for a bogus host (no external network needed) --- */
    connect = WinHttpConnect(session, L"nonexistent.invalid.adobelab", 443, 0);
    printf("WinHttpConnect_nonnull=%d\n", connect != NULL);
    if (connect) {
        request = WinHttpOpenRequest(connect, L"GET", L"/", NULL, WINHTTP_NO_REFERER,
                                     WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        printf("WinHttpOpenRequest_nonnull=%d\n", request != NULL);
        if (request) {
            BOOL ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                         WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
            err = GetLastError();
            printf("WinHttpSendRequest_bogus=1 ok=%d err=%lu\n", ok ? 1 : 0, (unsigned long)err);
            WinHttpCloseHandle(request);
        }
        WinHttpCloseHandle(connect);
    } else pe("WinHttpConnect_err", GetLastError());

    /* --- URL helpers (pure, deterministic) --- */
    {
        URL_COMPONENTS uc;
        WCHAR host[128], path[256], scheme[32];
        memset(&uc, 0, sizeof(uc));
        memset(host, 0, sizeof(host)); memset(path, 0, sizeof(path)); memset(scheme, 0, sizeof(scheme));
        uc.dwStructSize = sizeof(uc);
        uc.lpszScheme = scheme; uc.dwSchemeLength = 32;
        uc.lpszHostName = host; uc.dwHostNameLength = 128;
        uc.lpszUrlPath = path;  uc.dwUrlPathLength = 256;
        if (WinHttpCrackUrl(L"https://cc-api-cp.adobe.io/v2/foo?x=1", 0, 0, &uc)) {
            printf("CrackUrl_scheme=%ls\n", scheme);
            printf("CrackUrl_host=%ls\n", host);
            printf("CrackUrl_path=%ls\n", path);
            printf("CrackUrl_port=%u\n", (unsigned)uc.nPort);
            printf("CrackUrl_secure=%u\n", (unsigned)uc.nScheme);
        } else pe("CrackUrl_err", GetLastError());
    }

    WinHttpCloseHandle(session);

    /* --- WinINet --- */
    printf("--- wininet ---\n");
    {
        DWORD flags = 0;
        BOOL ok = InternetGetConnectedState(&flags, 0);
        printf("InternetGetConnectedState=%d flags=0x%08lx\n", ok ? 1 : 0, (unsigned long)flags);
    }
    {
        HINTERNET h = InternetOpenA("AdobeWineLab/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        printf("InternetOpen_nonnull=%d\n", h != NULL);
        if (h) {
            DWORD v = 0, s = sizeof(v);
            if (InternetQueryOptionA(h, INTERNET_OPTION_CONNECTED_STATE, &v, &s))
                printf("InternetQueryOption_CONNECTED_STATE=0x%08lx\n", (unsigned long)v);
            else pe("InternetQueryOption_err", GetLastError());
            InternetCloseHandle(h);
        }
    }
    {
        DWORD size2 = 0;
        BOOL ok = InternetQueryOptionA(NULL, INTERNET_OPTION_PROXY, NULL, &size2);
        printf("InternetQueryOption_PROXY_null=%d err=%lu need=%lu\n", ok ? 1 : 0,
               (unsigned long)GetLastError(), (unsigned long)size2);
    }

    printf("PROBE_RESULT=OK\n");
    return 0;
}
