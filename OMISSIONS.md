# OMISSIONS — what is NOT in this repo and why

Deliberate exclusions. Nothing below is needed to apply the patches or run
the probes; each entry says how to reproduce the omitted material.

## Secrets and credentials — NEVER included

- `lab.env` (`LAB_PW=…`), `VM login credentials.txt`, SSH keys
  (`~/.ssh/kubuntu_ai`, `keypairs/hermes1_openssh`), GitHub tokens.
- `${LAB_PW}` placeholders stand in where run-scripts pipe the lab password
  to `sudo -S` (6 files under `run-scripts/`, `scripts/linux/`). Export
  `LAB_PW` in your own environment; nothing here reads a committed secret
  (`scripts/lab.mjs`, `scripts/run_diff.mjs` read `LAB_PW` env or local
  `lab.env`, which is git-ignored).
- No Adobe account data, tokens, or key material anywhere (verified by scan:
  no `gho_`/tokens, no email addresses, no `LAB_PW=` values).

## Adobe binaries and runtime blobs

- Installers (`Creative_Cloud_Set-Up.exe`,
  `Creative_Cloud_Set-Up_747d.exe`, `setup.exe`, `setup_packed.exe`,
  `unpacked.exe`), WebView2 runtime (`msedgewebview2.exe`,
  `WebView2Loader_x86.dll`, `webview2/153.0.4234.32/`, 679 MB runtime tar),
  `wine-gecko-2.47.4-*.msi`, compiled probe `.exe`s, `tests/build/`.
  Bring your own Adobe download; reference SHA256 in `README.md`.
- `reproducers/creative-cloud/` binaries excluded for the same reason.

## Machine-scale state (GBs, not portable)

- Wine sources (`src/wine`, `src/wine-staging`, `src/wine-staging-work`,
  `src/mesa` — 16 GB) and mirrors (`src-mirror/`, `src-pristine/`):
  re-clone at the commits in `README.md` and apply `patches/` in order.
- Built trees (`install/wine-upstream`, `install/wine-arch` — 4.7 GB),
  all prefixes (`prefix*` — 1–5 GB each), `gecko/`, `work-unpack/`,
  `staging/`. Recreate per `README.md` + `scripts/linux/`.
- `runs/` (50+ timestamped differential runs) and `artifacts/` (76+
  screenshots incl. `cc-*.png`, `oracle-*.png`): raw logs and pixels, not
  needed for the patches. Representative verdicts are quoted in
  `notes/progress.md` and `STATUS.md`.

## Logs with third-party / account-adjacent data

- `evidence/cc-install-oracle/` (`WAM.log`, `dunamis.log`,
  `NGLClient_*.ngllogcontrolconfig`): installer telemetry samples incl.
  `apiKey=ccinstaller-service`. Excluded; behavior they evidence is
  described in `notes/progress.md`.
- Oracle ProcMon captures (`cc.pml` 4 GB + `cc-1.pml` 1.8 GB), timeline
  screenshots, `/tmp/*.out` traces: cited, not shipped.

## Lab-local network facts

- `LAB_STATUS.md`, `LAB_README.md`, `AGENTS.md`, `inventory/` name
  Hyper-V NAT addresses (`192.168.164.18`, `192.168.166.252`) and SSH host
  key fingerprints. These change across reboots — re-resolve in your own
  lab; connection method (`plink -hostkey …`) is documented, secrets are not.

## What WAS kept despite age

- `LAB_STATUS.md` §§6–7 are superseded by §9 (noted in the file header);
  kept verbatim as the lab record rather than rewritten.
- `notes/progress.md` is the full journal incl. refuted hypotheses — kept
  whole so dead ends stay dead.
