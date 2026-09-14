#!/usr/bin/env bun
/**
 * run_diff.mjs — differential oracle harness.
 *
 * Windows is the oracle. One probe source is cross-compiled once with mingw-w64
 * so the *identical* PE binary runs on both the Windows VM and under Wine on
 * the Linux VM. Outputs are captured per-probe and diffed.
 *
 * Usage (host, from the AdobeWinePatch directory):
 *   bun scripts/run_diff.mjs                     # build + run + diff all probes
 *   bun scripts/run_diff.mjs probe_osinfo probe_dxgi
 *   PROBES=probe_osinfo bun scripts/run_diff.mjs
 *
 * Credentials: LAB_PW env var, else `lab.env` (LAB_PW=...) next to this repo.
 *
 * Exit code: 0 when every probe agreed, 1 when at least one diverged.
 */
import { spawnSync } from "node:child_process";
import { mkdirSync, writeFileSync, readFileSync, existsSync, readdirSync } from "node:fs";
import { join, basename } from "node:path";

const HOST_DIR = new URL("..", import.meta.url).pathname.replace(/^\/([A-Za-z]:)/, "$1");
const PLINK = "C:\\Program Files\\PuTTY\\plink.exe";
const PSCP = "C:\\Program Files\\PuTTY\\pscp.exe";

const LINUX = "kubuntu@192.168.166.252";
const WINDOWS = "winnie@192.168.164.18";
const KEY_LINUX = "SHA256:ngVcxNV0+L2YFir1FkiQVO2KQY4IuYb8LlAZtYLNKMI";
const KEY_WINDOWS = "SHA256:6ynQLHkmgIaN4ywFOymAF/5gc4gaUlNTeUNgfAbBhzk";

const LAB_LINUX = "/home/kubuntu/adobe-wine-lab";
const LAB_WINDOWS = "C:\\Users\\winnie\\adobe-wine-lab";
const WINEPREFIX = process.env.LAB_PREFIX || `${LAB_LINUX}/prefix`;
const DISPLAY_ENV = "DISPLAY=:0 XAUTHORITY=/run/user/1000/xauth_qmqmif";
/* Which wine to exercise: the distro package by default, or a fork build. */
const WINE_BIN = process.env.LAB_WINE || "wine";

function password() {
  if (process.env.LAB_PW) return process.env.LAB_PW;
  const f = join(HOST_DIR, "lab.env");
  if (existsSync(f)) {
    const m = readFileSync(f, "utf8").match(/^LAB_PW=(.*)$/m);
    if (m) return m[1].trim();
  }
  throw new Error("No password: set LAB_PW or create lab.env with LAB_PW=<password>");
}
const PW = password();

function plink(host, key, cmd, timeoutMs = 300000) {
  const p = spawnSync(PLINK, ["-ssh", "-batch", "-hostkey", key, "-pw", PW, host, cmd],
    { encoding: "utf8", timeout: timeoutMs, maxBuffer: 64 * 1024 * 1024 });
  return { rc: p.status ?? -1, out: p.stdout ?? "", err: p.stderr ?? "" };
}
function pscp(key, local, remote) {
  const p = spawnSync(PSCP, ["-batch", "-hostkey", key, "-pw", PW, local, remote],
    { encoding: "utf8", timeout: 300000 });
  return { rc: p.status ?? -1, out: p.stdout ?? "", err: p.stderr ?? "" };
}
const lin = (c, t) => plink(LINUX, KEY_LINUX, c, t);
const win = (c, t) => plink(WINDOWS, KEY_WINDOWS, c, t);

/** Run a probe on Windows; cmd must be a quoted absolute path. */
function runWindows(exe) {
  return win(`"${exe}"`, 120000);
}
/** Run a probe under Wine. */
function runWine(exe) {
  /* NB: do not override mshtml here. mshtml is both the registrant of the
   * MIME content-type database and the handler for text/html, so disabling it
   * masks the very behaviour these probes measure. */
  const cmd = `cd ${LAB_LINUX}/tests/build/probes && WINEPREFIX=${WINEPREFIX} WINEDEBUG=-all ` +
    `${DISPLAY_ENV} ${WINE_BIN} ${exe}`;
  return lin(cmd, 180000);
}

