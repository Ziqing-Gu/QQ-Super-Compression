# QQ Super Compression 1.2.14 — Verification Notes

## Intended contract

Both Dynamic Display and the dedicated GAIN +/- meter represent compressor gain change after Mix plus actual time-dependent OutputCeiling attenuation. Hold follows the same combined meter value.

## Local checks in this package

- Source audit checks the shared Display/meter composition and preserves audible OutputCeiling equations.
- Strict 1:1 and continuity isolated tests remain available.
- Windows `revision1211` includes a real processor test comparing TP OFF vs TP ON on a reconstruction-sensitive alternating waveform and requires TP attenuation in GAIN +/-, Hold and Display telemetry.

## Not run here

Windows JUCE/MSVC build, real VST3 host playback and Cubase visual confirmation must still be run by the user.
