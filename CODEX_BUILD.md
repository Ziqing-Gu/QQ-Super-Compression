# QQ Super Compression - Build / validation brief
## v1.1.5 Stable - Fluid/Cached Dynamic Display Rendering

- Base: v1.1.4 Candidate; previous Stable rollback: v1.1.2.
- Windows x64 VST3 Release / JUCE 8.0.15 / MSVC 19.44.
- 60 Hz / 480-point approximately eight-second history, fixed projection arrays, retained path caches, sparse bounded GR shade, and opaque child rendering.
- GR math, Key Gain live projection, HPF background replay, audio DSP, parameters/state schema 10, automation IDs, and A/B remain unchanged.
- Twelve Python/source/math checks, standalone BS.1770, Steinberg validator, metadata, and build/output/install parity: PASS.
- Main binary SHA-256: `ABB9CFD1CF7929C6D4C6A7D9F72226535F36169854A1697B44550047F54B64E1`.
- Read `PLAN_A_VERIFICATION_1.1.5.md` first for Plan A evidence.
- Plan B formal source backup is complete and v1.1.5 is the current Stable baseline. Plan C/D, GitHub sync, macOS builds, and Release work remain separate at this checkpoint.

--- PREVIOUS CANDIDATE BUILD BRIEF BELOW ---

## v1.1.4 Candidate - Reliable/Faster HPF Display Replay

- Base: v1.1.3 Candidate; Stable rollback: v1.1.2.
- Windows x64 VST3 Release / JUCE 8.0.15 / MSVC 19.44.
- Latest-request retry, two-tick non-mouse debounce, visible-window pre-roll, current-domain two-engine replay, and `HPF UPDATING` status.
- Audio DSP, parameter/state schema 10, automation IDs, A/B, and real-time Key Gain behaviour are unchanged.
- Eleven Python/source/math checks, standalone BS.1770, Steinberg validator, metadata, and build/output/install parity: PASS.
- Replay-core benchmark: old about 19-26 ms; optimized about 8-13 ms, typically about 55% lower.
- Main binary SHA-256: `05A41D64AC1CA45A7EF89F34E6BE946A18E83323308A24A496C89EF381504731`.
- Read `PLAN_A_VERIFICATION_1.1.4.md` first for current Candidate evidence.
- No Plan B/C/D, Stable promotion, GitHub, Actions, macOS, or Release work is included.

--- PREVIOUS CANDIDATE BUILD BRIEF BELOW ---

## v1.1.3 Candidate - Sidechain Display History Replay

- Base and rollback: v1.1.2 Stable.
- Build target: Windows x64 VST3 Release, JUCE 8.0.15, MSVC 19.44.
- Plan A output: verified local formal VST3 bundle and ZIP; installed after Cubase was confirmed closed with build/output/install hash parity.
- Eleven Python/source/math checks, standalone BS.1770, and Steinberg vst3effectsvalidator: PASS.
- Bundle: 2 files / 6,742,107 bytes.
- Main binary SHA-256: `9B37B5C756D33D1E32E7E4982695CB5387FD89284C875D9CE99530AD7004AB59`.
- Key Gain history is real-time; HPF history replays after gesture release or a short non-mouse debounce.
- Display capture is editor-only, ten seconds, capped at 48 kHz; audio DSP and state schema 10 are unchanged.
- Read `PLAN_A_VERIFICATION_1.1.3.md` and the top of `AI_DEVELOPMENT_HANDOFF.md` for the current Candidate evidence.
- No Plan B/C/D, GitHub, Actions, macOS, Release, or Stable promotion is included.

--- CURRENT STABLE BUILD BRIEF BELOW ---


## v1.1.2 Stable - Mix-aware Dynamic Display

- Build target: Windows x64 VST3 Release, JUCE 8.0.15, MSVC 19.44.
- Plan A output: verified local formal output; machine-specific path omitted from public source.
- System install: C:\Program Files\Common Files\VST3\QQ Super Compression.vst3.
- Build/output/install parity: 2 files, 6,724,699 bytes, tree SHA-256 2D6CE2509C70C9D697756F4AD264BCB6EC284619DB2B341A5A670AE11BDF061C.
- Main binary SHA-256: 240A9DBEBDE3D88B14A59096218525204CD43B020B486E390A1042330EAA1DE3.
- Ten Python/source/math checks and standalone BS.1770 pass; validator is unavailable.
- v1.1.2 completed Plan A and Plan B and is Stable by the project standing rule. Plan C reuses the verified Windows Plan A output and manually dispatches only Apple Silicon VST3, Intel VST3, and Universal 2 AU from the exact public v1.1.2 tag. Windows Actions is retained for explicit reproduction only and is not run by Plan C; Plan D/Release remains separate.

Read AI_DEVELOPMENT_HANDOFF.md and PLAN_A_VERIFICATION_1.1.2.md first for the current Stable contract and Plan A evidence.

