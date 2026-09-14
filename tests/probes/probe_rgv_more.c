/* probe_rgv_more.c — the ERROR_MORE_DATA boundary for RegGetValueA.
 *
 * The reported *pcbData drives whether a caller gets ERROR_MORE_DATA, so the
 * size rule cannot be changed without pinning this boundary. For each value the
 * probe sweeps the supplied buffer size from smaller than the content up to
 * larger, recording the return code and the reported size.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_rgv_more.exe probe_rgv_more.c -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HKEY hk;

static void sweep(const char *name, const char *label, DWORD flags, int from, int to) {
    int cap;
    for (cap = from; cap <= to; cap++) {
        char buf[256];
        DWORD blen = (DWORD)cap, type = 0xdeadbeef;
        LONG r;
        memset(buf, 0xcc, sizeof(buf));
        r = RegGetValueA(hk, NULL, name, flags, &type, buf, &blen);
        /* Unique key per case for the comparison harness. */
        printf("case %s cap=%d ret=0x%08lx out=%lu nul=%d\n",
               label, cap, (unsigned long)r, (unsigned long)blen,
               (cap > 0 && r == ERROR_SUCCESS && buf[blen - 1] == 0) ? 1 : 0);
    }
}

int main(void) {
    DWORD disp;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\RGVMore", 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &disp) != ERROR_SUCCESS) {
        printf("setup failed\n");
        return 2;
    }

    /* REG_SZ of 10 chars: stored 11 bytes incl. NUL  (S = 11) */
    RegSetValueExA(hk, "SZ10", 0, REG_SZ, (const BYTE *)"abcdefghij", 11);

    /* REG_EXPAND_SZ: raw 12 chars + NUL (S = 13) expanding to 10 chars */
    SetEnvironmentVariableA("LABX", "0123456789");
    RegSetValueExA(hk, "EXPshorter", 0, REG_EXPAND_SZ, (const BYTE *)"%LABX%AAAAAA", 13);

    /* REG_EXPAND_SZ expanding to a longer string than the raw form (S = 7) */
    SetEnvironmentVariableA("LABY", "0123456789abcdef");
    RegSetValueExA(hk, "EXPlonger", 0, REG_EXPAND_SZ, (const BYTE *)"%LABY%", 7);

    printf("=== REG_SZ SZ10 (content 10 + NUL = 11) ===\n");
    sweep("SZ10", "sz10", RRF_RT_REG_SZ, 8, 14);

    printf("=== REG_EXPAND_SZ expands 7..9 chars (raw 6 + NUL = 7) ===\n");
    sweep("EXPshorter", "expshort", RRF_RT_REG_SZ, 6, 14);

    printf("=== REG_EXPAND_SZ expands to 16 chars (raw 6 + NUL = 7) ===\n");
    sweep("EXPlonger", "explong", RRF_RT_REG_SZ, 6, 20);

    RegCloseKey(hk);
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab");
    printf("PROBE_RESULT=OK\n");
    return 0;
}
