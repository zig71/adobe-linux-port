/* probe_expand.c — ExpandEnvironmentStringsA/W return-value semantics.
 *
 * Triggered by investigating RegGetValue's size reporting. The pattern
 *
 *     n = ExpandEnvironmentStringsA(src, dst, sizeof(dst));
 *
 * is documented to return the required size in characters including the
 * terminating NUL when the destination is NULL or too small. This probe pins
 * that return value for every destination size so the rule can be stated, and
 * so Wine's behaviour can be compared against Windows directly.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_expand.exe probe_expand.c -lkernel32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

/* "%LABX%" is 6 chars; LABX is set to a 4-character value, so the expanded
 * string is 4 characters + NUL = 5 characters. */
#define SRC "%LABX%"
#define EXPANDED_CHARS 4

static void run_a(int cap) {
    char dst[64];
    DWORD n;
    memset(dst, 0xcc, sizeof(dst));
    if (cap >= 0 && cap <= (int)sizeof(dst)) {
        n = ExpandEnvironmentStringsA(SRC, dst, (DWORD)cap);
        printf("A cap=%-3d ret=%-3lu nul=%d\n", cap, (unsigned long)n,
               (cap > 0 && n <= (DWORD)cap && dst[n ? n - 1 : 0] == 0) ? 1 : 0);
    } else {
        n = ExpandEnvironmentStringsA(SRC, NULL, 0);
        printf("A cap=NULL ret=%-3lu\n", (unsigned long)n);
    }
}

static void run_w(int cap) {
    WCHAR src[16], dst[64];
    DWORD n;
    MultiByteToWideChar(CP_ACP, 0, SRC, -1, src, 16);
    if (cap >= 0 && cap <= (int)(sizeof(dst) / sizeof(WCHAR))) {
        memset(dst, 0xcc, sizeof(dst));
        n = ExpandEnvironmentStringsW(src, dst, (DWORD)cap);
        printf("W cap=%-3d ret=%-3lu\n", cap, (unsigned long)n);
    } else {
        n = ExpandEnvironmentStringsW(src, NULL, 0);
        printf("W cap=NULL ret=%-3lu\n", (unsigned long)n);
    }
}

int main(void) {
    int cap;

    SetEnvironmentVariableA("LABX", "abcd");   /* expands to 4 chars */

    printf("src=\"%s\" expands to %d chars + NUL\n", SRC, EXPANDED_CHARS);

    /* NULL destination first */
    run_a(-1);
    run_w(-1);

    /* destinations that do not fit the result */
    run_a(0);
    run_a(1);
    run_a(2);
    run_a(4);
    run_a(5);
    run_a(6);

    /* destinations that fit exactly and with room to spare */
    run_a(7);
    run_a(8);
    run_a(16);

    run_w(0);
    run_w(1);
    run_w(4);
    run_w(5);
    run_w(6);
    run_w(8);

    /* A value with nothing to expand, for comparison */
    {
        char dst[64];
        DWORD n;
        memset(dst, 0xcc, sizeof(dst));
        n = ExpandEnvironmentStringsA("plaintext", NULL, 0);
        printf("plain cap=NULL ret=%lu\n", (unsigned long)n);
        n = ExpandEnvironmentStringsA("plaintext", dst, sizeof(dst));
        printf("plain cap=64   ret=%lu strlen=%lu\n", (unsigned long)n,
               (unsigned long)strlen(dst));
    }

    printf("PROBE_RESULT=OK\n");
    return 0;
}