--- CURRENT STABLE BUILD BRIEF BELOW ---
# QQ Super Compression 1.1.1 Stable - Codex build / validation brief

## v1.1.1 Stable - Side Chain HPF

- Build target: Windows x64 VST3 Release, JUCE 8.0.15, MSVC 19.44.
- Plan A output: verified local formal output; machine-specific path omitted from public source.
- System install: C:\Program Files\Common Files\VST3\QQ Super Compression.vst3.
- Build/output/install parity: 2 files, 6,716,507 bytes, tree SHA-256 50FB3109C22DDB55E591941301C81A034CFEC097501C2639BA0E7FF9D273CFB6.
- Nine Python/source/math checks and standalone BS.1770 pass; validator is unavailable.
- Plan B source backup: verified internal formal backup; machine-specific path omitted from public source.
- v1.1.1 is the previous Stable rollback; v1.1.2 is current Stable.
- No Plan C/D, GitHub, Actions, macOS build, packaging, or Release work is part of this run.

**Stable baseline:** v1.1.0 External Key (Plan A/B complete; user-promoted Stable on 2026-09-02)
**Previous stable rollback reference:** v1.0.4 Light / Classic UI switch

Read first:

1. `AI_DEVELOPMENT_HANDOFF.md`
2. `PRODUCT_DESIGN_NOTES.md`
3. `README.md`
4. `CHANGELOG.md`
5. `DEVELOPMENT_HISTORY.md`
6. `docs/TEST_CHECKLIST.md`

## v1.1.0 Stable baseline

**Current Stable target:** `v1.1.0 — External Key` (Plan A and formal Plan B complete)

**Previous Stable rollback baseline:** `v1.0.4 — Light / Classic UI switch`

Build directly from the v1.0.4 Stable code. External Key may replace only the detector source; it must not change the carrier, future-window gain law, Threshold, Lookahead, Oversampling, PDC, Match, Display, Monitor, A/B semantics, or either theme.

Required contract:

- optional mono/stereo input bus named `Sidechain`;
- INT = exact v1.0.4 post-Input-Gain detector; EXT = sidechain after dedicated Key Gain;
- disconnected/silent EXT = zero GR, audible carrier intact;
- ST common key, LR independent key domains, stereo MS matrix, mono EXT common to both M/S;
- Key Source + Key Gain are appended APVTS parameters and A/B members; old states migrate to INT / 0 dB;
- SC Listen is non-automatable/non-persistent/non-A/B, latency-aligned, ignored by true Bypass, and resets OFF on panel/editor close/state restore;
- the same 230x146 floating panel geometry is used in LIGHT and CLASSIC; no main-layout movement.

Verified Plan A evidence inherited by Plan B:

- JUCE 8.0.15 / MSVC Windows x64 Release VST3 build with no new warnings;
- all eight Python/source/math tests plus standalone BS.1770 test pass;
- binary and module metadata show 1.1.0;
- copy the verified bundle to local formal output storage;
- install only after Cubase/DAW processes are confirmed closed;
- hash parity between build, output, and system-installed bundle;
- clearly record Steinberg validator as unavailable if it cannot be found;
- the user explicitly promoted v1.1.0 to Stable on 2026-09-02; detailed Cubase sidechain/audio/UI/PDC/old-project checks remain recorded as manual follow-up.

## Scope

Build from v1.0.4 Stable. The LIGHT / CLASSIC theme choice is visual-only. Do not alter the transparent future-window compressor, Threshold, LINK, independent Mix, Match, Oversampling, PDC, Display or v1.0.3 Monitor behaviour.

Verify the new Monitor exactly:

- LR ALL normal stereo; L/R centered at `1/sqrt(2)`.
- MS ALL normal stereo; M centered at unity; S centered at `1/sqrt(2)`.
- Display/Meters/Match stay pre-monitor and must not fall 3.01 dB when L/R/S Monitor is selected.
- Monitor hidden in ST; LR shows ALL/L/R; MS shows ALL/M/S.
- LR and MS selections restore separately after project save/reopen.
- A/B does not change Monitor; host automation list does not gain Monitor parameters.
- True Bypass remains a true bypass and ignores Monitor.

## Required validation before Stable claim

- Windows Release VST3 compile with JUCE 8.0.15; no new warnings.
- Cubase scan/load and panel shows v1.0.4.
- Listen to LR L/R centered and compare level against the established -3.01 dB centered convention.
- Confirm M is **not** 3.01 dB quieter.
- Confirm S is centered mono and compensated -3.01 dB.
- Toggle Monitor while Display/Meter are active and verify graphs/meters retain normal processing levels.
- Save/reopen project and verify independent LR/MS Monitor memory.
- Regression: all v1.0.2 LINK behaviours, future-window DSP, Threshold, Mix, PDC/Bypass, 0 ms Oversampling and Match.

Current v1.0.4 environment: JUCE/MSVC Windows x64 Release build, installed-bundle parity and seven source/math regression tests passed. Steinberg validator was unavailable and is not claimed for v1.0.4.

