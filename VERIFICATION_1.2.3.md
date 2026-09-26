# QQ Super Compression1.2.3 Stable verification

The user-approved dB comparison algorithm has been promoted under the original formal Super Compression identity. All mathematical production files are byte-identical to the accepted comparison source. Other production edits are restricted to product branding/preferences/state variant metadata. Original VST3 CIDs are restored; schema17 remains compatible with existing parameter state.

Built Release VST3, actual processor regression console and offscreen editor renderer with MSVC2022/JUCE8.0.15.

- Fixed Down tested at thresholds-10/-30/-90/-110dB with Ratios2/5/6.75/8/1000. Input0dB at threshold-10/Ratio5 produces-8dB. Below-threshold material stays unity.
- Actual Single/Dual ST/LR/M/S processing checked at44.1/48/96kHz with17/256/1024-sample blocks. Lookahead26ms/PDC unchanged. 64/511-sample output parity passed.
- -inf retained rational-family audio verified at Ratios.001/.125/1/8/1000 and multiple levels.
- Reciprocal8/.125 common-interior output nulls below1e-7 sample error after35dB compensation, for threshold-40dB/upper0dB. Gate/Range transition edges are outside this symmetry claim.
- Finite Up at gate-110/input-100/Ratio.125 produces+87.5dB, with matching meter and Display. Silence stays zero.
- Deep Down at threshold-110/input0/Ratio1000 produces-109.89dB with accurate audio, Mix and meters in Single/Dual ST/LR/M/S at0/25/100% Mix. Weighted sums avoid cancellation, and meter conversion retains gains below-100dB.
- Steady400Hz test fixtures have measured H2-H7 sum below-110dBc. No all-signal or all-frequency distortion claim is made.
- Finite Range/gate edge continuity, collision semantics, outside unity, branch enable state and interrupted10ms crossfades passed. 0ms/8x/16x branch fade duration passed.
- Range-crossing600Hz carrier at3Hz modulation agrees with an independent double-precision future-peak/continuous-curve reference within2e-6 sample error. This replaces a legacy absolute gain-step bound which was specific to its shallower curve.
- Actual UI knob-to-DSP changes, relative Ratio Link/offsets/shared limits, I/O opposite Link, A/B/project/legacy restore, all three skins and ST/LR/MS passed. Test-only preference injection used; no user settings written.
- Fixed-dB Display tested against0->-8dB, subthreshold unity, +87.5dB Up and-109.89dB Down. Six new formal-name screenshots rendered with synthetic detector history.


This is a fresh build and processor/editor validation for the formal identity. Logs and install parity proof are in the output Verification directory. No independent Steinberg validator or live DAW test was run. The accepted algorithm is unchanged; historical Python formula models are not the current transfer-law oracle.

Installation replaces the old Super Compression binary and removes the independent dB test installation, with verified rollback copies on D. Installation completion is recorded in Verification/INSTALLATION.json. Plan B completion is certified only by the external COMPLETE record linked from STABLE_1.2.3.md; no Plan C/D or online publication is claimed.

Original approved bilingual PDF manuals remain byte-identical. Current1.2.3 supplemental guides describe the accepted curve and old-project behavior.
