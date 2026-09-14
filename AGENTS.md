# AGENTS.md — operating rules

## Non-negotiables

1. **Do not modify Adobe binaries.** No patching in memory, no bypassing
   authentication, licensing, activation, DRM, or entitlement.
2. **Do not add application-name checks** or Adobe-specific hacks when correct
   Windows-compatible behavior can be implemented generically.
3. **Do not assume a FIXME is causal.** A FIXME is a lead, not a diagnosis.
4. **No speculative changes.** Reproduce, then minimize, then prove a concrete
   behavioral difference, then patch.
5. **Guest VMs only.** The host is off-limits. Hyper-V settings, GPU-P
   allocation, host networking, and checkpoints require explicit human approval.
6. **No credential exfiltration.** Never copy keys, tokens, or Adobe account
   data off the guests.
7. **Pin versions.** Do not upgrade Windows, NVIDIA drivers, the distro, or
   major toolchains without explicit justification and approval.

## Evidence standard

Prefer `"Windows returns X, Wine returns Y"` over
`"the application crashed around here"`.

A patch is only justified by:

1. a minimal reproducer that runs unchanged on Windows and Wine,
2. a captured, deterministic difference in observable behavior
   (HRESULT / GetLastError / status / callback sequence / output bytes),
3. a change that removes that difference without special-casing the app.

## Per-run discipline

Every experiment gets `runs/<run-id>/` with `metadata.json` holding at minimum:
timestamp, application + version, workflow/test name, Windows version, Linux
version, Wine commit, Wine Staging revision, VKD3D commit, DXVK commit,
NVIDIA Windows driver, NVIDIA Linux driver, Windows result, Wine result,
crash/exit status, log paths, suspected subsystem, hypothesis, patch commit.

Record exact git commits for every build. Never overwrite a known-good branch.
Commit each compatibility fix independently.

## Journal

Append to `notes/progress.md` after each meaningful discovery:
symptom, evidence, Windows behavior, Wine behavior, root cause, patch/test
status, next step. Do not paste routine command output.

## Escalation

Stop and ask before: modifying Hyper-V / GPU-P / host networking, reverting or
deleting checkpoints, reinstalling either OS, installing unknown kernel modules,
changing NVIDIA driver versions, touching Adobe licensing state, destructive
filesystem changes, exposing SSH externally.

If blocked for ~30 minutes without new evidence: stop, summarize knowns, list
tested and remaining hypotheses, propose the next 3 highest-information
experiments.

## Escalation is not a substitute for work

Finish every reachable step first. Reduce scope only with explicit approval.

