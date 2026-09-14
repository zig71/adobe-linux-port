/* probe_rgv_slack2.c — decisive characterization of the RegGetValue size rule.
 *
 * Goal: distinguish, for every case, what Windows reports
 *   (a) when a real output buffer is supplied, and
 *   (b) when pvData is NULL (size-only query),
 * for values whose expanded form is longer, shorter, and equal to the raw form.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_rgv_slack2.exe probe_rgv_slack2.c -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HKEY hk;

static void row(const char *name, DWORD flags, const char *label) {
    char buf[1024];
    DWORD blen, size, type;
    LONG r;
    LONG rn;
    DWORD sn = 0, tn = 0;

    memset(buf, 0, sizeof(buf));
    blen = sizeof(buf);
    r = RegGetValueA(hk, NULL, name, flags, &type, buf, &blen);
    rn = RegGetValueA(hk, NULL, name, flags, &tn, NULL, &sn);

    printf("%-24s withbuf: ret=%ld type=%lu size=%lu strlen=%lu | nobuf: ret=%ld size=%lu\n",
           label, (long)r, (unsigned long)type, (unsigned long)blen,
           (unsigned long)strlen(buf), (long)rn, (unsigned long)sn);
}

int main(void) {
    DWORD disp;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\RGVSlack2", 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &disp) != ERROR_SUCCESS) {
        printf("setup failed\n");
        return 2;
    }

    /* reg_sz, lengths 0..3 and 10 */
    RegSetValueExA(hk, "SZ0", 0, REG_SZ, (const BYTE *)"", 1);
    RegSetValueExA(hk, "SZ0raw", 0, REG_SZ, (const BYTE *)"", 0);          /* zero bytes, no NUL */
    RegSetValueExA(hk, "SZ1", 0, REG_SZ, (const BYTE *)"a", 2);
    RegSetValueExA(hk, "SZ10", 0, REG_SZ, (const BYTE *)"abcdefghij", 11);
    RegSetValueExA(hk, "SZ10nonul", 0, REG_SZ, (const BYTE *)"abcdefghij", 10); /* no NUL to append to */

    /* multi_sz, one string and two strings */
    RegSetValueExA(hk, "MS1", 0, REG_MULTI_SZ, (const BYTE *)"a\0\0", 4);
    RegSetValueExA(hk, "MS2", 0, REG_MULTI_SZ, (const BYTE *)"a\0b\0\0", 6);

    /* expand_sz: a controlled variable so both targets expand to the same
     * string. %TEMP% is not comparable -- it resolves to each target's own
     * temporary directory. */
    SetEnvironmentVariableA("LABTEMP", "C:\\labtemp");
    RegSetValueExA(hk, "EXPlonger", 0, REG_EXPAND_SZ, (const BYTE *)"%LABTEMP%\\aaaaaaaaaa", 21);
    /* expand_sz: %SystemRoot% expands to something shorter */
    RegSetValueExA(hk, "EXPshorter", 0, REG_EXPAND_SZ, (const BYTE *)"%SystemRoot%", 13);
    /* expand_sz: unknown variable expands to itself (equal) */
    RegSetValueExA(hk, "EXPequal", 0, REG_EXPAND_SZ, (const BYTE *)"%NOPE%", 7);

    printf("=== REG_SZ (no expansion) ===\n");
    row("SZ0", RRF_RT_REG_SZ, "SZ0 len0");
    row("SZ0raw", RRF_RT_REG_SZ, "SZ0raw zero bytes");
    row("SZ1", RRF_RT_REG_SZ, "SZ1 len1");
    row("SZ10", RRF_RT_REG_SZ, "SZ10 len10");
    row("SZ10nonul", RRF_RT_REG_SZ, "SZ10nonul no NUL");

    printf("=== REG_MULTI_SZ (no expansion) ===\n");
    row("MS1", RRF_RT_REG_MULTI_SZ, "MS1 a\\0\\0 (4 bytes)");
    row("MS2", RRF_RT_REG_MULTI_SZ, "MS2 a\\0b\\0\\0 (6 bytes)");

    printf("=== REG_EXPAND_SZ expanded (default) ===\n");
    row("EXPlonger", RRF_RT_REG_SZ, "EXPlonger %LABTEMP%");
    row("EXPshorter", RRF_RT_REG_SZ, "EXPshorter %SystemRoot%");
    row("EXPequal", RRF_RT_REG_SZ, "EXPequal %NOPE%");

    printf("=== REG_EXPAND_SZ with RRF_NOEXPAND ===\n");
    row("EXPlonger", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, "EXPlonger noexpand");
    row("EXPshorter", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, "EXPshorter noexpand");
    row("EXPequal", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, "EXPequal noexpand");

    RegCloseKey(hk);
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab");
    printf("PROBE_RESULT=OK\n");
    return 0;
}
