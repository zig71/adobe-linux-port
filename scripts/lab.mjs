/**
 * lab.mjs — SSH/SCP helpers for the Adobe Wine lab.
 *
 * Import from an eval cell so the helpers survive kernel resets:
 *   const L = await import("./scripts/lab.mjs");
 *   L.lin("hostname"); L.winps("..."); L.pushLin(local, remote);
 */
import { spawnSync } from "node:child_process";
import { readFileSync, existsSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

export const PLINK = "C:\\Program Files\\PuTTY\\plink.exe";
export const PSCP = "C:\\Program Files\\PuTTY\\pscp.exe";

export const LINUX = "kubuntu@192.168.166.252";
export const WINDOWS = "winnie@192.168.164.18";
export const KEY_LINUX = "SHA256:ngVcxNV0+L2YFir1FkiQVO2KQY4IuYb8LlAZtYLNKMI";
export const KEY_WINDOWS = "SHA256:6ynQLHkmgIaN4ywFOymAF/5gc4gaUlNTeUNgfAbBhzk";

export const LAB_LINUX = "/home/kubuntu/adobe-wine-lab";
export const LAB_WINDOWS = "C:\\Users\\winnie\\adobe-wine-lab";
export const DISPLAY_ENV = "DISPLAY=:0 XAUTHORITY=/run/user/1000/xauth_qmqmif";
export const FORK_WINE = `${LAB_LINUX}/install/wine-upstream/bin/wine`;

const HOST_DIR = dirname(dirname(fileURLToPath(import.meta.url)));

function password() {
  if (process.env.LAB_PW) return process.env.LAB_PW;
  const f = join(HOST_DIR, "lab.env");
  if (existsSync(f)) {
    const m = readFileSync(f, "utf8").match(/^LAB_PW=(.*)$/m);
    if (m) return m[1].trim();
  }
  throw new Error("Lab password missing: set LAB_PW or lab.env");
}
const PW = password();

export function plink(host, key, cmd, timeoutSec = 300) {
  const p = spawnSync(PLINK, ["-ssh", "-batch", "-hostkey", key, "-pw", PW, host, cmd],
    { encoding: "utf8", timeout: timeoutSec * 1000, maxBuffer: 64 * 1024 * 1024 });
  return { rc: p.status ?? -1, out: p.stdout ?? "", err: p.stderr ?? "" };
}
export function pscp(key, local, remote, timeoutSec = 300) {
  const p = spawnSync(PSCP, ["-batch", "-hostkey", key, "-pw", PW, local, remote],
    { encoding: "utf8", timeout: timeoutSec * 1000 });
  return { rc: p.status ?? -1, out: p.stdout ?? "", err: p.stderr ?? "" };
}
export function pscpDown(key, remote, local, timeoutSec = 300) {
  const p = spawnSync(PSCP, ["-batch", "-hostkey", key, "-pw", PW, remote, local],
    { encoding: "utf8", timeout: timeoutSec * 1000 });
  return { rc: p.status ?? -1, out: p.stdout ?? "", err: p.stderr ?? "" };
}

export const lin = (c, t) => plink(LINUX, KEY_LINUX, c, t);
export const win = (c, t) => plink(WINDOWS, KEY_WINDOWS, c, t);
/** Run a PowerShell script string on the Windows oracle. */
export function winps(script, t = 300) {
  const enc = Buffer.from(script, "utf16le").toString("base64");
  return plink(WINDOWS, KEY_WINDOWS, "powershell -NoProfile -ExecutionPolicy Bypass -EncodedCommand " + enc, t);
}
export const winfile = (p, t = 300) =>
  plink(WINDOWS, KEY_WINDOWS, `powershell -NoProfile -ExecutionPolicy Bypass -File "${p}"`, t);
export const pushLin = (l, r) => pscp(KEY_LINUX, l, LINUX + ":" + r);
export const pushWin = (l, r) => pscp(KEY_WINDOWS, l, WINDOWS + ":" + r);
export const pullLin = (r, l) => pscpDown(KEY_LINUX, LINUX + ":" + r, l);
export const pullWin = (r, l) => pscpDown(KEY_WINDOWS, WINDOWS + ":" + r, l);
