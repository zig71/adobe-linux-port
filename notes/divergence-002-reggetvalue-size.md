# Divergence #2 — RegGetValue size reporting for string values

Status: **CLOSED.** Fixed in
`patches/0002-kernelbase-Report-the-value-size-RegGetValue-uses-no.patch`.

## The rule (oracle-verified)

`RegGetValueA`/`RegGetValueW` report the size of the buffer they worked from,
not the size of the data they return. In every formula below:

- `R` is the value's stored size, in bytes, as `RegQueryValueEx` reports it.
- `needs_nul` is 1 when `R > 0` and the last stored byte is not already a
  terminator, else 0.
- `S = R + needs_nul` — the size once a terminator is ensured.
- `E` is the length in characters of the expanded string, excluding its
  terminator.

**Non-expanded values** (`REG_SZ`, `REG_MULTI_SZ`, or `REG_EXPAND_SZ` with
`RRF_NOEXPAND`):

    returned in the caller's buffer : cbData = max(S, 1)
    size-only query (pvData == NULL) : cbData = R ? S + k : 1

where `k` is 2 for `REG_MULTI_SZ` (the terminating empty string) and 1
otherwise.

**Expanded values** (`REG_EXPAND_SZ` without `RRF_NOEXPAND`):

    cbData = max(E + 1, S)          in both forms
    cbData = max(E + 1, S) + 1      when E + 1 > S, i.e. the expansion grew

`*pdwType` and `*pcbData` are filled even when the call fails.

## Evidence

Same mingw-w64-built PE run on both targets; pvData NULL, `*pcbData` zeroed.

`runs/20260911-224950-diff/` — 11 rows over `REG_SZ`, `REG_MULTI_SZ`,
`REG_EXPAND_SZ` with and without `RRF_NOEXPAND`, expansions longer/shorter/equal:

| case | R | E | Win buf | Win nobuf |
|---|---|---|---|---|
| `REG_SZ` len 0 (`\0` stored) | 1 | – | 1 | 2 |
| `REG_SZ` len 0 (0 bytes stored) | 0 | – | 1 | 1 |
| `REG_SZ` len 1 | 2 | – | 2 | 3 |
| `REG_SZ` len 10 | 11 | – | 11 | 12 |
| `REG_SZ` len 10, no NUL stored | 10 | – | 11 | 12 |
| `REG_MULTI_SZ` `a\0\0` | 4 | – | 4 | 6 |
| `REG_MULTI_SZ` `a\0b\0\0` | 6 | – | 6 | 8 |
| `REG_EXPAND_SZ` `%SystemRoot%` expanded | 13 | 10 | 13 | 13 |
| `REG_EXPAND_SZ` `%NOPE%` expanded | 7 | 6 | 7 | 7 |
| `REG_EXPAND_SZ` grown expansion | 21 | 21 | 35 | 36 |
| `%SystemRoot%` `RRF_NOEXPAND` | 13 | – | 13 | 14 |
| `%LABTEMP%\aaaaaaaaaa` no NUL, `RRF_NOEXPAND` | 25 | – | 27 | 28 |

`runs/20260911-225158-diff/` (`probe_rgv_raw.c`) swept `E - R_chars` over
−3…+3 with and without a stored terminator, confirming `S` is what dominates and
that the stored terminator does not shift the result.

`runs/20260911-225239-diff/` (`probe_rgv_more.c`) swept the caller's buffer size
around the boundary and showed the `ERROR_MORE_DATA` threshold itself is
**unchanged** — cap = `E + 1` for an expanded value, cap = `S` otherwise. Only
the number reported differs.

## Refuted hypotheses (kept for the record)

- **H1** "Windows reports needed + 2, or + 3 when expanding" — refuted by the
  25-character case, where Windows reported `raw + 1` regardless of the
  expanded length.
- **H2** "Windows always reports at least as much as Wine" — refuted by
  `%NOPE%`-expanded, where Windows reported 7 and (old) Wine 8.
- **H3** "the extra size is a constant added for expansion" — refuted once the
  terminator-appending cases (`R = 10` no NUL → 12) were separated from the
  already-terminated ones (`R = 11` → 12).

The curve-fit model that other candidate rules degenerated into was resolved by
identifying the real quantity: `S`, the size of the buffer Windows reads the raw
value into.

## Root cause

Wine recomputed `cbData` from the *returned* data (`ExpandEnvironmentStrings`
return length, or the stored size) where Windows reports the size of the raw
buffer it read. Wine also never added the slack Windows reserves for the
terminator it may have to append. A second defect surfaced while fixing this:
`ExpandEnvironmentStringsA` returns **needed + 1** whenever the destination
cannot take the result (a documented quirk Wine faithfully reproduces — see
`probe_expand`, 0 divergences), so its return value must not be used directly as
the required size.

## Changes

`dlls/kernelbase/registry.c`:

- capture `cbRaw`, the stored size, immediately after the initial
  `RegQueryValueEx` call in both entry points;
- expanded path: take `max(E + 1, S)`, and size the `ERROR_MORE_DATA` test from
  `E + 1` rather than the possibly-larger value;
- A variant: normalise `ExpandEnvironmentStringsA`'s return (`n - 1` when the
  destination could not take the result);
- after the string handling, when the value could not be returned in the
  caller's buffer, add one character of terminator slack (two for
  `REG_MULTI_SZ`) for non-expanded values, and one only when the expansion grew;
  a value with no data at all gets none.

`dlls/advapi32/tests/registry.c`:

- new `test_get_value_size()` pinning the whole matrix for both entry points;
- `TP1_SZ` size-only expectation corrected to `strlen + 2`;
- three stale `todo_wine` markers removed, their expectations are now met;
- `test_string_termination` / `test_multistring_termination`: the
  size-only expectations corrected from `insize + 1` to `insize + 2`.

## Verification

| probe | before | after |
|---|---|---|
| `probe_reggetvalue` | 33 | **0** |
| `probe_rgv_size` | 10 | **0** |
| `probe_rgv_slack` | 11 | **0** |
| `probe_rgv_slack2` | 11 | **0** |
| `probe_rgv_raw` | 8 | **0** |
| `probe_rgv_more` | 2 | **0** |
| `probe_rgv_inner` | 1 | **0** |
| `probe_expand` | 0 | 0 |

`advapi32:registry` conformance: **19 failures (1 inside a `todo_wine` block) →
18 failures (0 inside todo)**, the 18 being the pre-existing `KEY_WOW64_*` view
failures of this 64-bit-only build, unrelated to this change and present before
it.
