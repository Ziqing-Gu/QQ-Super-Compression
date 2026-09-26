# QQ Super Compression 1.2.3 Stable

The user accepted the QQ dB Compression test algorithm as the new Super Compression. This build restores the original formal name and VST3 identity, replaces QQ Super Compression1.2.2 and removes the separate QQ dB Compression test plugin from the system installation.

## Installation and use

Close audio hosts. Install the entire `QQ Super Compression.vst3` folder from Win into `C:\Program Files\Common Files\VST3`, replacing the previous bundle. Rescan and select **QQ Super Compression**; its editor should read **v1.2.3**. Do not install by renaming a binary.

Existing Super Compression projects retain plugin recognition, parameter values and original preferences. Finite thresholds now use the accepted new law, so those project settings change sound; -inf retains the agreed legacy fallback. Projects using the separate QQ dB Compression identity need that instance replaced with the formal Super Compression plugin.

**Use a finite Threshold for the new law. -inf still selects the compatibility branch. All Ratio defaults remain1:1.** To verify, use Single, Threshold-10dB, Ratio5:1, RangeOFF, Mix100%, Lookahead26ms and all other gains0dB: a steady0dB signal produces-8dB, with sub-threshold material unchanged.

## Changed transfer law

Finite Down uses a fixed dB ratio: `output level = threshold + (detector level - threshold) / Ratio`. At a -10 dB threshold and 5:1, a steady 0 dB detector level produces -8 dB output; -12 dB remains unchanged. This describes Range OFF without additional gain; a finite Range modifies the curve near its upper boundary to return smoothly to unity.

Up still affects only material above its lower enabling gate. To preserve reciprocal symmetry, it uses the same dB slope as reciprocal Down, referenced to unity at the upper boundary: `boost dB = (upper boundary dB - detector level dB) * (1 - Up Ratio)`. The upper boundary is finite Single Range, 0 dB for Single Range OFF, or Dual DOWN threshold. Away from boundary transitions, 8:1 and 1:8 match after a constant gain offset in their common active interval. This is not simply a sign inversion of Down gain.

Single restores 0 dB dynamic gain at or above a finite Range. Dual stops Up at DOWN threshold, then Down takes over. Existing gate/Range transitions, 10 ms branch-switch crossfades, lookahead detector and latency remain. No Attack/Release envelope was added. Thresholds apply to the detected envelope, not each carrier sample; lookahead still creates pre-influence near abrupt changes.

The -inf fallback applies separately: a -inf Down threshold uses legacy Down; a -inf Up gate uses legacy Up. Dual with UP at -inf and a finite DOWN therefore combines legacy Up with new Down.

Ratio ranges remain Single 1:1000 through 1000:1, Up 1:1000 through 1:1 and Down 1:1 through 1000:1. Finite dB Up can lift more than the original curve: boost depends on dB distance to the upper boundary and is no longer restricted by the old 60 dB limit. This version supports up to 120 dB in its gain and display calculations. Adjust deep gates against the actual material and meters; the existing fixed plot scale is unchanged.

## Retained controls

Single/Dual, ST/LR/MS, three skins, relative Ratio Link, opposite Input/Output Link, branch switches, A/B, MATCH, sidechain and Mix retain their 1.2.2 operation. The original PDFs are included unchanged as control references; this guide takes precedence for the current transfer law.

This is the user-approved 1.2.3 Stable algorithm. Plan C/D release packaging includes Windows x64 VST3, native Apple Silicon and Intel VST3, Universal 2 AU, bilingual installation guides and the unchanged original PDFs. See the release and installation guides for platform requirements. Complete corresponding source is published under the Qing Audio non-commercial source-share license.
