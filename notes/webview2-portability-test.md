# Can the Windows WebView2 runtime be brought over to Linux? — tested

**Short answer: the files transfer and the binaries load under Wine, and it
closes the discovery gap completely — but the runtime cannot initialise. It
faults immediately at its entry point.**

This was tested end to end rather than reasoned about, because the intuitive
answer ("it's just files, copy them") turned out to be wrong in an interesting
way: the copying works, the loading works, and the failure is one layer deeper.

## What was done

The whole Edge WebView2 runtime was copied from the Windows oracle to the Linux
target.

| | |
|---|---|
| version | `153.0.4234.32` |
| size | 860 MB on disk, 680 MB transferred as an uncompressed tar |
| transfer | `tar` on Windows (ntfs-3g, preserves everything) → `pscp` → `tar -x` |
| staging | symlinked into the Wine prefix at the path the registry advertises |

`Compress-Archive` silently produced a 0-byte file, so `tar.exe` was used
instead. No file was lost: the extracted tree is byte-complete.

## What works

**The binaries load.** Using `probe_loadlib.c`, which reports the Win32 error on
failure:

| module | architecture | result under Wine |
|---|---|---|
| `msedge.dll` (Chromium core, 332 MB) | x86_64 | **loads, error 0** |
| `msedge_elf.dll` (allocator shim) | x86_64 | **loads, error 0** |
| `EmbeddedBrowserWebView.dll` (host shim) | i386 | **loads, error 0** |

That is a genuinely good result: the 332 MB Chromium core maps successfully.

> A false negative was nearly recorded here. `msedge.dll` first failed with
> `ERROR_MOD_NOT_FOUND (126)` and Wine's loader named the culprit as
> `msedge_elf.dll` — its own sibling. That was an artefact of the probe sitting
> one directory above the runtime: dependency modules resolve from the
> **executable's** directory, not the loaded DLL's. Running from inside the
> runtime, or calling `SetDllDirectoryW` first, fixes it. Worth knowing for any
> future attempt.

**The discovery gap closes.** The installer's own probe
(`probe_webview2.c`) now reports results **identical to the Windows oracle**:

```
HKLM_default_open=found (0)              RUNTIME_DIR0_version_dirs=1
HKLM_default_pv=153.0.4234.32            msedgewebview2_exe=present
HKLM_default_pv_type=1                   RUNTIME_DIR0_first_version=153.0.4234.32
```

This required staging the runtime **and** writing the registration a normal
Windows machine has:

```
HKLM\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-...}
    pv       = 153.0.4234.32
    name     = Microsoft Edge WebView2 Runtime
    location = C:\Program Files (x86)\Microsoft\EdgeWebView\Application
```

That key is written by the WebView2 runtime's own installer on Windows, so
reproducing it is reproducing a normal machine's state, not an application-
specific workaround. Divergence #5's registry finding is therefore **resolved**.

## What fails

The installer does not use the public loader. Analysing the unpacked
bootstrapper:

```
CreateWebViewEnvironmentWithOptionsInternal    PRESENT
WebView2Loader                                 absent
CreateCoreWebView2EnvironmentWithOptions       absent
EmbeddedBrowserWebView                         absent
msedgewebview2                                 absent
```

So its actual path is:

1. read `EdgeUpdate\Clients\{F3017226-...}` → `pv`, `location` — **works on Wine**
2. `LoadLibraryW("<location>\<pv>\EBWebView\x86\EmbeddedBrowserWebView.dll")`
   — **works on Wine**, and the export resolves
3. `CreateWebViewEnvironmentWithOptionsInternal(...)` — **faults**

`probe_wv2_internal.c` replicates exactly that sequence. Under Wine:

```
STEP1_open=0x00000000 (found)
STEP1_pv=153.0.4234.32
STEP2_load_x86_ok=1 err=0
STEP2_export_present=1
STEP3_CoInitializeEx=0x00000000
wine: Unhandled page fault on execute access to 00000000 at address 00000000
```

Exception detail:

```
code=c0000005 (EXCEPTION_ACCESS_VIOLATION)
info[0]=00000008   (execute access)
info[1]=00000000   (address 0)
eip=00000000
```

The instruction pointer is **zero**: Chromium's host calls through a function
pointer that was never populated and jumps to NULL. This is not a crash after
partial work — it dies before the environment exists. Corroborating: the
installer never creates a WebView2 user-data directory, and it falls back to
mshtml (`fixme:mshtml:ActiveScriptSite_OnScriptError` appears, and the
`webview2` directory Windows creates is absent).

## Why, beyond the immediate fault

`msedgewebview2.exe` is **x64-only** (machine `0x8664`) and cannot be run
standalone — it expects `--initial-client-data`, a blob of Mojo IPC handles, so
it must be launched by a client that has already established the channel. Its
strings show the machinery it needs: `mojo`, `network.mojom.NetworkService`,
`storage.mojom.StorageService`, `IsSandboxedProcess`,
`embedded-browser-webview`, `user-data-dir`.

So hosting the real runtime is not "load three DLLs". The chain is:

1. create named-pipe-backed Mojo channels with the right security descriptors
2. spawn an x64 helper process and hand it those handles
3. run Chromium's sandbox model in whatever form Wine permits
4. service shared-memory and compositing between them

Step 1 is where the NULL call occurs. Wine lacks the full surface Chromium
probes for at startup.

## Verdict

| | |
|---|---|
| copy the runtime across | **works** |
| load its binaries under Wine | **works** |
| satisfy the installer's runtime probe | **works** (matches Windows exactly) |
| initialise a WebView2 environment | **fails — immediate NULL call** |
| therefore: installer takes its WebView2 path | **no** |

Redistribution is also a question worth flagging: the runtime is Microsoft's
proprietary binary, and shipping it inside a Wine patch is not something that
could go upstream. It is fine as a local experiment; it is not a path to a
mergeable fix.

## Recommendation

Make WebView2 a first-class item with its own plan, rather than an incidental
step. The two viable directions:

- **Implement the surface in Wine** — `WebView2Loader.dll` plus the
  `ICoreWebView2*` interfaces, backed by Wine's existing Gecko (the same engine
  mshtml already drives successfully, as `probe_webbrowser.c` shows reaching
  `READYSTATE_COMPLETE`). Open, upstreamable, and useful to every WebView2
  application. Large, but the pieces already exist in Wine.
- **Keep the runtime-under-Wine idea alive as a research track** — worth one
  bounded investigation into which specific capability the NULL call is missing,
  because if it were a small number of unimplemented APIs that would change the
  calculus considerably.

What is *not* worth doing is proceeding on the assumption that copying the files
is sufficient. It is now measured that it is not.

## Files

- `tests/probes/probe_loadlib.c` — per-module load test with accurate errors
- `tests/probes/probe_wv2_internal.c` — replicates the installer's exact path
- `scripts/pe_imports.py` — PE import lister (no loader needed)
- `remote/lin_test_wv2_e2e.sh` — staging + registration + end-to-end run
- Runtime staged at `/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32`
