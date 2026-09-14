/* probe_rgv_slack.c — independent validation of the RegGetValue size rule.
 *
 * Hypothesis H1 (from probe_rgv_size.c):
 *     Windows *pcbData = (characters needed incl. terminating NUL) + 1,
 *                        + 2 when a REG_EXPAND_SZ value was expanded.
 *     Wine    *pcbData = (characters needed incl. terminating NUL).
 *
 * This probe varyies string length, emptiness and value type to try to refute
 * H1 rather than confirm it. All measurements use pvData = NULL, size = 0.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_rgv_slack.exe probe_rgv_slack.c -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HKEY hk;

static void add(const char *name, DWORD type, const char *data, DWORD bytes) {
    RegSetValueExA(hk, name, 0, type, (const BYTE *)data, bytes);
}

static void measure(const char *name, DWORD flags, const char *label) {
    DWORD sizeA = 0, sizeW = 0, typeA = 0, typeW = 0;
    WCHAR wname[128];
    LONG ra, rw;
    char buf[256];
    DWORD blen = sizeof(buf);

    MultiByteToWideChar(CP_ACP, 0, name, -1, wname, 128);
    ra = RegGetValueA(hk, NULL, name, flags, &typeA, NULL, &sizeA);
    rw = RegGetValueW(hk, NULL, wname, flags, &typeW, NULL, &sizeW);

    /* content length as actually returned with a real buffer */
    memset(buf, 0, sizeof(buf));
    blen = sizeof(buf);
    RegGetValueA(hk, NULL, name, flags, &typeA, buf, &blen);

    printf("%-26s A: ret=0x%08lx type=%lu size=%lu | W: ret=0x%08lx size=%lu | content_strlen=%lu\n",
           label, (unsigned long)ra, (unsigned long)typeA, (unsigned long)sizeA,
           (unsigned long)rw, (unsigned long)sizeW, (unsigned long)strlen(buf));
}

int main(void) {
    DWORD disp;
    static char ten[11], twenty[21], one[2];

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\RGVSlack", 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &disp) != ERROR_SUCCESS) {
        printf("setup failed\n");
        return 2;
    }

    memset(ten, 'a', 10); ten[10] = 0;
    memset(twenty, 'b', 20); twenty[20] = 0;
    one[0] = 'z'; one[1] = 0;

    add("SZ0", REG_SZ, "", 1);                       /* empty string, just NUL */
    add("SZ0_noNUL", REG_SZ, "", 0);                 /* zero-byte value */
    add("SZ1", REG_SZ, one, 2);
    add("SZ10", REG_SZ, ten, 11);
    add("SZ20", REG_SZ, twenty, 21);

    add("MS3", REG_MULTI_SZ, "a\0b\0\0", 6);

    add("EXP_short", REG_EXPAND_SZ, "%SystemRoot%", 13);
    add("EXP_long", REG_EXPAND_SZ, "%SystemRoot%\\%SystemRoot%", 25);
    add("EXP_nowin", REG_EXPAND_SZ, "%NOPE%", 7);    /* expands to itself */

    printf("--- REG_SZ, no expansion ---\n");
    measure("SZ0", RRF_RT_REG_SZ, "SZ0 len0(1 byte)");
    measure("SZ0_noNUL", RRF_RT_REG_SZ, "SZ0_nul len0(0 bytes)");
    measure("SZ1", RRF_RT_REG_SZ, "SZ1 len1");
    measure("SZ10", RRF_RT_REG_SZ, "SZ10 len10");
    measure("SZ20", RRF_RT_REG_SZ, "SZ20 len20");

    printf("--- REG_MULTI_SZ, no expansion ---\n");
    measure("MS3", RRF_RT_REG_MULTI_SZ, "MS3 a\\0b\\0 (6 bytes)");

    printf("--- REG_EXPAND_SZ expanded ---\n");
    measure("EXP_short", RRF_RT_REG_SZ, "EXP_short -> expand");
    measure("EXP_long", RRF_RT_REG_SZ, "EXP_long -> expand");
    measure("EXP_nowin", RRF_RT_REG_SZ, "EXP_nowin -> expand");

    printf("--- REG_EXPAND_SZ with RRF_NOEXPAND ---\n");
    measure("EXP_short", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, "EXP_short {6,6}");
    measure("EXP_long", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, "EXP_long {12,12}");
    measure("EXP_nowin", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, "EXP_nowin");

    RegCloseKey(hk);
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab");
    printf("PROBE_RESULT=OK\n");
    return 0;
}
