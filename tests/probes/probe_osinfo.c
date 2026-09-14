/* probe_osinfo.c — version / edition / capability reporting.
 *
 * Why: Adobe installers and feature gates branch on Windows version, build
 * number, edition, product type and version-lie behavior. A divergence here
 * changes which code paths an application takes.
 *
 * Build (mingw):  x86_64-w64-mingw32-gcc -O2 -o probe_osinfo.exe probe_osinfo.c
 * Prints deterministic KEY=VALUE lines; identical binary runs on both OSes.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef LONG (WINAPI *RtlGetVersion_t)(PRTL_OSVERSIONINFOW);
typedef LONG (WINAPI *RtlGetNtVersionNumbers_t)(DWORD *, DWORD *, DWORD *);
typedef BOOL (WINAPI *IsWow64Process2_t)(HANDLE, USHORT *, USHORT *);

static void p(const char *k, const char *v) { printf("%s=%s\n", k, v ? v : "(null)"); }
static void pd(const char *k, DWORD v) { printf("%s=%lu\n", k, (unsigned long)v); }

int main(void) {
    OSVERSIONINFOEXA ovex;
    RTL_OSVERSIONINFOW rtl;
    HMODULE ntdll, k32;
    RtlGetVersion_t fRtlGetVersion;
    RtlGetNtVersionNumbers_t fNtVer;
    IsWow64Process2_t fWow64_2;
    DWORD major = 0, minor = 0, build = 0;
    USHORT procArch = 0, nativeArch = 0;
    DWORDLONG condMask;
    char buf[512];
    HKEY hk;
    DWORD sz, type;

    /* --- RtlGetVersion: the truth, unaffected by manifests --- */
    memset(&rtl, 0, sizeof(rtl));
    rtl.dwOSVersionInfoSize = sizeof(rtl);
    ntdll = GetModuleHandleA("ntdll.dll");
    fRtlGetVersion = (RtlGetVersion_t)(void *)GetProcAddress(ntdll, "RtlGetVersion");
    if (fRtlGetVersion && fRtlGetVersion(&rtl) == 0) {
        snprintf(buf, sizeof(buf), "%lu.%lu.%lu", (unsigned long)rtl.dwMajorVersion,
                 (unsigned long)rtl.dwMinorVersion, (unsigned long)rtl.dwBuildNumber);
        p("RTL_VERSION", buf);
        p("RTL_CSD", "");
        printf("RTL_PLATFORMID=%lu\n", (unsigned long)rtl.dwPlatformId);
        {
            WCHAR w[128];
            int i;
            for (i = 0; i < 127 && rtl.szCSDVersion[i]; i++) w[i] = rtl.szCSDVersion[i];
            w[i] = 0;
            printf("RTL_CSD_W=%ls\n", w);
        }
    } else {
        p("RTL_VERSION", "(failed)");
    }

    /* --- RtlGetNtVersionNumbers --- */
    fNtVer = (RtlGetNtVersionNumbers_t)(void *)GetProcAddress(ntdll, "RtlGetNtVersionNumbers");
    if (fNtVer) {
        fNtVer(&major, &minor, &build);
        snprintf(buf, sizeof(buf), "%lu.%lu.%lu (raw build=0x%08lx)", (unsigned long)major,
                 (unsigned long)minor, (unsigned long)(build & 0x0FFFFFFF), (unsigned long)build);
        p("NT_VERSION_NUMBERS", buf);
    } else {
        p("NT_VERSION_NUMBERS", "(missing)");
    }

    /* --- GetVersionExA (subject to manifest / version lie) --- */
    memset(&ovex, 0, sizeof(ovex));
    ovex.dwOSVersionInfoSize = sizeof(ovex);
    if (GetVersionExA((OSVERSIONINFOA *)&ovex)) {
        snprintf(buf, sizeof(buf), "%lu.%lu.%lu sp=%u.%u suite=%u prodtype=%u",
                 (unsigned long)ovex.dwMajorVersion, (unsigned long)ovex.dwMinorVersion,
                 (unsigned long)ovex.dwBuildNumber, (unsigned)ovex.wServicePackMajor,
                 (unsigned)ovex.wServicePackMinor, (unsigned)ovex.wSuiteMask,
                 (unsigned)ovex.wProductType);
        p("GETVERSIONEX", buf);
    } else {
        p("GETVERSIONEX", "(failed)");
    }

    /* --- VerifyVersionInfoA: what installers actually gate on --- */
    {
        OSVERSIONINFOEXA c;
        memset(&c, 0, sizeof(c));
        c.dwOSVersionInfoSize = sizeof(c);
        condMask = 0;
        c.dwMajorVersion = 10;
        c.dwMinorVersion = 0;
        c.dwBuildNumber = 19041;
        VerSetConditionMask(condMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
        VerSetConditionMask(condMask, VER_MINORVERSION, VER_GREATER_EQUAL);
        VerSetConditionMask(condMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        printf("VERIFY_ge_10.0.19041=%d\n",
               VerifyVersionInfoA(&c, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, condMask) ? 1 : 0);

        memset(&c, 0, sizeof(c));
        c.dwOSVersionInfoSize = sizeof(c);
        c.dwMajorVersion = 10;
        c.dwBuildNumber = 22000;
        condMask = 0;
        VerSetConditionMask(condMask, VER_MAJORVERSION, VER_EQUAL);
        VerSetConditionMask(condMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        printf("VERIFY_ge_10.0.22000=%d\n",
               VerifyVersionInfoA(&c, VER_MAJORVERSION | VER_BUILDNUMBER, condMask) ? 1 : 0);

        memset(&c, 0, sizeof(c));
        c.dwOSVersionInfoSize = sizeof(c);
        c.dwMajorVersion = 10;
        c.dwBuildNumber = 26000;
        condMask = 0;
        VerSetConditionMask(condMask, VER_MAJORVERSION, VER_EQUAL);
        VerSetConditionMask(condMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        printf("VERIFY_ge_10.0.26000=%d\n",
               VerifyVersionInfoA(&c, VER_MAJORVERSION | VER_BUILDNUMBER, condMask) ? 1 : 0);
    }

    /* --- architecture --- */
    fWow64_2 = (IsWow64Process2_t)(void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process2");
    if (fWow64_2 && fWow64_2(GetCurrentProcess(), &procArch, &nativeArch)) {
        printf("WOW64_PROCESS_ARCH=%u\n", (unsigned)procArch);
        printf("WOW64_NATIVE_ARCH=%u\n", (unsigned)nativeArch);
    } else {
        p("WOW64_PROCESS_ARCH", "(missing)");
    }
    {
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);
        printf("NATIVE_ARCH=%u\n", (unsigned)si.wProcessorArchitecture);
        printf("ALLOC_GRAN=%lu\n", (unsigned long)si.dwAllocationGranularity);
        printf("PROCESSOR_COUNT=%lu\n", (unsigned long)si.dwNumberOfProcessors);
    }

    /* --- edition / product name from registry --- */
    k32 = GetModuleHandleA("kernel32.dll");
    (void)k32;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        char cv[256];
        sz = sizeof(cv); type = 0;
        if (RegQueryValueExA(hk, "ProductName", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) {
            p("REG_PRODUCTNAME", cv);
            printf("REG_PRODUCTNAME_TYPE=%lu\n", (unsigned long)type);
        } else p("REG_PRODUCTNAME", "(missing)");

        sz = sizeof(cv); type = 0;
        if (RegQueryValueExA(hk, "DisplayVersion", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) p("REG_DISPLAYVERSION", cv);
        else p("REG_DISPLAYVERSION", "(missing)");

        sz = sizeof(cv); type = 0;
        if (RegQueryValueExA(hk, "CurrentBuild", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) p("REG_CURRENTBUILD", cv);
        else p("REG_CURRENTBUILD", "(missing)");

        sz = sizeof(cv); type = 0;
        if (RegQueryValueExA(hk, "UBR", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) {
            DWORD ubr = 0;
            memcpy(&ubr, cv, sizeof(DWORD) < sz ? sizeof(DWORD) : sz);
            pd("REG_UBR", ubr);
        } else p("REG_UBR", "(missing)");

        sz = sizeof(cv); type = 0;
        if (RegQueryValueExA(hk, "EditionID", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) p("REG_EDITIONID", cv);
        else p("REG_EDITIONID", "(missing)");

        sz = sizeof(cv); type = 0;
        if (RegQueryValueExA(hk, "InstallationType", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) p("REG_INSTALLATIONTYPE", cv);
        else p("REG_INSTALLATIONTYPE", "(missing)");

        sz = sizeof(cv); type = 0;
        if (RegQueryValueExA(hk, "BuildLabEx", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) p("REG_BUILDLABEX", cv);
        else p("REG_BUILDLABEX", "(missing)");

        RegCloseKey(hk);
    } else {
        p("REG_CurrentVersion", "(open failed)");
    }

    /* --- GetProductInfo at the reported OS version and at Windows 10/11 --- */
    {
        DWORD pt = 0;
        if (GetProductInfo(6, 0, 0, 0, &pt)) pd("PRODUCT_INFO_6_0", pt);
        else pd("PRODUCT_INFO_6_0", 0);
        pt = 0;
        if (GetProductInfo(10, 0, 0, 0, &pt)) pd("PRODUCT_INFO_10_0", pt);
        else pd("PRODUCT_INFO_10_0", 0);
    }

    /* --- WOW64 redirection visibility --- */
    {
        HKEY hk2;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ | KEY_WOW64_64KEY, &hk2) == ERROR_SUCCESS) {
            p("WOW64_64KEY_OPEN", "ok");
            RegCloseKey(hk2);
        } else p("WOW64_64KEY_OPEN", "failed");
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_READ | KEY_WOW64_32KEY, &hk2) == ERROR_SUCCESS) {
            p("WOW64_32KEY_OPEN", "ok");
            RegCloseKey(hk2);
        } else p("WOW64_32KEY_OPEN", "failed");
    }

    /* --- suite mask (gates Home-vs-Pro branches) --- */
    {
        RTL_OSVERSIONINFOEXW rex;
        memset(&rex, 0, sizeof(rex));
        rex.dwOSVersionInfoSize = sizeof(rex);
        if (fRtlGetVersion && fRtlGetVersion((PRTL_OSVERSIONINFOW)&rex) == 0)
            printf("RTL_SUITEMASK=%u\n", (unsigned)rex.wSuiteMask);
    }
    memset(&ovex, 0, sizeof(ovex));
    ovex.dwOSVersionInfoSize = sizeof(ovex);
    if (GetVersionExA((OSVERSIONINFOA *)&ovex))
        printf("GVE_SUITEMASK=%u\n", (unsigned)ovex.wSuiteMask);
    /* --- machine profile (the rest of the requirements-gate inputs) --- */
    {
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);
        printf("MACH_arch=%u\n", (unsigned)si.wProcessorArchitecture);
        printf("MACH_cores=%lu\n", (unsigned long)si.dwNumberOfProcessors);
        printf("MACH_page=%lu\n", (unsigned long)si.dwPageSize);
        printf("MACH_gran=%lu\n", (unsigned long)si.dwAllocationGranularity);
        printf("MACH_proclevel=%u\n", (unsigned)si.wProcessorLevel);
        printf("MACH_procrev=%u\n", (unsigned)si.wProcessorRevision);
    }
    {
        MEMORYSTATUSEX ms;
        ms.dwLength = sizeof(ms);
        if (GlobalMemoryStatusEx(&ms)) {
            printf("MEM_pct=%lu\n", (unsigned long)ms.dwMemoryLoad);
            printf("MEM_totalMB=%llu\n", (unsigned long long)(ms.ullTotalPhys / (1024*1024)));
            printf("MEM_availMB=%llu\n", (unsigned long long)(ms.ullAvailPhys / (1024*1024)));
            printf("MEM_pageMB=%llu\n", (unsigned long long)(ms.ullTotalPageFile / (1024*1024)));
        } else p("MEM", "(failed)");
    }
    {
        ULARGE_INTEGER freeB, totalB, freeU;
        if (GetDiskFreeSpaceExA("C:\\", &freeU, &totalB, &freeB)) {
            printf("DISK_totalGB=%llu\n", (unsigned long long)(totalB.QuadPart / (1024*1024*1024)));
            printf("DISK_freeGB=%llu\n", (unsigned long long)(freeB.QuadPart / (1024*1024*1024)));
        } else p("DISK", "(failed)");
    }
    {
        DYNAMIC_TIME_ZONE_INFORMATION tz;
        if (GetDynamicTimeZoneInformation(&tz) != TIME_ZONE_ID_INVALID) {
            printf("TZ_bias=%ld\n", (long)tz.Bias);
            printf("TZ_stdname=%ls\n", tz.StandardName);
        } else p("TZ", "(failed)");
    }
    {
        HKEY hk;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
            char cv[256]; DWORD sz, type;
            sz = sizeof(cv); type = 0;
            if (RegQueryValueExA(hk, "SystemProductName", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) p("BIOS_product", cv);
            else p("BIOS_product", "(missing)");
            sz = sizeof(cv); type = 0;
            if (RegQueryValueExA(hk, "SystemManufacturer", NULL, &type, (LPBYTE)cv, &sz) == ERROR_SUCCESS) p("BIOS_vendor", cv);
            else p("BIOS_vendor", "(missing)");
            RegCloseKey(hk);
        } else p("BIOS", "(open failed)");
    }
    {
        DWORD len = GetSystemFirmwareTable('RSMB', 0, NULL, 0);
        printf("SMBIOS_len=%lu\n", (unsigned long)len);
    }
    /* --- session identity (interactive vs service session gates) --- */
    {
        DWORD sid = 0xFFFFFFFF;
        if (ProcessIdToSessionId(GetCurrentProcessId(), &sid))
            printf("SESSION_id=%lu\n", (unsigned long)sid);
        else
            printf("SESSION_id=(failed:%lu)\n", (unsigned long)GetLastError());
    }
    /* --- sandbox-relevant token/job state -------------------------------- */
    {
        HANDLE tok = NULL;
        DWORD len = 0;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) {
            char buf[256];
            DWORD rl = sizeof(buf);
            if (GetTokenInformation(tok, TokenIntegrityLevel, buf, sizeof(buf), &rl)) {
                TOKEN_MANDATORY_LABEL *tml = (TOKEN_MANDATORY_LABEL *)buf;
                DWORD sub = 0, cnt = 0;
                if (GetSidSubAuthorityCount(tml->Label.Sid)) {
                    cnt = *(GetSidSubAuthorityCount(tml->Label.Sid));
                    if (cnt > 0) sub = *(GetSidSubAuthority(tml->Label.Sid, cnt - 1));
                }
                printf("TOKEN_integrity_rid=%lu\n", (unsigned long)sub);
            } else printf("TOKEN_integrity=(failed:%lu)\n", (unsigned long)GetLastError());
            {
                TOKEN_TYPE tt = TokenPrimary;
                DWORD rl2 = sizeof(tt);
                if (GetTokenInformation(tok, TokenType, &tt, sizeof(tt), &rl2))
                    printf("TOKEN_type=%d\n", (int)tt);
                else printf("TOKEN_type=(failed)\n");
            }
            CloseHandle(tok);
        } else printf("TOKEN_open=(failed:%lu)\n", (unsigned long)GetLastError());
        {
            BOOL injob = FALSE;
            if (IsProcessInJob(GetCurrentProcess(), NULL, &injob))
                printf("JOB_injob=%d\n", injob ? 1 : 0);
            else printf("JOB_injob=(failed)\n");
        }
    }
    /* --- screen-reader presence (drives Chromium a11y tree depth) -------- */
    {
        BOOL sr = FALSE;
        if (SystemParametersInfoA(SPI_GETSCREENREADER, 0, &sr, 0))
            printf("A11Y_screenreader=%d\n", sr ? 1 : 0);
        else
            printf("A11Y_screenreader=(failed)\n");
    }
    p("PROBE_RESULT", "OK");

    return 0;
}
