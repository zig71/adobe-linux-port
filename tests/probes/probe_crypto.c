/* probe_crypto.c — CryptoAPI / certificate store oracle probe.
 *
 * Why: Creative Cloud's licensing and telemetry flows validate certificate
 * chains and use CryptProtectData for per-user secrets. Divergences in chain
 * building, store contents or DPAPI availability are directly causal.
 *
 * Build: x86_64-w64-mingw32-gcc -O2 -o probe_crypto.exe probe_crypto.c \
 *            -lcrypt32 -lwintrust -ladvapi32 -lole32
 */
#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <string.h>

static void pl(const char *k, LONG v) { printf("%s=%ld\n", k, (long)v); }
static void pe(const char *k, DWORD e) { printf("%s=%lu\n", k, (unsigned long)e); }

int main(void) {
    /* --- provider availability --- */
    {
        HCRYPTPROV prov = 0;
        BOOL ok;
        ok = CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
        printf("AcquireContext_RSA_AES=%d err=%lu\n", ok ? 1 : 0, (unsigned long)GetLastError());
        if (ok) {
            DWORD len = 0;
            CryptGenRandom(prov, 32, (BYTE *)&len);
            printf("CryptGenRandom_32=1\n");
            {
                HCRYPTKEY key = 0;
                if (CryptGenKey(prov, CALG_AES_256, CRYPT_EXPORTABLE, &key)) {
                    DWORD blen = 0;
                    printf("CryptGenKey_AES256=1\n");
                    /* exporting a session key with no key exchange key fails */
                    CryptExportKey(key, 0, PLAINTEXTKEYBLOB, 0, NULL, &blen);
                    printf("CryptExportKey_plaintext_err=%lu need=%lu\n",
                           (unsigned long)GetLastError(), (unsigned long)blen);
                    CryptDestroyKey(key);
                } else pe("CryptGenKey_AES256_err", GetLastError());
            }
            {
                HCRYPTHASH hash = 0;
                if (CryptCreateHash(prov, CALG_SHA_256, 0, 0, &hash)) {
                    BYTE data[] = "adobe-wine-lab";
                    BYTE digest[64];
                    DWORD dlen = sizeof(digest);
                    CryptHashData(hash, data, (DWORD)strlen((char *)data), 0);
                    if (CryptGetHashParam(hash, HP_HASHVAL, digest, &dlen, 0)) {
                        printf("SHA256_len=%lu\n", (unsigned long)dlen);
                        printf("SHA256_prefix=%02x%02x%02x%02x\n", digest[0], digest[1], digest[2], digest[3]);
                    } else pe("CryptGetHashParam_err", GetLastError());
                    CryptDestroyHash(hash);
                } else pe("CryptCreateHash_SHA256_err", GetLastError());
            }
            {
                /* Calg names for modern algorithms */
                static const ALG_ID algs[] = { CALG_SHA_256, CALG_SHA_384, CALG_SHA_512,
                                               CALG_AES_128, CALG_AES_256, CALG_ECDSA };
                const char *names[] = { "SHA256", "SHA384", "SHA512", "AES128", "AES256", "ECDSA" };
                int i;
                for (i = 0; i < 6; i++) {
                    DWORD len = 0;
                    if (CryptFindOIDInfo(CRYPT_OID_INFO_ALGID_KEY, (void *)&algs[i], 0))
                        printf("OIDINFO_%s=present\n", names[i]);
                    else printf("OIDINFO_%s=absent\n", names[i]);
                    (void)len;
                }
            }
            CryptReleaseContext(prov, 0);
        }
    }

    /* --- certificate stores --- */
    {
        static const char *stores[] = { "MY", "ROOT", "CA", "TrustedPublisher", "Disallowed", "ADDRESSBOOK", "AuthRoot" };
        int i;
        for (i = 0; i < (int)(sizeof(stores) / sizeof(stores[0])); i++) {
            HCERTSTORE h = CertOpenSystemStoreA(0, stores[i]);
            printf("Store_%s_nonnull=%d\n", stores[i], h != NULL);
            if (h) {
                DWORD n = 0;
                PCCERT_CONTEXT c = NULL;
                while ((c = CertEnumCertificatesInStore(h, c)) != NULL) n++;
                printf("Store_%s_count=%lu\n", stores[i], (unsigned long)n);
                CertCloseStore(h, 0);
            } else pe("Store_err", GetLastError());
        }
    }

    /* --- chain building on a value, and DPAPI --- */
    {
        DATA_BLOB in, out, out2;
        BYTE secret[] = "adobe-wine-lab entropy";
        BYTE entropy[] = "lab";
        in.pbData = secret; in.cbData = (DWORD)strlen((char *)secret);
        out.pbData = NULL; out.cbData = 0;
        if (CryptProtectData(&in, L"lab", NULL, NULL, NULL, 0, &out)) {
            printf("CryptProtectData=1 size=%lu\n", (unsigned long)out.cbData);
            if (CryptUnprotectData(&out, NULL, NULL, NULL, NULL, 0, &out2)) {
                printf("CryptUnprotectData=1 match=%d\n",
                       (out2.cbData == in.cbData && !memcmp(out2.pbData, in.pbData, in.cbData)) ? 1 : 0);
                LocalFree(out2.pbData);
            } else pe("CryptUnprotectData_err", GetLastError());
            LocalFree(out.pbData);
        } else pe("CryptProtectData_err", GetLastError());

        /* entropy variant */
        in.pbData = secret; in.cbData = (DWORD)strlen((char *)secret);
        out.pbData = NULL; out.cbData = 0;
        {
            DATA_BLOB ent;
            ent.pbData = entropy; ent.cbData = (DWORD)strlen((char *)entropy);
            if (CryptProtectData(&in, L"lab", &ent, NULL, NULL, 0, &out)) {
                printf("CryptProtectData_entropy=1 size=%lu\n", (unsigned long)out.cbData);
                out2.pbData = NULL; out2.cbData = 0;
                if (CryptUnprotectData(&out, NULL, &ent, NULL, NULL, 0, &out2)) {
                    printf("CryptUnprotectData_entropy=1 match=%d\n",
                           (out2.cbData == in.cbData && !memcmp(out2.pbData, in.pbData, in.cbData)) ? 1 : 0);
                    LocalFree(out2.pbData);
                } else pe("CryptUnprotectData_entropy_err", GetLastError());
                out2.pbData = NULL; out2.cbData = 0;
                if (CryptUnprotectData(&out, NULL, NULL, NULL, NULL, 0, &out2)) {
                    printf("CryptUnprotectData_wrongentropy=1 (unexpected)\n");
                    LocalFree(out2.pbData);
                } else pe("CryptUnprotectData_wrongentropy_err", GetLastError());
                LocalFree(out.pbData);
            } else pe("CryptProtectData_entropy_err", GetLastError());
        }
    }

    /* --- CryptStringToBinary - used by PEM handling in installers --- */
    {
        const char *b64 = "YWRvYmUtd2luZS1sYWI=";
        BYTE buf[64];
        DWORD len = sizeof(buf);
        if (CryptStringToBinaryA(b64, 0, CRYPT_STRING_BASE64, buf, &len, NULL, NULL)) {
            printf("CryptStringToBinary=1 len=%lu text=%s\n", (unsigned long)len, (char *)buf);
        } else pe("CryptStringToBinary_err", GetLastError());
        len = sizeof(buf);
        if (CryptStringToBinaryA("not!base64!", 0, CRYPT_STRING_BASE64, buf, &len, NULL, NULL))
            printf("CryptStringToBinary_invalid=1\n");
        else pe("CryptStringToBinary_invalid_err", GetLastError());
    }

    /* --- CryptGenRandom determinism properties (only shape, not values) --- */
    {
        HCRYPTPROV prov = 0;
        if (CryptAcquireContextA(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            BYTE a[16], b[16];
            int diff = 0, i;
            CryptGenRandom(prov, 16, a);
            CryptGenRandom(prov, 16, b);
            for (i = 0; i < 16; i++) if (a[i] != b[i]) diff++;
            printf("CryptGenRandom_distinct_bytes=%d\n", diff > 0 ? 1 : 0);
            CryptReleaseContext(prov, 0);
        }
    }

    (void)pl;
    printf("PROBE_RESULT=OK\n");
    return 0;
}