/* Link libraries required by each probe. */
const PROBE_LIBS = {
  probe_com: "-lole32 -loleaut32 -luuid -ladvapi32",
  probe_crypto: "-lcrypt32 -lwintrust -ladvapi32 -lole32",
  probe_directwrite: "-ldwrite -lole32 -luuid",
  probe_dxgi: "-ldxgi -ld3d11 -lole32 -luuid",
  probe_mf: "-lmfplat -lmfuuid -lole32 -luuid",
  probe_osinfo: "-ladvapi32",
  probe_registry: "-ladvapi32",
  probe_reggetvalue: "-ladvapi32",
  probe_winhttp: "-lwinhttp -lwininet -lole32",
  probe_gpu: "",
  probe_fs: "-ladvapi32",
  probe_expand: "",
  probe_webview2: "-lole32 -ladvapi32",
  probe_wv2_controller: "-lole32 -ladvapi32 -luuid -lgdi32 -luser32",
  probe_mime: "-lole32 -ladvapi32 -luuid",
  probe_window: "-luser32 -lgdi32",
  probe_metrics: "-luser32 -lgdi32",
  probe_webbrowser: "-lole32 -loleaut32 -luuid -luser32 -lgdi32",
  probe_dcomp: "-lole32 -luuid",
  probe_fls: "",
};

/* Probes that must be built 32-bit to match the software under test.
 * The Creative Cloud bootstrapper is a PE32 i386 image. */
const PROBE_ARCH = {
  probe_webview2: "i686",
  probe_mime: "i686",
  probe_webbrowser: "i686",
  probe_wv2_controller: "i686",
};
const MINGW = {
  i686: { cc: "i686-w64-mingw32-gcc", dir: "i686-windows" },
  x86_64: { cc: "x86_64-w64-mingw32-gcc", dir: "x86_64-windows" },
};

const args = process.argv.slice(2);
const probeDir = join(HOST_DIR, "tests", "probes");
let probes = args.length
  ? args
  : process.env.PROBES
    ? process.env.PROBES.split(/\s+/)
    : readdirSync(probeDir).filter((f) => f.endsWith(".c")).map((f) => basename(f, ".c"));

if (!probes.length) {
  console.error(`No probes found in ${probeDir}`);
  process.exit(2);
}

// --- run id ---------------------------------------------------------------
const now = new Date();
const pad = (n) => String(n).padStart(2, "0");
const runId = `${now.getFullYear()}${pad(now.getMonth() + 1)}${pad(now.getDate())}-${pad(now.getHours())}${pad(now.getMinutes())}${pad(now.getSeconds())}-diff`;
const runDir = join(HOST_DIR, "runs", runId);
for (const d of ["windows", "linux", "comparison"]) mkdirSync(join(runDir, d), { recursive: true });

console.log(`run ${runId}`);
console.log(`probes: ${probes.join(", ")}`);

// --- build once on Linux, deploy to both --------------------------------
mkdirSync(join(HOST_DIR, "artifacts"), { recursive: true });
win(`if not exist "${LAB_WINDOWS}\\tests\\build\\probes" mkdir "${LAB_WINDOWS}\\tests\\build\\probes"`, 60000);
const built = [];
const failed = [];
for (const p of probes) {
  const src = `tests/probes/${p}.c`;
  if (!existsSync(join(HOST_DIR, src))) { console.error(`skip ${p}: ${src} missing`); failed.push(p); continue; }
  let r = lin(`mkdir -p ${LAB_LINUX}/tests/probes ${LAB_LINUX}/tests/build/probes`, 30000);
  r = pscp(KEY_LINUX, join(HOST_DIR, src), `${LINUX}:${LAB_LINUX}/tests/probes/${p}.c`);
  if (r.rc !== 0) { console.error(`push ${p} failed: ${r.err.trim()}`); failed.push(p); continue; }
  const mingw = MINGW[PROBE_ARCH[p] ?? "x86_64"];
  r = lin(`cd ${LAB_LINUX}/tests/probes && ${mingw.cc} -O2 -Wall -o ../build/probes/${p}.exe ${p}.c ${PROBE_LIBS[p] ?? ""} && echo OK`, 180000);
  if (!r.out.includes("OK")) { console.error(`build ${p} failed:\n${r.out}${r.err}`); failed.push(p); continue; }
  r = pscp(KEY_LINUX, `${LINUX}:${LAB_LINUX}/tests/build/probes/${p}.exe`, join(HOST_DIR, "artifacts", `${p}.exe`));
  if (r.rc !== 0) { console.error(`pull ${p}.exe failed: ${r.err.trim()}`); failed.push(p); continue; }
  r = pscp(KEY_WINDOWS, join(HOST_DIR, "artifacts", `${p}.exe`), `${WINDOWS}:${LAB_WINDOWS}\\tests\\build\\probes\\${p}.exe`);
  if (r.rc !== 0) { console.error(`deploy ${p}.exe failed: ${r.err.trim()}`); failed.push(p); continue; }
  built.push(p);
}
if (!built.length) {
  console.error(`no probe could be built/deployed (${failed.length} failed)`);
  process.exit(2);
}

