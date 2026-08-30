# QQ Super Compression 1.0.1 Plan D release checklist

## Cumulative gates

- [ ] Plan A: Windows x64 Release VST3 builds with pinned JUCE 8.0.15.
- [ ] Plan A: source manifest, BS.1770 self-test, DLL/VST3 entry points, moduleinfo, and Steinberg validator pass.
- [ ] Plan A: installed Windows VST3 hash matches the verified output.
- [ ] Plan B: refreshed formal source snapshot under the user's Vibe Coding backup tree has manifest-backed parity.
- [ ] Plan C: public GitHub repository is synchronized at the intended release commit.
- [ ] Plan D: Windows and macOS GitHub Actions jobs pass and their artifacts are downloaded.

## Required archives

- [ ] `Win/QQ-Super-Compression-1.0.1-Windows-x64.zip`
- [ ] `Mac/QQ-Super-Compression-1.0.1-macOS-Apple-Silicon-VST3.zip`
- [ ] `Mac/QQ-Super-Compression-1.0.1-macOS-Intel-VST3.zip`
- [ ] `Mac/QQ-Super-Compression-1.0.1-macOS-Universal-AU.zip`

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

Checksums, workflow logs, build directories, architecture proofs, and other internal evidence belong in a separate `InternalVerification/` sibling folder, never inside the end-user folder.

## Stable baseline boundary

Under the user's standing rule, completing Plan D promotes this exact 1.0.1 Display 0…-90 dB Scale Polish source and its same-commit artifacts to the Stable baseline. Cubase listening, UI inspection, CPU, PDC, Mix/Bypass alignment, automation, and state-migration checks remain explicit manual follow-up items rather than unperformed automated claims.
