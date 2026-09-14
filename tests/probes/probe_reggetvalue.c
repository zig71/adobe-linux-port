/* probe_reggetvalue.c — RegGetValueA/W type-restriction matrix.
 *
 * Divergence under investigation (Wine 11.17-dev, dlls/kernelbase/registry.c):
 *
 *   RegGetValueA/W contain a pre-check
 *       if ((dwFlags & RRF_RT_REG_EXPAND_SZ) && !(dwFlags & RRF_NOEXPAND) &&
 *           ((dwFlags & RRF_RT_ANY) != RRF_RT_ANY))
 *           return ERROR_INVALID_PARAMETER;
 *   Windows 8+ does not reject the flags up front: it expands first and then
 *   applies the type restriction, returning ERROR_UNSUPPORTED_TYPE when the
 *   (post-expansion) type is not permitted.
 *
 * This probe pins the exact Windows matrix so the Wine behaviour can be
 * reconciled rather than guessed.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_reggetvalue.exe probe_reggetvalue.c -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static void run(const char *label, HKEY hk, const char *value, DWORD flags) {
    char buf[256];
    DWORD size = sizeof(buf);
    DWORD type = 0xdeadbeef;
    LONG ret;

    memset(buf, 0, sizeof(buf));
    ret = RegGetValueA(hk, NULL, value, flags, &type, buf, &size);
    printf("%s ret=0x%08lx type=%lu size=%lu data=%s\n",
           label, (unsigned long)ret, (unsigned long)type, (unsigned long)size,
           (ret == ERROR_SUCCESS && buf[0]) ? buf : "-");

    /* no-buffer form: size-only query */
    size = 0;
    type = 0xdeadbeef;
    ret = RegGetValueA(hk, NULL, value, flags, &type, NULL, &size);
    printf("%s_nobuf ret=0x%08lx type=%lu size=%lu\n",
           label, (unsigned long)ret, (unsigned long)type, (unsigned long)size);
}

int main(void) {
    HKEY hk;
    DWORD disp;
    LONG lr;
    static const char *expand_src = "%SystemRoot%\\system32";
    static const char *plain = "C:\\Windows";
    const char *multi = "one\0two\0\0";
    DWORD dw = 0x11223344;
    ULONGLONG qw = 0x1122334455667788ULL;
    BYTE bin4[4] = { 1, 2, 3, 4 };

    lr = RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\ProbeRGV", 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &disp);
    if (lr != ERROR_SUCCESS) { printf("setup failed %ld\n", (long)lr); return 2; }

    RegSetValueExA(hk, "SZ", 0, REG_SZ, (const BYTE *)plain, (DWORD)strlen(plain) + 1);
    RegSetValueExA(hk, "EXP", 0, REG_EXPAND_SZ, (const BYTE *)expand_src, (DWORD)strlen(expand_src) + 1);
    RegSetValueExA(hk, "MULTI", 0, REG_MULTI_SZ, (const BYTE *)multi, 9);
    RegSetValueExA(hk, "DWORD", 0, REG_DWORD, (const BYTE *)&dw, sizeof(dw));
    RegSetValueExA(hk, "QWORD", 0, REG_QWORD, (const BYTE *)&qw, sizeof(qw));
    RegSetValueExA(hk, "BIN4", 0, REG_BINARY, bin4, sizeof(bin4));
    RegSetValueExA(hk, "NONE", 0, REG_NONE, (const BYTE *)"x", 1);

    printf("--- REG_EXPAND_SZ value, all flag combinations ---\n");
    run("EXP_regsz",           hk, "EXP", RRF_RT_REG_SZ);
    run("EXP_expandsz",        hk, "EXP", RRF_RT_REG_EXPAND_SZ);
    run("EXP_expandsz_noexp",  hk, "EXP", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND);
    run("EXP_sz_expandsz",     hk, "EXP", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ);
    run("EXP_sz_expandsz_noexp", hk, "EXP", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND);
    run("EXP_any",             hk, "EXP", RRF_RT_ANY);
    run("EXP_any_noexp",       hk, "EXP", RRF_RT_ANY | RRF_NOEXPAND);
    run("EXP_dword",           hk, "EXP", RRF_RT_REG_DWORD);
    run("EXP_multi",           hk, "EXP", RRF_RT_REG_MULTI_SZ);
    run("EXP_none",            hk, "EXP", RRF_RT_REG_NONE);

    printf("--- REG_SZ value ---\n");
    run("SZ_regsz",            hk, "SZ", RRF_RT_REG_SZ);
    run("SZ_expandsz",         hk, "SZ", RRF_RT_REG_EXPAND_SZ);
    run("SZ_expandsz_noexp",   hk, "SZ", RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND);
    run("SZ_sz_expandsz",      hk, "SZ", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ);
    run("SZ_any",              hk, "SZ", RRF_RT_ANY);

    printf("--- REG_MULTI_SZ value ---\n");
    run("MULTI_multi",         hk, "MULTI", RRF_RT_REG_MULTI_SZ);
    run("MULTI_regsz",         hk, "MULTI", RRF_RT_REG_SZ);
    run("MULTI_expandsz",      hk, "MULTI", RRF_RT_REG_EXPAND_SZ);
    run("MULTI_any",           hk, "MULTI", RRF_RT_ANY);

    printf("--- REG_DWORD value ---\n");
    run("DW_dword",            hk, "DWORD", RRF_RT_REG_DWORD);
    run("DW_regsz",            hk, "DWORD", RRF_RT_REG_SZ);
    run("DW_expandsz",         hk, "DWORD", RRF_RT_REG_EXPAND_SZ);
    run("DW_qword",            hk, "DWORD", RRF_RT_REG_QWORD);
    run("DW_dword_qword",      hk, "DWORD", RRF_RT_DWORD | RRF_RT_QWORD);
    run("DW_any",              hk, "DWORD", RRF_RT_ANY);

    printf("--- REG_QWORD value ---\n");
    run("QW_qword",            hk, "QWORD", RRF_RT_REG_QWORD);
    run("QW_dword",            hk, "QWORD", RRF_RT_REG_DWORD);
    run("QW_any",              hk, "QWORD", RRF_RT_ANY);

    printf("--- REG_BINARY value ---\n");
    run("BIN_binary",          hk, "BIN4", RRF_RT_REG_BINARY);
    run("BIN_dword",           hk, "BIN4", RRF_RT_REG_DWORD);
    run("BIN_binary_dword",    hk, "BIN4", RRF_RT_REG_BINARY | RRF_RT_REG_DWORD);
    run("BIN_regsz",           hk, "BIN4", RRF_RT_REG_SZ);

    printf("--- REG_NONE value ---\n");
    run("NONE_none",           hk, "NONE", RRF_RT_REG_NONE);
    run("NONE_any",            hk, "NONE", RRF_RT_ANY);
    run("NONE_regsz",          hk, "NONE", RRF_RT_REG_SZ);

    printf("--- missing value / invalid flags ---\n");
    run("MISSING_regsz",       hk, "NoSuchValue", RRF_RT_REG_SZ);
    run("MISSING_any",         hk, "NoSuchValue", RRF_RT_ANY);
    run("EXP_wow64both",       hk, "EXP", RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY | RRF_SUBKEY_WOW6432KEY);
    run("EXP_zeroonfail",      hk, "EXP", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_ZEROONFAILURE);

    RegCloseKey(hk);
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab");

    printf("PROBE_RESULT=OK\n");
    return 0;
}
