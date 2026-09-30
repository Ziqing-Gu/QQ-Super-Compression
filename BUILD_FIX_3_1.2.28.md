# QQ Super Compression 1.2.28 - Build Fix 3

- No DSP change.
- Updates the remaining 1.2.27-era visual regression assumption in `strict_one_to_one_link_checks.inc`.
- TP OFF in 1.2.28 is an 8x oversampled Hard Clipper, so reconstructed inter-sample overshoot is expected to produce positive Ceiling GR even when native samples are at 0 dBFS.
- The regression now requires both Hard Clip and TP branches to expose their real Ceiling attenuation consistently in GAIN +/-, Hold, and Dynamic Display telemetry.
- Build cache auto-clean behavior from Build Fix 2 is retained.
- Refreshes one legacy Display Python selftest version guard from 1.2.3 to 1.2.28; its rendering assertions are otherwise unchanged.