// --- execute on both, capture, diff --------------------------------------
const results = [];
for (const p of built) {
  const w = runWindows(`${LAB_WINDOWS}\\tests\\build\\probes\\${p}.exe`);
  const l = runWine(`${p}.exe`);
  writeFileSync(join(runDir, "windows", `${p}.out`), `exit=${w.rc}\n${w.out}`);
  writeFileSync(join(runDir, "linux", `${p}.out`), `exit=${l.rc}\n${l.out}`);

  const norm = (s) => s.replace(/\r\n/g, "\n").split("\n")
    // Channels that legitimately differ by environment rather than behaviour:
    //  - MODULE= is the absolute path of the binary
    //  - OSVERSION= is the Windows version Wine is configured to report
    //  - the SystemRoot spelling/case comes from the prefix configuration
    .map((x) => x
      .replace(/^MODULE=.*$/, "MODULE=<path>")
      .replace(/^OSVERSION=.*$/, "OSVERSION=<v>")
      .replace(/[Cc]:[\\/][Ww][Ii][Nn][Dd][Oo][Ww][Ss]/g, "<sysroot>"))
    .filter((x) => x.length);
  const wn = norm(w.out), ln = norm(l.out);
  const keys = new Set([...wn, ...ln].map((x) => x.split("=")[0]));
  const diffs = [];
  for (const k of [...keys].sort()) {
    const wv = wn.find((x) => x.startsWith(k + "=")) ?? "(absent)";
    const lv = ln.find((x) => x.startsWith(k + "=")) ?? "(absent)";
    if (wv !== lv) diffs.push(`  ${k}\n    windows: ${wv}\n    wine:    ${lv}`);
  }
  results.push({ probe: p, winExit: w.rc, wineExit: l.rc, diffs });
  console.log(`\n=== ${p} === windows exit=${w.rc} wine exit=${l.rc} divergences=${diffs.length}`);
  if (diffs.length) console.log(diffs.join("\n"));
}

// --- report ---------------------------------------------------------------
const total = results.reduce((n, r) => n + r.diffs.length, 0);
const lines = [
  `# differential report ${runId}`,
  ``,
  `probes: ${results.map((r) => r.probe).join(", ")}`,
  `total divergences: ${total}`,
  ``,
];
for (const r of results) {
  lines.push(`## ${r.probe}`, ``, `exit: windows=${r.winExit} wine=${r.wineExit}`, ``);
  if (!r.diffs.length) lines.push(`no divergence`, ``);
  else lines.push("```", ...r.diffs, "```", ``);
}
writeFileSync(join(runDir, "comparison", "diff.md"), lines.join("\n"));

const meta = {
  run_id: runId, timestamp: now.toISOString(), kind: "differential-probe",
  probes: results.map((r) => r.probe),
  failed_probes: failed,
  results: results.map((r) => ({ probe: r.probe, windows_exit: r.winExit, wine_exit: r.wineExit, divergences: r.diffs.length })),
  binaries: "mingw-w64 13-win32 x86_64, single PE run on both targets",
  outputs: { windows: "windows/*.out", linux: "linux/*.out", diff: "comparison/diff.md" },
};
writeFileSync(join(runDir, "metadata.json"), JSON.stringify(meta, null, 2) + "\n");

console.log(`\nreport: runs/${runId}/comparison/diff.md`);
console.log(`total divergences: ${total}`);
process.exit(total === 0 ? (failed.length ? 2 : 0) : 1);
