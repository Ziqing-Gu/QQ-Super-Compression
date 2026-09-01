# QQ Super Compression 1.0.4 - Plan A verification record

Date: 2026-09-01  
Status: Stable baseline, explicitly confirmed by the user on 2026-09-01  
Previous Stable rollback reference: 1.0.3

## Scope

- Added an upper-right `LIGHT` / `CLASSIC` visual-theme switch.
- Preserved the existing 1020 x 820 layout and all plug-in functions.
- Stored the last selected theme in local UI settings and restored it on later editor opens.
- Kept theme state outside DSP, APVTS, host automation, A/B snapshots and project state.

## Windows verification inherited by Plan B

- JUCE/MSVC Release VST3 build: passed.
- Installed bundle parity with the Release build: passed.
- Installed path: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`
- Windows x64 VST3 binary SHA-256: `3197F0710D5A6D7E3C43896A254A862ED046A9E46775B063BFEF14A565D8E698`
- Regression tests: transparent core, threshold rebuild, domain link, LINK UI, independent Mix, display scale and monitor audition all passed.
- Steinberg validator was unavailable in the local environment and remains an explicitly recorded validation gap; the user nevertheless promoted this exact Plan A/Plan B source and binary to the Stable baseline.

## Manual update included before Plan B

- Preserved every original page of the English and Chinese v1.0.1 manuals.
- Appended one v1.0.4 UI-theme page to each manual.
- The new page documents theme switching, visual-only equivalence and last-theme restoration, using the supplied v1.0.4 CLASSIC screenshot.

## License

The project remains under `Qing Audio Non-Commercial Source-Share License 1.0` (`LicenseRef-Qing-Audio-NC-Source-Share-1.0`). This is source-available, non-commercial software and is not represented as OSI-approved open source.
