# Plan C version coverage — QQ Super Compression 1.0.3

Date: 2026-08-30

- Remote last fully recorded version: `1.0.2`
- Evidence: remote `main` at `844a78e9e8b81093bc524a65680dd7c9c5fb8a65`, tag `v1.0.2` at `c6495c492513cd1bf0260a619150deed1bd49446`, README and CHANGELOG directly record 1.0.2.
- Target version: `1.0.3`
- Real versions after 1.0.2 through target: `1.0.3`
- README directly recorded versions for the interval: `1.0.3`
- CHANGELOG/development-history versions for the interval: `1.0.3`
- README formal Release links: intentionally remain on the real public `v1.0.2` Release until a separate Plan G run.
- Missing versions / 遗漏版本: None / 无

## 1.0.3 bilingual summary / 双语概要

- 中文：从 1.0.2 Stable 新增 LR ALL/L/R 与 MS ALL/M/S 居中试听。L/R/S 复制到双输出时采用 1/sqrt(2)（-3.0103 dB）补偿，M 维持 unity；Monitor 仅作用于最终试听输出，Display/Meter/Match 保持 Monitor 前信号。LR/MS 状态分别随工程保存，排除在 A/B 与宿主自动化之外。Windows Plan A 已通过；Plan D 跨平台验证待完成。
- English: Built from 1.0.2 Stable, adds centered LR ALL/L/R and MS ALL/M/S audition. L/R/S use 1/sqrt(2) (-3.0103 dB) compensation when copied to both outputs while M remains unity; Monitor affects only the final audition output and Display/Meter/Match remain pre-monitor. LR/MS states persist independently with the project and stay outside A/B and host automation. Windows Plan A passed; Plan D cross-platform verification remains pending.