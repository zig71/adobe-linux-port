/* probe_gpu.c - portable CUDA-driver-API probe.
 *
 * Loads the CUDA driver dynamically (nvcuda.dll / libcuda.so.1) so the same
 * source builds on Windows (cl) and Linux (gcc) with no SDK/lib dependency.
 * Prints deterministic lines; used to verify GPU acceleration exists on both
 * the Windows oracle and the Linux target (GPU-P / dxgkrnl passthrough).
 */
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define LIBNAME "nvcuda.dll"
static void *load_lib(const char *n) { return (void *)LoadLibraryA(n); }
static void *load_sym(void *h, const char *n) { return (void *)GetProcAddress((HMODULE)h, n); }
#else
#include <dlfcn.h>
#define LIBNAME "libcuda.so.1"
static void *load_lib(const char *n) { return dlopen(n, RTLD_NOW | RTLD_GLOBAL); }
static void *load_sym(void *h, const char *n) { return dlsym(h, n); }
#endif

typedef int CUresult;
typedef int CUdevice;
typedef struct CUctx_st *CUcontext;

#define CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR 75
#define CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR 76
#define CU_DEVICE_ATTRIBUTE_TOTAL_MEMORY 1

typedef CUresult (*cuInit_t)(unsigned int);
typedef CUresult (*cuDriverGetVersion_t)(int *);
typedef CUresult (*cuDeviceGetCount_t)(int *);
typedef CUresult (*cuDeviceGet_t)(CUdevice *, int);
typedef CUresult (*cuDeviceGetName_t)(char *, int, CUdevice);
typedef CUresult (*cuDeviceTotalMem_t)(size_t *, CUdevice);
typedef CUresult (*cuDeviceGetAttribute_t)(int *, int, CUdevice);
typedef CUresult (*cuCtxCreate_t)(CUcontext *, unsigned int, CUdevice);
typedef CUresult (*cuCtxDestroy_t)(CUcontext);
typedef CUresult (*cuGetErrorString_t)(CUresult, const char **);

static const char *cuda_err(cuGetErrorString_t f, CUresult r) {
    const char *s = NULL;
    if (f && f(r, &s) == 0 && s) return s;
    return "?";
}

int main(void) {
    void *lib;
    cuInit_t p_cuInit;
    cuDriverGetVersion_t p_cuDriverGetVersion;
    cuDeviceGetCount_t p_cuDeviceGetCount;
    cuDeviceGet_t p_cuDeviceGet;
    cuDeviceGetName_t p_cuDeviceGetName;
    cuDeviceTotalMem_t p_cuDeviceTotalMem;
    cuDeviceGetAttribute_t p_cuDeviceGetAttribute;
    cuCtxCreate_t p_cuCtxCreate;
    cuCtxDestroy_t p_cuCtxDestroy;
    cuGetErrorString_t p_cuGetErrorString;
    int driverver = -1, count = -1, major = -1, minor = -1, r;
    CUdevice dev = -1;
    CUcontext ctx = NULL;
    char name[256];
    size_t total = 0;

    memset(name, 0, sizeof(name));

    lib = load_lib(LIBNAME);
    if (!lib) {
        printf("CUDA_LIB=%s LOAD=FAILED\n", LIBNAME);
        printf("PROBE_RESULT=NO_CUDA_DRIVER\n");
        return 2;
    }
    printf("CUDA_LIB=%s LOAD=OK\n", LIBNAME);

#define SYM(v, n)                                   \
    do {                                            \
        v = (void *)load_sym(lib, n);               \
        if (!v) { printf("SYM_%s=MISSING\n", n); }  \
    } while (0)

    SYM(p_cuInit, "cuInit");
    SYM(p_cuDriverGetVersion, "cuDriverGetVersion");
    SYM(p_cuDeviceGetCount, "cuDeviceGetCount");
    SYM(p_cuDeviceGet, "cuDeviceGet");
    SYM(p_cuDeviceGetName, "cuDeviceGetName");
    SYM(p_cuDeviceTotalMem, "cuDeviceTotalMem_v2");
    if (!p_cuDeviceTotalMem) SYM(p_cuDeviceTotalMem, "cuDeviceTotalMem");
    SYM(p_cuDeviceGetAttribute, "cuDeviceGetAttribute");
    SYM(p_cuCtxCreate, "cuCtxCreate_v2");
    if (!p_cuCtxCreate) SYM(p_cuCtxCreate, "cuCtxCreate");
    SYM(p_cuCtxDestroy, "cuCtxDestroy_v2");
    SYM(p_cuGetErrorString, "cuGetErrorString");

    if (!p_cuInit || !p_cuDeviceGetCount) {
        printf("PROBE_RESULT=MISSING_ENTRYPOINTS\n");
        return 2;
    }

    r = p_cuInit(0);
    printf("cuInit=%d (%s)\n", r, cuda_err(p_cuGetErrorString, r));
    if (r) { printf("PROBE_RESULT=CUINIT_FAILED\n"); return 3; }

    if (p_cuDriverGetVersion) {
        r = p_cuDriverGetVersion(&driverver);
        printf("cuDriverGetVersion=%d value=%d\n", r, driverver);
    }

    r = p_cuDeviceGetCount(&count);
    printf("cuDeviceGetCount=%d count=%d\n", r, count);
    if (r || count < 1) { printf("PROBE_RESULT=NO_DEVICE\n"); return 4; }

    r = p_cuDeviceGet(&dev, 0);
    printf("cuDeviceGet=%d dev=%d\n", r, dev);

    r = p_cuDeviceGetName(name, sizeof(name), dev);
    printf("cuDeviceGetName=%d name=%s\n", r, name);

    if (p_cuDeviceTotalMem) {
        r = p_cuDeviceTotalMem(&total, dev);
        printf("cuDeviceTotalMem=%d bytes=%llu\n", r, (unsigned long long)total);
    }
    if (p_cuDeviceGetAttribute) {
        r = p_cuDeviceGetAttribute(&major, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR, dev);
        printf("cuAttrCCMajor=%d value=%d\n", r, major);
        r = p_cuDeviceGetAttribute(&minor, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR, dev);
        printf("cuAttrCCMinor=%d value=%d\n", r, minor);
    }

    if (p_cuCtxCreate) {
        r = p_cuCtxCreate(&ctx, 0, dev);
        printf("cuCtxCreate=%d ctx=%s (%s)\n", r, ctx ? "non-null" : "null", cuda_err(p_cuGetErrorString, r));
        if (!r && ctx && p_cuCtxDestroy) {
            r = p_cuCtxDestroy(ctx);
            printf("cuCtxDestroy=%d\n", r);
        }
    }

    printf("PROBE_RESULT=OK\n");
    return 0;
}
