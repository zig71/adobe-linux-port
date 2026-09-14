/* probe_fls.c — FlsGetValue2 availability and semantics.
 *
 * Why: tracing GetProcAddress during WebView2 environment creation showed
 * exactly one export resolving to NULL, immediately before Chromium's host DLL
 * faults with eip=0 (a call through an unpopulated function pointer):
 *
 *     NULL  "FlsGetValue2"
 *
 * Windows kernel32 exports FlsGetValue2; Wine's does not — it has only
 * FlsAlloc/FlsFree/FlsGetValue/FlsSetValue. This probe pins the Windows
 * behaviour of FlsGetValue2 so the Wine implementation can match it, rather
 * than assuming it is a plain alias of FlsGetValue.
 *
 * Specifically it checks whether the two differ in:
 *   - whether a successful call clears the last error
 *   - what the last error is for an unused index
 *   - what the last error is for an invalid index
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_fls.exe probe_fls.c
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

typedef PVOID (WINAPI *fls_get_t)(DWORD);

int main(void) {
    HMODULE k32;
    fls_get_t fls_get, fls_get2;
    DWORD idx;
    char *probe_value = (char *)0x11223344;
    PVOID got;

    printf("PROBE_BITS=%u\n", (unsigned)(sizeof(void *) * 8));

    k32 = GetModuleHandleA("kernel32.dll");
    fls_get = (fls_get_t)(void *)GetProcAddress(k32, "FlsGetValue");
    fls_get2 = (fls_get_t)(void *)GetProcAddress(k32, "FlsGetValue2");

    printf("FlsGetValue_present=%d\n", fls_get != NULL);
    printf("FlsGetValue2_present=%d\n", fls_get2 != NULL);
    printf("FlsAlloc_present=%d\n", GetProcAddress(k32, "FlsAlloc") != NULL);
    printf("FlsSetValue_present=%d\n", GetProcAddress(k32, "FlsSetValue") != NULL);
    printf("FlsFree_present=%d\n", GetProcAddress(k32, "FlsFree") != NULL);

    if (!fls_get) { printf("PROBE_RESULT=NO_FlsGetValue\n"); return 2; }

    /* --- an allocated, unused index ------------------------------------ */
    idx = FlsAlloc(NULL);
    printf("FlsAlloc_idx_nonzero=%d\n", idx != FLS_OUT_OF_INDEXES);

    SetLastError(0xDEADBEEF);
    got = fls_get(idx);
    printf("FlsGetValue_unused=null=%d lasterror=0x%08lx\n",
           got == NULL, (unsigned long)GetLastError());

    if (fls_get2) {
        SetLastError(0xDEADBEEF);
        got = fls_get2(idx);
        printf("FlsGetValue2_unused=null=%d lasterror=0x%08lx\n",
               got == NULL, (unsigned long)GetLastError());
    }

    /* --- an index holding a value -------------------------------------- */
    FlsSetValue(idx, probe_value);
    SetLastError(0xDEADBEEF);
    got = fls_get(idx);
    printf("FlsGetValue_set_matches=%d lasterror=0x%08lx\n",
           got == probe_value, (unsigned long)GetLastError());

    FlsSetValue(idx, probe_value);
    if (fls_get2) {
        SetLastError(0xDEADBEEF);
        got = fls_get2(idx);
        printf("FlsGetValue2_set_matches=%d lasterror=0x%08lx\n",
               got == probe_value, (unsigned long)GetLastError());
    }

    FlsFree(idx);

    /* --- an invalid index ---------------------------------------------- */
    SetLastError(0xDEADBEEF);
    got = fls_get(0xFFFF);
    printf("FlsGetValue_invalid=null=%d lasterror=0x%08lx\n",
           got == NULL, (unsigned long)GetLastError());

    if (fls_get2) {
        SetLastError(0xDEADBEEF);
        got = fls_get2(0xFFFF);
        printf("FlsGetValue2_invalid=null=%d lasterror=0x%08lx\n",
               got == NULL, (unsigned long)GetLastError());
    }

    /* --- do they return the same thing for every live index? ----------- */
    if (fls_get2) {
        int i, differ = 0;
        for (i = 0; i < 32; i++) {
            PVOID a = fls_get((DWORD)i);
            PVOID b = fls_get2((DWORD)i);
            if (a != b) differ++;
        }
        printf("index_scan_differing=%d\n", differ);
    }

    printf("PROBE_RESULT=OK\n");
    return 0;
}
