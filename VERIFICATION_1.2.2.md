# Stable promotion — 2026-09-26

The user accepted this exact verified build as Stable. The checks below describe
the retained candidate validation, not a second test run. All production files
match the original delivered source ZIP and build/output/installed bundle hashes
match. Promotion edits release records only. See STABLE_1.2.2.md for the new status.
The historical desktop ZIP was absent at the promotion precheck; the D-drive ZIP
remains verified. No desktop regeneration is required for this Plan B.

--- Earlier records below are historical ---

# QQ Super Compression 1.2.2 local verification

Status: local listening candidate; no Stable designation, Plan B/C/D or online release.

Based on 1.2.0 Stable. Original product name, VST3 class IDs, Lookahead detector,
static dynamics curves, finite Range transitions and branch fades retained.
Single Ratio 0.001..1000, Dual Up 0.001..1, Dual Down 1..1000. All default to1.
Original relative Ratio LINK restored, including shared bounds and remembered preference.
Input/Output LINK defaults OFF, preserves dB sum during either control's gestures,
and persists with project state outside the A/B sound banks. State schema16.
JUCE attachment re-entry at linked limits was corrected so knob and DSP values agree.

Validation:
- Release VST3, QQSCDynamicsCheck and QQSCVisualCheck compiled successfully with JUCE8.0.15/MSVC.
- Actual processor regression passed at 44.1/48/96 kHz, varied block sizes and ST/LR/MS.
- Single/Dual upward 1:1000: -100 dBFS steady sine produces -40.0863 dBFS,
  +59.9137 dB measured gain. Sampled harmonics H2-H7 below -110 dBc in this fixture.
- Actual finite 1000:1 downward audio, host text/parameters and state restore passed.
- Existing Range-crossing and Dual branch-crossfade regression checks passed.
- Actual editor text entry, native rotary drag, Alt reset, Ratio shared bounds,
  reciprocal/offset pair linkage and host gestures passed.
- Input/Output opposite edits, shared bounds, host gestures, toggle/no-jump,
  independent automation, ST/LR/MS, project/A-B/legacy restore checks passed.
- Three production themes rendered offline at compact size; user settings untouched.
- 13 inherited Python checks passed. Two obsolete skin source-string checks remain
  excluded after known failures on untouched1.2.0; runtime geometry/pixel checks passed.
- No independent Steinberg validator run is claimed. This does not establish all-signal
  distortion immunity or true-peak limiting.

The original bilingual PDFs are retained byte-for-byte. START_1.2.2_CN/EN.md documents
the new range, link and old normalized-automation implications. Existing absolute
parameter values are retained; normalized Ratio automation is rescaled.
See adjacent logs and DELIVERY.json/INSTALL_AND_HANDOFF.json for hashes and installation proof.
