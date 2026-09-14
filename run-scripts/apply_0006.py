#!/usr/bin/env python3
"""Apply the GetProductInfo fix + test by exact-anchor replacement.
Fails loudly unless each anchor matches exactly once."""
import sys

WINE = "/home/kubuntu/adobe-wine-lab/src/wine"

HELPER = """/**********************************************************************
 *         is_core_edition
 *
 * Whether the running system identifies as the Core edition, consulting
 * the EditionId value a real machine carries under the CurrentVersion key.
 */
static BOOL is_core_edition(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW, valueW;
    HANDLE hkey;
    char tmp[128];
    DWORD count;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)tmp;
    BOOL ret = FALSE;

    RtlInitUnicodeString( &nameW, L"\\\\Registry\\\\Machine\\\\Software\\\\Microsoft\\\\Windows NT\\\\CurrentVersion" );
    InitializeObjectAttributes( &attr, &nameW, OBJ_CASE_INSENSITIVE, 0, NULL );
    if (NtOpenKey( &hkey, KEY_QUERY_VALUE, &attr )) return FALSE;

    RtlInitUnicodeString( &valueW, L"EditionId" );
    if (!NtQueryValueKey( hkey, &valueW, KeyValuePartialInformation, tmp, sizeof(tmp) - sizeof(WCHAR), &count ))
    {
        WCHAR *str = (WCHAR *)info->Data;
        str[info->DataLength / sizeof(WCHAR)] = 0;
        ret = !wcsicmp( str, L"Core" );
    }
    NtClose( hkey );
    return ret;
}

"""

OLD_BODY = """    if (current_version->wProductType == VER_NT_WORKSTATION)
        *pdwReturnedProductType = PRODUCT_ULTIMATE_N;
    else
        *pdwReturnedProductType = PRODUCT_STANDARD_SERVER;
"""

NEW_BODY = """    if (current_version->wProductType == VER_NT_WORKSTATION)
    {
        /* Windows reports the running edition mapped into the requested
         * schema: Core is PRODUCT_CORE on 10+ and PRODUCT_HOME_PREMIUM
         * where the schema predates it. Verified against Windows 10 Home. */
        BOOL core = is_core_edition();
        if (core && dwOSMajorVersion >= 10)
            *pdwReturnedProductType = PRODUCT_CORE;
        else if (core && dwOSMajorVersion == 6)
            *pdwReturnedProductType = PRODUCT_HOME_PREMIUM;
        else
            *pdwReturnedProductType = PRODUCT_ULTIMATE_N;
    }
    else
        *pdwReturnedProductType = PRODUCT_STANDARD_SERVER;
"""

TEST_ANCHOR = """    /* NULL pointer is not a problem */
    SetLastError(0xdeadbeef);
    res = pGetProductInfo(6, 1, 0, 0, NULL);
    ok( (!res) && (GetLastError() == 0xdeadbeef),
        "got %ld with 0x%lx (expected FALSE with LastError untouched\\n", res, GetLastError());
}
"""

TEST_NEW = """    /* NULL pointer is not a problem */
    SetLastError(0xdeadbeef);
    res = pGetProductInfo(6, 1, 0, 0, NULL);
    ok( (!res) && (GetLastError() == 0xdeadbeef),
        "got %ld with 0x%lx (expected FALSE with LastError untouched\\n", res, GetLastError());

    /* Core edition reports Core/HomePremium (verified on Windows 10 Home) */
    {
        HKEY hk;
        char saved[64];
        DWORD saved_len = sizeof(saved), type;
        LONG save_res;

        if (RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\\\Microsoft\\\\Windows NT\\\\CurrentVersion",
                           0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hk ))
        {
            win_skip( "cannot open CurrentVersion key\\n" );
            return;
        }
        save_res = RegQueryValueExA( hk, "EditionId", NULL, &type, (LPBYTE)saved, &saved_len );
        if (RegSetValueExA( hk, "EditionId", 0, REG_SZ, (const BYTE *)"Core", 5 ))
        {
            win_skip( "cannot stage Core EditionId\\n" );
            RegCloseKey( hk );
            return;
        }
        product = 0;
        res = pGetProductInfo( 10, 0, 0, 0, &product );
        ok( res && product == PRODUCT_CORE,
            "got %ld and %lu (expected TRUE and PRODUCT_CORE)\\n", res, product );
        product = 0;
        res = pGetProductInfo( 6, 0, 0, 0, &product );
        ok( res && product == PRODUCT_HOME_PREMIUM,
            "got %ld and %lu (expected TRUE and PRODUCT_HOME_PREMIUM)\\n", res, product );
        if (!save_res && type == REG_SZ)
            RegSetValueExA( hk, "EditionId", 0, REG_SZ, (const BYTE *)saved, saved_len );
        else
            RegDeleteValueA( hk, "EditionId" );
        RegCloseKey( hk );
    }
}
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
p1 = WINE + "/dlls/ntdll/version.c"
ok &= replace_once(p1, OLD_BODY, NEW_BODY)

with open(p1, "r", newline="") as f:
    t = f.read()
anchor = " *           RtlGetProductInfo    (NTDLL.@)"
if t.count(anchor) != 1:
    print(f"FAIL: helper anchor found {t.count(anchor)}x")
    ok = False
else:
    idx = t.index(anchor)
    # back up to the "/***..." comment start of that block
    start = t.rindex("/**", 0, idx)
    t = t[:start] + HELPER + t[start:]
    with open(p1, "w", newline="") as f:
        f.write(t)
    print("OK: helper inserted")

p2 = WINE + "/dlls/kernel32/tests/version.c"
ok &= replace_once(p2, TEST_ANCHOR, TEST_NEW)

print("APPLY_DONE" if ok else "APPLY_FAILED")
sys.exit(0 if ok else 1)
