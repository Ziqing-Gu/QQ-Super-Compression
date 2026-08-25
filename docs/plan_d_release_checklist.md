# QQ Super Compression 0.1.8 Plan D release checklist

## Cumulative gates

- [ ] Plan A: Windows Release build succeeds.
- [ ] Plan A: BS.1770 self-test and VST3 factory-load smoke test pass.
- [ ] Plan A: installed Windows VST3 hash matches the verified output.
- [ ] Plan B: refreshed formal source snapshot has manifest-backed parity.
- [ ] Plan C: public GitHub repository is synchronized at the release commit.
- [ ] Plan C: Windows and macOS GitHub Actions runs pass.

## Required archives

- [ ] `Win/QQ-Super-Compression-0.1.8-Windows-x64.zip`
- [ ] `Mac/QQ-Super-Compression-0.1.8-macOS-Apple-Silicon-VST3.zip`
- [ ] `Mac/QQ-Super-Compression-0.1.8-macOS-Intel-VST3.zip`
- [ ] `Mac/QQ-Super-Compression-0.1.8-macOS-Universal-AU.zip`

## macOS checks

- [ ] Apple Silicon VST3 binary reports `arm64`.
- [ ] Intel VST3 binary reports `x86_64`.
- [ ] Universal AU binary reports both `arm64` and `x86_64`.
- [ ] `auval -v aufx Qscp Qing` passes in GitHub Actions.

## End-user folder layout

The final end-user folder contains only:

- Chinese installation guide at the root.
- English installation guide at the root.
- `Win/` with exactly one Windows archive.
- `Mac/` with exactly three macOS archives.

Checksums, workflow logs, build directories and internal proof files belong in a
separate `InternalVerification/` sibling folder, never inside the end-user folder.
