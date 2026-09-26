# QQ Super Compression 1.2.2 — Ratio 1000 and Gain Link

Windows x64 VST3 Stable build based on 1.2.0 Stable, retaining its dynamics algorithm and Lookahead.

- Single Ratio: 1:1000 through 1:1 to 1000:1; unity remains centred.
- Dual Up Ratio: 1:1000 to 1:1. Dual Down Ratio: 1:1 to 1000:1.
- All Ratios default to 1:1. Every setting is finite; there is no Infinity mode. Numeric input accepts `1:1000`, `1/1000` and `1000:1`.

The original relative Dual Ratio LINK is restored. Starting at unity, Down=2 pairs with Up=1/2, reaching Up=1/1000 and Down=1000 together. Starting with Up=1/4 and Down=8, doubling Down to 16 makes Up=1/8, preserving the existing relationship. Enabling LINK never snaps values; both stop at their shared travel limit. Ratio LINK defaults ON and remembers the last user selection.

A small LINK button above and to the right of Input links Input and Output by equal, opposite dB changes: Input +3 dB moves Output -3 dB, and either knob can lead. Existing offsets are retained and enabling LINK causes no immediate gain change. Both stop when either reaches its ±24 dB limit. It defaults OFF, saves with the project, and is shared across A/B. LR/MS uses a smaller button.

1:1000 is a Ratio setting, not a fixed 1000-fold boost. Actual gain depends on detector level, thresholds and Mix. The upward gate, Dual handoff to Down, finite Range, threshold collision rules, branch crossfades and themes are retained. Mix, Display and numeric gain readings support the extended gain; the gain bar retains its existing fixed scale.

Close audio hosts and copy the complete `Win/QQ Super Compression.vst3` bundle to `C:\Program Files\Common Files\VST3`, replacing the old version. Rescan/reload and check v1.2.2.

For a listening test use Single, Threshold −inf, Range OFF, Ratio 1:1000, Lookahead 26 ms, Mix 100%, all trims at 0 dB, and quiet material. For Dual use Up Threshold −inf, Down Threshold 0 dB, Up Ratio 1:1000 and Down Ratio 1:1; disable Ratio LINK before setting independent values.

Saved absolute parameter values are retained. Both upward and downward normalized Ratio scales have changed, so existing Ratio automation positions map to different values. Test automated projects using a copy. Input/Output LINK follows deliberate editor gestures; state recall and host automation do not cause extra companion writes.

The original manuals are included unchanged; their Ratio ranges are superseded by this guide. Corresponding source and original license accompany the package. The user promoted this build to Stable on2026-09-26 and requested the formal Plan B backup. This promotion does not publish a GitHub Release.
