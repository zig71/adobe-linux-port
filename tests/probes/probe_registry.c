/* probe_registry.c — registry semantics oracle probe.
 *
 * Why: Adobe's installers, licensing and per-user configuration all lean on
 * registry semantics: key virtualization, WOW64 views, REG_EXPAND_SZ handling
 * and enumeration ordering. These are cheap to compare and cheap to fix.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_registry.exe probe_registry.c -ladvapi32
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static void pl(const char *k, LONG v) { printf("%s=%ld\n", k, (long)v); }
static void p(const char *k, const char *v) { printf("%s=%s\n", k, v ? v : "(null)"); }

int main(void) {
    HKEY hk = NULL, hk2 = NULL;
    LONG lr;
    DWORD disp = 0;

    /* --- create/delete a private test key under HKCU (no system impact) --- */
    lr = RegCreateKeyExA(HKEY_CURRENT_USER,
                         "Software\\AdobeWineLab\\ProbeReg", 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hk, &disp);
    pl("CreateKey", lr);
    pl("CreateKey_disposition", (LONG)disp);

    if (lr == ERROR_SUCCESS) {
        const char *s = "hello";
        DWORD dw = 0x12345678;
        const char *multi = "a\0b\0c\0\0";

        pl("SetValue_SZ", RegSetValueExA(hk, "Str", 0, REG_SZ, (const BYTE *)s, (DWORD)strlen(s) + 1));
        pl("SetValue_DWORD", RegSetValueExA(hk, "Dw", 0, REG_DWORD, (const BYTE *)&dw, sizeof(dw)));
        /* deliberately store a malformed DWORD size to observe divergence */
        pl("SetValue_DWORD3", RegSetValueExA(hk, "Dw3", 0, REG_DWORD, (const BYTE *)&dw, 3));
        pl("SetValue_MULTI", RegSetValueExA(hk, "Multi", 0, REG_MULTI_SZ, (const BYTE *)multi, 6));
        pl("SetValue_EXPAND", RegSetValueExA(hk, "Exp", 0, REG_EXPAND_SZ, (const BYTE *)"%SystemRoot%\\system32", 22));
        pl("SetValue_EMPTY_NAME", RegSetValueExA(hk, "", 0, REG_SZ, (const BYTE *)"root", 5));
        pl("SetValue_BINARY0", RegSetValueExA(hk, "Zero", 0, REG_BINARY, (const BYTE *)"", 0));
        pl("SetValue_NONE_TYPE", RegSetValueExA(hk, "NoneType", 0, REG_NONE, (const BYTE *)"x", 1));
        pl("SetValue_LINK", RegSetValueExA(hk, "Link", 0, REG_LINK, (const BYTE *)"\\Registry\\Machine", 36));

        /* --- querying type + size --- */
        {
            DWORD type = 0, size = 0;
            lr = RegQueryValueExA(hk, "Str", NULL, &type, NULL, &size);
            printf("Query_Str_nullbuf=0x%08lx type=%lu size=%lu\n", (unsigned long)lr,
                   (unsigned long)type, (unsigned long)size);
            lr = RegQueryValueExA(hk, "Dw", NULL, &type, NULL, &size);
            printf("Query_Dw_nullbuf=0x%08lx type=%lu size=%lu\n", (unsigned long)lr,
                   (unsigned long)type, (unsigned long)size);
            lr = RegQueryValueExA(hk, "Zero", NULL, &type, NULL, &size);
            printf("Query_Zero_nullbuf=0x%08lx type=%lu size=%lu\n", (unsigned long)lr,
                   (unsigned long)type, (unsigned long)size);
            /* nonexistent value */
            lr = RegQueryValueExA(hk, "NoSuchValue", NULL, &type, NULL, &size);
            printf("Query_missing=0x%08lx\n", (unsigned long)lr);
            /* buffer too small */
            {
                char small[2];
                DWORD s2 = sizeof(small);
                type = 0;
                lr = RegQueryValueExA(hk, "Str", NULL, &type, (BYTE *)small, &s2);
                printf("Query_toolsmall=0x%08lx needsize=%lu buf2=%c\n", (unsigned long)lr,
                       (unsigned long)s2, small[0] ? small[0] : '?');
            }
        }

        /* --- enumeration --- */
        {
            DWORD idx = 0, nsub = 0, nval = 0;
            char name[260];
            DWORD nlen;
            while (1) {
                nlen = sizeof(name);
                lr = RegEnumValueA(hk, idx, name, &nlen, NULL, NULL, NULL, NULL);
                if (lr != ERROR_SUCCESS) break;
                nval++;
                idx++;
            }
            printf("ENUM_VALUE_COUNT=%lu\n", (unsigned long)nval);
            idx = 0; nlen = sizeof(name);
            while (1) {
                nlen = sizeof(name);
                lr = RegEnumKeyExA(hk, idx, name, &nlen, NULL, NULL, NULL, NULL);
                if (lr != ERROR_SUCCESS) break;
                nsub++;
                idx++;
            }
            printf("ENUM_SUBKEY_COUNT=%lu\n", (unsigned long)nsub);
            idx = 9999; nlen = sizeof(name);
            lr = RegEnumValueA(hk, idx, name, &nlen, NULL, NULL, NULL, NULL);
            printf("ENUM_VALUE_out_of_range=0x%08lx\n", (unsigned long)lr);
        }

        /* --- RegQueryInfoKey counts --- */
        {
            DWORD sub = 0, maxsub, maxcls, val, maxnam, maxval, sec;
            lr = RegQueryInfoKeyA(hk, NULL, &maxcls, NULL, &sub, &maxsub, NULL, &val,
                                  &maxnam, &maxval, &sec, NULL);
            printf("QIK=0x%08lx subkeys=%lu values=%lu maxnamelen=%lu maxvalnamelen=%lu maxclslen=%lu\n",
                   (unsigned long)lr, (unsigned long)sub, (unsigned long)val,
                   (unsigned long)maxnam, (unsigned long)maxval, (unsigned long)maxcls);
        }

        /* --- RegQueryMultipleValues --- */
        {
            VALENT va[2];
            char b1[64], b2[64];
            DWORD need = 0;
            memset(va, 0, sizeof(va));
            va[0].ve_valuename = (LPSTR)"Str";
            va[0].ve_valuelen = sizeof(b1);
            va[0].ve_valueptr = (DWORD_PTR)b1;
            va[0].ve_type = 0;
            va[1].ve_valuename = (LPSTR)"Dw";
            va[1].ve_valuelen = sizeof(b2);
            va[1].ve_valueptr = (DWORD_PTR)b2;
            va[1].ve_type = 0;
            lr = RegQueryMultipleValuesA(hk, va, 2, NULL, &need);
            printf("QueryMultiple_nullbuf=0x%08lx need=%lu\n", (unsigned long)lr, (unsigned long)need);
        }

        /* --- RegCopyTree --- */
        {
            HKEY dst = NULL;
            RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\ProbeRegCopy", 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &dst, &disp);
            lr = RegCopyTreeA(hk, NULL, dst);
            pl("RegCopyTree", lr);
            if (dst) RegCloseKey(dst);
            RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\ProbeRegCopy");
        }

        RegCloseKey(hk);
    }

    /* --- WOW64 views of a real system key --- */
    {
        static const char *key = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion";
        struct { const char *n; REGSAM s; } views[] = {
            { "DEFAULT", 0 },
            { "64KEY", KEY_WOW64_64KEY },
            { "32KEY", KEY_WOW64_32KEY },
        };
        int i;
        for (i = 0; i < 3; i++) {
            HKEY k = NULL;
            char nm[64];
            DWORD val, sub;
            lr = RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ | views[i].s, &k);
            printf("VIEW_%s_open=0x%08lx\n", views[i].n, (unsigned long)lr);
            if (lr == ERROR_SUCCESS) {
                if (RegQueryInfoKeyA(k, NULL, NULL, NULL, &sub, NULL, NULL, &val, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
                    printf("VIEW_%s_subkeys_eq0=%d\n", views[i].n, sub == 0);
                RegCloseKey(k);
            }
            /* empty subkey name means "the key itself" */
            lr = RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ | views[i].s, &k);
            if (lr == ERROR_SUCCESS) {
                char buf[512];
                DWORD sz = sizeof(buf), type = 0;
                lr = RegQueryValueExA(k, "ProgramFilesDir", NULL, &type, (BYTE *)buf, &sz);
                if (lr == ERROR_SUCCESS) {
                    snprintf(nm, sizeof(nm), "VIEW_%s_ProgramFilesDir", views[i].n);
                    p(nm, buf);
                    printf("VIEW_%s_ProgramFilesDir_type=%lu\n", views[i].n, (unsigned long)type);
                } else printf("VIEW_%s_ProgramFilesDir=0x%08lx\n", views[i].n, (unsigned long)lr);
                RegCloseKey(k);
            }
        }
    }

    /* --- access-mask behavior on a non-existent key --- */
    {
        HKEY k = NULL;
        lr = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\AdobeWineLab\\DoesNotExist", 0, KEY_READ, &k);
        printf("OPEN_missing=0x%08lx\n", (unsigned long)lr);
        lr = RegOpenKeyExA(HKEY_LOCAL_MACHINE, NULL, 0, KEY_READ, &k);
        printf("OPEN_null_subkey=0x%08lx\n", (unsigned long)lr);
        lr = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE", 0, NULL, 0, KEY_READ, NULL, &k, &disp);
        printf("CREATE_existing_readonly=0x%08lx disp=%lu\n", (unsigned long)lr, (unsigned long)disp);
        if (lr == ERROR_SUCCESS) RegCloseKey(k);
    }

    /* --- HKEY_CLASSES_ROOT merged view sanity --- */
    {
        HKEY k = NULL;
        lr = RegOpenKeyExA(HKEY_CLASSES_ROOT, ".txt", 0, KEY_READ, &k);
        printf("HKCR_txt=0x%08lx\n", (unsigned long)lr);
        if (lr == ERROR_SUCCESS) {
            char buf[512];
            DWORD sz = sizeof(buf), type = 0;
            lr = RegQueryValueExA(k, NULL, NULL, &type, (BYTE *)buf, &sz);
            if (lr == ERROR_SUCCESS) { p("HKCR_txt_default", buf); printf("HKCR_txt_type=%lu\n", (unsigned long)type); }
            else printf("HKCR_txt_default=0x%08lx\n", (unsigned long)lr);
            RegCloseKey(k);
        }
    }

    /* --- RegLoadMUIString / RegGetValue semantics --- */
    {
        char buf[512];
        DWORD sz = sizeof(buf), type = 0;
        lr = RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                          "SystemRoot", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, &type, buf, &sz);
        printf("RegGetValue_SystemRoot=0x%08lx type=%lu\n", (unsigned long)lr, (unsigned long)type);
        sz = sizeof(buf); type = 0;
        lr = RegGetValueA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                          "SystemRoot", RRF_RT_REG_SZ, &type, buf, &sz);
        printf("RegGetValue_SystemRoot_SZonly=0x%08lx\n", (unsigned long)lr);
        sz = sizeof(buf); type = 0;
        lr = RegGetValueA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\ProbeReg", "Exp",
                          RRF_RT_REG_SZ, &type, buf, &sz);
        printf("RegGetValue_Exp_SZonly=0x%08lx\n", (unsigned long)lr);
        sz = sizeof(buf); type = 0;
        lr = RegGetValueA(HKEY_CURRENT_USER, "Software\\AdobeWineLab\\ProbeReg", "Exp",
                          RRF_RT_REG_EXPAND_SZ, &type, buf, &sz);
        printf("RegGetValue_Exp_EXPANDSZonly=0x%08lx\n", (unsigned long)lr);
    }

    (void)hk2;
    RegDeleteTreeA(HKEY_CURRENT_USER, "Software\\AdobeWineLab");
    printf("PROBE_RESULT=OK\n");
    return 0;
}
