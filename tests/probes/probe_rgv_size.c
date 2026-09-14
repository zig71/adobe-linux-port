/* probe_rgv_size.c — RegGetValue size-reporting semantics.
 *
 * Companion to probe_reggetvalue.c. Isolates the pcbData value reported by
 * RegQueryValueEx vs RegGetValueA/W for string values, both with and without
 * an output buffer, so the byte-count rules can be stated exactly instead of
 * inferred.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_rgv_size.exe probe_rgv_size.c -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static const char *q = "abcdefghij";            /* 10 chars, incl. no NUL */
static const char *exp = "%SystemRoot%";        /* 12 chars */

static void raw(HKEY hk, const char *name, const char *what) {
    DWORD size = 0xdeadbeef, type = 0xdeadbeef;
    LONG r = RegQueryValueExA(hk, name, NULL, &type, NULL, &size);
    printf("RegQueryValueExA[%s] ret=0x%08lx type=%lu size=%lu\n",
           what, (unsigned long)r, (unsigned long)type, (unsigned long)size);
}

static void rgv(HKEY hk, const char *name, const char *what, DWORD flags) {
    DWORD size, type = 0xdeadbeef;
    LONG r;

    /* Callers that only want the required size pass pvData = NULL and a
     * zero-initialised length, which is the pattern that matters here. */
    size = 0;
    r = RegGetValueA(hk, NULL, name, flags, &type, NULL, &size);
    printf("RegGetValueA[%s] ret=0x%08lx type=%lu size_nobuf=%lu\n",
           what, (unsigned long)r, (unsigned long)type, (unsigned long)size);

    {
        char buf[64];
        DWORD s2 = sizeof(buf), t2 = 0xdeadbeef;
        memset(buf, 0, sizeof(buf));
        r = RegGetValueA(hk, NULL, name, flags, &t2, buf, &s2);
        printf("RegGetValueA[%s] ret=0x%08lx type=%lu size_buf=%lu strlen=%lu\n",
               what, (unsigned long)r, (unsigned long)t2, (unsigned long)s2,
               (unsigned long)strlen(buf));
    }
    {
        WCHAR wname[64];
        WCHAR wbuf[64];
        DWORD s3 = sizeof(wbuf), t3 = 0xdeadbeef;
        MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 64);
        memset(wbuf, 0, sizeof(wbuf));
        r = RegGetValueW(hk, NULL, wname, flags, &t3, wbuf, &s3);
        printf("RegGetValueW[%s] ret=0x%08lx type=%lu size_buf=%lu wcslen=%lu\n",
               what, (unsigned long)r, (unsigned long)t3, (unsigned long)s3,
               (unsigned long)wcslen(wbuf));
    }
    {
        /* size-only form of the W variant */
        WCHAR wname[64];
        DWORD s4 = 0, t4 = 0xdeadbeef;
        MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 64);
        r = RegGetValueW(hk, NULL, wname, flags, &t4, NULL, &s4);
        printf("RegGetValueW[%s] ret=0x%08lx type=%lu size_nobuf=%lu\n",
               what, (unsigned long)r, (unsigned long)t4, (unsigned long)s4);
    }
}

int main(void) {
    HKEY hk;
    DWORD disp;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\RGVSize", 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &disp) != ERROR_SUCCESS) {
        printf("setup failed\n");
        return 2;
    }

    /* exact byte counts, written explicitly (no implicit NUL from a literal) */
    RegSetValueExA(hk, "SZ10", 0, REG_SZ, (const BYTE *)q, 11);       /* 10 chars + NUL */
    RegSetValueExA(hk, "SZ10nonull", 0, REG_SZ, (const BYTE *)q, 10); /* 10 chars, no NUL */
    RegSetValueExA(hk, "EXP12", 0, REG_EXPAND_SZ, (const BYTE *)exp, 13);
    RegSetValueExA(hk, "EXP12nonull", 0, REG_EXPAND_SZ, (const BYTE *)exp, 12);

    raw(hk, "SZ10", "SZ10");
    raw(hk, "SZ10nonull", "SZ10nonull");
    raw(hk, "EXP12", "EXP12");
    raw(hk, "EXP12nonull", "EXP12nonull");

    rgv(hk, "SZ10", "SZ10/regsz", RRF_RT_REG_SZ);
    rgv(hk, "SZ10nonull", "SZ10nonull/regsz", RRF_RT_REG_SZ);
    rgv(hk, "EXP12", "EXP12/regsz", RRF_RT_REG_SZ);
    rgv(hk, "EXP12", "EXP12/noexpand", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND);
    rgv(hk, "EXP12nonull", "EXP12nonull/regsz", RRF_RT_REG_SZ);

    RegCloseKey(hk);
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab");
    printf("PROBE_RESULT=OK\n");
    return 0;
}
