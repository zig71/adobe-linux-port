/* hello_win32.c - trivial Win32 console program.
 * Purpose: prove we can compile + run a native Win32 executable on the
 * Windows oracle, then run the SAME .exe under Wine on the Linux target.
 * Prints deterministic, comparable lines.
 */
#include <windows.h>
#include <stdio.h>

typedef LONG (WINAPI *RtlGetVersion_t)(PRTL_OSVERSIONINFOW);

int main(void) {
    RTL_OSVERSIONINFOW os;
    SYSTEM_INFO si;
    HMODULE ntdll;
    RtlGetVersion_t RtlGetVersion;
    char module[MAX_PATH];
    DWORD len;

    ZeroMemory(&os, sizeof(os));
    os.dwOSVersionInfoSize = sizeof(os);

    ntdll = GetModuleHandleA("ntdll.dll");
    RtlGetVersion = (RtlGetVersion_t)(void *)GetProcAddress(ntdll, "RtlGetVersion");
    if (RtlGetVersion) {
        RtlGetVersion(&os);
        printf("OSVERSION=%lu.%lu.%lu\n", os.dwMajorVersion, os.dwMinorVersion, os.dwBuildNumber);
        printf("OSCSD=%S\n", os.szCSDVersion);
    } else {
        printf("OSVERSION=unavailable\n");
    }

    GetNativeSystemInfo(&si);
    printf("ARCH=%u\n", (unsigned)si.wProcessorArchitecture);
    printf("PAGESIZE=%u\n", (unsigned)si.dwPageSize);
    printf("PTRSIZE=%u\n", (unsigned)(sizeof(void *)));

    len = GetModuleFileNameA(NULL, module, sizeof(module));
    printf("MODULE=%s\n", len ? module : "(error)");

    printf("LASTERROR=%lu\n", (unsigned long)GetLastError());
    printf("HELLO_OK\n");
    return 0;
}