## Build

- VST3 / Release.
- Prefer an existing JUCE checkout with `-DJUCE_PATH=...`.
- Otherwise CMake is pinned to JUCE 8.0.15 when network access is available.
- Do not call Python/static checks a successful plug-in build.
- Plan C reuses the verified Windows x64 VST3 from the formal local Plan A output; it does not rebuild Windows in GitHub Actions by default.
- Plan C manually dispatches `.github/workflows/build-macos-vst3-au.yml` for Apple Silicon VST3, Intel x86_64 VST3 and Universal 2 AU from the confirmed public `v1.0.4` source commit/tag.

## Non-negotiable DSP baseline

v1.0.2 must be a workflow-only change from the v1.0.1 Stable audio engine:

```text
input
 -> optional 0 ms-only Oversampling (1x/8x/16x)
 -> future-window peak detector
 -> QQ Ratio + optional Threshold boundary
 -> apply gain to matching delayed sample
 -> Makeup / Mix / Output
```

There is no user Attack/Release envelope. Threshold OFF must execute the exact pre-Threshold QQ law. Lookahead intentionally controls the future-window length; the accepted microscopic pre-influence near abrupt level changes is not a regression.

## Domain parameters

ST: one Ratio / Threshold / Makeup / Mix.

LR: independent L/R Ratio / Threshold / Makeup / Mix.

MS: independent M/S Ratio / Threshold / Makeup / Mix.

ST uses the stronger exact current L/R window peak level, then calculates linked gain with the **ST** Ratio/Threshold. Hidden LR Ratio values must not affect ST.

## v1.0.2 LINK semantics

One Link state covers **Ratio, Threshold, Makeup and Mix** pairs in LR/MS.

It is **relative link**, not equality link:

- preserve the numerical difference captured at edit start;
- both controls move by the same delta;
- when either hits a boundary, both stop;
- never snap values equal merely because Link is enabled;
- Mix uses percentage-point delta (e.g. `100/70 -> -10 -> 90/60`);
- direct numeric entry must use the same shared-delta/boundary law as normal drag and Shift fine drag.

Threshold OFF is conceptual `-inf`; if exactly one threshold starts OFF, keep that OFF member outside finite-dB linking for that edit. If both are OFF, a finite direct entry may bring both out together from the same value.

## Oversampling regression

- 0 ms shows `1x / 8x / 16x`.
- 10/26/40/80/100 ms hide Oversampling and run 1x internally.
- remembered 0 ms choice must survive switching away/back.
- PDC/Dry/Mix/Bypass must include exact Oversampling FIR latency at 0 ms.

## UI regression

- Fixed design space 1020x820, uniformly scaled.
- Display/Meter row = 550 design px; current v1.0.4 lower row = 158 design px (uses former bottom slack).
- LR/MS use two stacked full-width histories.
- Display visible range = 0…-90 dB.
- Mode is a click-cycle button; Lookahead is a ComboBox. Mode and Lookahead are 108x23 and aligned; LINK is 34x23 to the right.
- Threshold Shift fine drag / Alt reset / direct entry remain functional.

## Historical v1.0.2 validation checklist (already promoted Stable)

- build VST3 with JUCE/MSVC;
- Cubase scan/open/save/restore;
- verify panel/binary metadata show v1.0.2;
- LINK Ratio drag + Shift + direct numeric entry in LR and MS;
- LINK Threshold drag + Shift + direct numeric entry, including one-OFF and both-OFF cases;
- LINK Makeup drag + Shift + direct numeric entry;
- LINK Mix drag + Shift + direct numeric entry;
- verify shared-boundary stopping preserves offsets;
- verify LINK OFF leaves all four pairs independent;
- regression: transparent core/Threshold/Lookahead/Oversampling/PDC/Display/A-B/Undo-Redo match v1.0.1 Stable;
- old project state loads without migration changes (v1.0.2 adds no parameter/state schema).

v1.0.4 validation: local JUCE 8.0.15 / MSVC Windows x64 VST3 build, installed-bundle parity and seven source/math regression tests passed. The user explicitly promoted this Plan A/Plan B revision to the current Stable baseline on 2026-09-01. Steinberg validator was unavailable for the v1.0.4 local run and remains an explicit validation gap; v1.0.3 remains the verified cross-platform rollback reference.

---

## v1.0.1 Candidate Revision 2 verification

After building, verify in Cubase/PluginDoctor:

1. Mode button is visible in ST, LR and MS and cycles ST -> MS -> LR -> ST.
2. LINK is visible in LR/MS, hidden in ST, and still links only Ratio/Threshold/Makeup relatively.
3. LR exposes separate L/R Mix; MS exposes separate M/S Mix; ST exposes one Mix.
4. In MS, setting M Mix=100% and S Mix=0% changes only the Mid processed contribution while Side remains Dry before decode.
5. Threshold faders are stacked vertically: L/M top, R/S bottom.
6. Display remains the same enlarged v1.0.1 size.
