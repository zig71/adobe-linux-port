#!/usr/bin/env python3
"""Second fix: derive wSuiteMask from the Core edition (was always 0 on the
registry path; Windows 10 Home reports 768 = SINGLEUSERTS|PERSONAL).
Plus test + probe coverage. Exact-anchor, fails loudly otherwise."""
import sys

WINE = "/home/kubuntu/adobe-wine-lab/src/wine"
LAB = "/home/kubuntu/adobe-wine-lab"

SUITE_ANCHOR = "        /* FIXME: get wSuiteMask */\n"

SUITE_NEW = """        /* Derive the suite mask from the edition: Core (Home) systems carry
         * VER_SUITE_SINGLEUSERTS | VER_SUITE_PERSONAL (Windows 10 Home
         * reports 768). Other editions keep the previous value. */
        RtlInitUnicodeString( &valueW, L"EditionId" );
        if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp) - sizeof(WCHAR), &count ))
        {
            WCHAR *str = (WCHAR *)info->Data;
            str[info->DataLength / sizeof(WCHAR)] = 0;
            if (!wcsicmp( str, L"Core" ))
                version->wSuiteMask = VER_SUITE_SINGLEUSERTS | VER_SUITE_PERSONAL;
        }
"""

TEST_ANCHOR = """        if (!save_res && type == REG_SZ)
            RegSetValueExA( hk, "EditionId", 0, REG_SZ, (const BYTE *)saved, saved_len );
        else
            RegDeleteValueA( hk, "EditionId" );
        RegCloseKey( hk );
"""

TEST_NEW = """        if (pRtlGetVersion)
        {
            RTL_OSVERSIONINFOEXW ver;
            memset( &ver, 0, sizeof(ver) );
            ver.dwOSVersionInfoSize = sizeof(ver);
            if (!pRtlGetVersion( &ver ))
                ok( ver.wSuiteMask == (VER_SUITE_SINGLEUSERTS | VER_SUITE_PERSONAL),
                    "got suite %#x (expected SINGLEUSERTS|PERSONAL)\\n", ver.wSuiteMask );
            else
                ok( 0, "RtlGetVersion failed\\n" );
        }
        else win_skip( "RtlGetVersion not available\\n" );
        if (!save_res && type == REG_SZ)
            RegSetValueExA( hk, "EditionId", 0, REG_SZ, (const BYTE *)saved, saved_len );
        else
            RegDeleteValueA( hk, "EditionId" );
        RegCloseKey( hk );
"""

PROBE_ANCHOR = '    p("PROBE_RESULT", "OK");\n'

PROBE_NEW = """    /* --- suite mask (gates Home-vs-Pro branches) --- */
    {
        RTL_OSVERSIONINFOEXW rex;
        memset( &rex, 0, sizeof(rex) );
        rex.dwOSVersionInfoSize = sizeof(rex);
        if (fRtlGetVersion && fRtlGetVersion( (PRTL_OSVERSIONINFOW)&rex ) == 0)
            printf( "RTL_SUITEMASK=%u\\n", (unsigned)rex.wSuiteMask );
    }
    memset( &ovex, 0, sizeof(ovex) );
    ovex.dwOSVersionInfoSize = sizeof(ovex);
    if (GetVersionExA( (OSVERSIONINFOA *)&ovex ))
        printf( "GVE_SUITEMASK=%u\\n", (unsigned)ovex.wSuiteMask );

    p("PROBE_RESULT", "OK");
"""


def replace_once(path, old, new):
    with open(path, "r", newline="") as f:
        text = f.read()
    n = text.count(old)
    if n != 1:
        print(f"FAIL: anchor found {n}x in {path}")
        return False
    with open(path, "w", newline="") as f:
        f.write(text.replace(old, new))
    print(f"OK: applied in {path}")
    return True


ok = True
ok &= replace_once(WINE + "/dlls/ntdll/version.c", SUITE_ANCHOR, SUITE_NEW)
ok &= replace_once(WINE + "/dlls/kernel32/tests/version.c", TEST_ANCHOR, TEST_NEW)

print("APPLY_DONE" if ok else "APPLY_FAILED")
sys.exit(0 if ok else 1)
