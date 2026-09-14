/* probe_rgv_raw.c — close the RegGetValue size rule.
 *
 * Decisive experiment for notes/divergence-002-reggetvalue-size.md. The two
 * open questions were:
 *
 *   Q1. Does Windows size the range from the *raw* stored value (R), or from
 *       the expansion (E)?  -- vary delta = E - raw_chars across -3..+3.
 *   Q2. Does the raw value's own trailing NUL matter?  -- store the same source
 *       text with and without a terminating NUL byte.
 *
 * The expansion target is fully controlled: a process environment variable is
 * set by the probe itself, so `%LABX%` expands to a string of exactly the
 * chosen length on both Windows and Wine, with no dependence on the host
 * environment.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_rgv_raw.exe probe_rgv_raw.c -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HKEY hk;

/* "%LABX%" is 6 characters + NUL = 7 bytes. */
#define SRC "%LABX%"
#define SRC_CHARS 6
#define SRC_BYTES 7

static void case_run(int value_chars, int with_nul) {
    char varname[32], value[64], name[64];
    DWORD flags = RRF_RT_REG_SZ;
    char buf[256];
    DWORD blen, size, type;
    DWORD sn = 0, tn = 0;
    LONG r, rn;
    int delta = value_chars - SRC_CHARS;

    memset(value, 'x', sizeof(value));
    value[value_chars] = 0;
    strcpy(varname, "LABX");
    SetEnvironmentVariableA(varname, value);

    sprintf(name, "V%+03d_%s", delta, with_nul ? "nul" : "nonul");

    /* Store SRC with, or without, its terminating NUL. */
    RegSetValueExA(hk, name, 0, REG_EXPAND_SZ,
                   (const BYTE *)SRC, with_nul ? SRC_BYTES : SRC_BYTES - 1);

    memset(buf, 0, sizeof(buf));
    blen = sizeof(buf);
    r = RegGetValueA(hk, NULL, name, flags, &type, buf, &blen);

    rn = RegGetValueA(hk, NULL, name, flags, &tn, NULL, &sn);

    /* Unique prefix per case: the comparison harness keys on the text before
     * the first '='. */
    printf("case E%+d_R%d%s buf ret=0x%08lx type=%lu size=%lu strlen=%lu"
           " nobuf ret=0x%08lx size=%lu\n",
           delta, with_nul ? SRC_BYTES : SRC_BYTES - 1, with_nul ? "_nul" : "_nonul",
           (unsigned long)r, (unsigned long)type, (unsigned long)blen,
           (unsigned long)strlen(buf), (unsigned long)rn, (unsigned long)sn);
}

int main(void) {
    DWORD disp;
    int len, nul;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\RGVRaw", 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &disp) != ERROR_SUCCESS) {
        printf("setup failed\n");
        return 2;
    }

    /* Environment variable name and source literal must agree. */
    if (strlen(SRC) != SRC_CHARS) { printf("SRC mismatch\n"); return 2; }

    printf("SRC=\"%s\" chars=%d bytes_with_nul=%d\n", SRC, SRC_CHARS, SRC_BYTES);

    for (nul = 1; nul >= 0; nul--)
        for (len = SRC_CHARS - 3; len <= SRC_CHARS + 3; len++)
            case_run(len, nul);

    RegCloseKey(hk);
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab");
    printf("PROBE_RESULT=OK\n");
    return 0;
}
