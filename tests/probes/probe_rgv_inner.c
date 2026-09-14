/* probe_rgv_inner.c — observe the intermediate calls RegGetValue builds on.
 *
 * Purpose: explain why the pvData == NULL path of RegGetValueA reports a larger
 * size than expected. Prints what RegQueryValueExA reports for the same value,
 * including the type it hands back and the bytes it produces, then what
 * RegGetValueA reports for the same value with and without a buffer.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_rgv_inner.exe probe_rgv_inner.c -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    HKEY hk;
    DWORD disp;
    static const char raw[] = "%LABX%";     /* 6 chars + NUL = 7 bytes */
    DWORD size, type;
    LONG r;
    char buf[128];
    DWORD i;

    SetEnvironmentVariableA("LABX", "abcdef");   /* 6 chars, same length as raw */

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\RGVInner", 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &disp) != ERROR_SUCCESS) {
        printf("setup failed\n");
        return 2;
    }
    RegSetValueExA(hk, "V", 0, REG_EXPAND_SZ, (const BYTE *)raw, 7);

    /* size-only query */
    size = 0; type = 0xdeadbeef;
    r = RegQueryValueExA(hk, "V", NULL, &type, NULL, &size);
    printf("QVE_nobuf ret=%ld type=%lu size=%lu\n", (long)r, (unsigned long)type, (unsigned long)size);

    /* with buffer: print the exact bytes returned */
    size = sizeof(buf); type = 0xdeadbeef;
    memset(buf, 0xcc, sizeof(buf));
    r = RegQueryValueExA(hk, "V", NULL, &type, (BYTE *)buf, &size);
    printf("QVE_buf ret=%ld type=%lu size=%lu bytes=", (long)r, (unsigned long)type, (unsigned long)size);
    for (i = 0; i < size && i < 16; i++) printf("%02x", (unsigned char)buf[i]);
    printf("\n");

    /* expanded content, for reference */
    {
        char ebuf[128];
        DWORD n = ExpandEnvironmentStringsA(buf, ebuf, sizeof(ebuf));
        printf("Expand_ret=%lu strlen=%lu text=%s\n", (unsigned long)n,
               (unsigned long)strlen(ebuf), ebuf);
    }

    size = 0; type = 0xdeadbeef;
    r = RegGetValueA(hk, NULL, "V", RRF_RT_REG_SZ, &type, NULL, &size);
    printf("RGV_nobuf ret=%ld type=%lu size=%lu\n", (long)r, (unsigned long)type, (unsigned long)size);

    size = sizeof(buf); type = 0xdeadbeef;
    memset(buf, 0, sizeof(buf));
    r = RegGetValueA(hk, NULL, "V", RRF_RT_REG_SZ, &type, buf, &size);
    printf("RGV_buf ret=%ld type=%lu size=%lu strlen=%lu\n", (long)r, (unsigned long)type,
           (unsigned long)size, (unsigned long)strlen(buf));

    RegCloseKey(hk);
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab");
    printf("PROBE_RESULT=OK\n");
    return 0;
}
