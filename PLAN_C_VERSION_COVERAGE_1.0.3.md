# Plan C version coverage - QQ Super Compression 1.0.3

Date: 2026-08-30

- Remote last fully recorded version before this release line: `1.0.2`.
- Target version: `1.0.3`.
- Real versions after 1.0.2 through target: `1.0.3`.
- README, CHANGELOG, development history, installation guides, Actions workflow names, CMake version, and source manifest all directly record 1.0.3.
- Candidate source sync: `7504914e458abdff4bfcc0dd4a83ff96bf500e8d`.
- Final build-source correction: `0ef89e19dd7e74ec5588f64501a951c8487efb9c` enables the declared AU format; tag `v1.0.3` points exactly to this commit.
- Actions passed from that tag/source commit: Windows x64 VST3, macOS Apple Silicon VST3, macOS Intel VST3, and Universal 2 AU with `auval`.
- README formal Release links intentionally remain on the real public `v1.0.2` Release until a separate Plan G run.
- Missing versions / 遗漏版本: None / 无.

## 1.0.3 bilingual summary / 双语概要

- 中文：基于 1.0.2 Stable 新增 LR ALL/L/R 与 MS ALL/M/S 居中试听。L/R/S 复制到双输出时采用 1/sqrt(2)（-3.0103 dB）补偿，M 维持 unity；Monitor 仅作用于最终试听输出，Display/Meter/Match 保持 Monitor 前信号。LR/MS 状态分别随工程保存，排除在 A/B 与宿主自动化之外。Plan A/B/C/D 已完成，1.0.3 是当前稳定基线。
- English: Built from 1.0.2 Stable, adds centered LR ALL/L/R and MS ALL/M/S audition. L/R/S use 1/sqrt(2) (-3.0103 dB) compensation when copied to both outputs while M remains unity; Monitor affects only the final audition output and Display/Meter/Match remain pre-monitor. LR/MS states persist independently with the project and stay outside A/B and host automation. Plan A/B/C/D are complete and 1.0.3 is the current stable baseline.
