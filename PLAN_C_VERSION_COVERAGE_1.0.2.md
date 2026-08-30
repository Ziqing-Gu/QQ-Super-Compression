# Plan C version coverage — QQ Super Compression 1.0.2

Date: 2026-08-30

- Remote repository: `Ziqing-Gu/QQ-Super-Compression` (Public)
- Default branch: `main`
- Last fully recorded remote version: `1.0.1`
- Evidence: remote `main` and tag `v1.0.1` at commit `a191d39a7472c46a4eb3c233a7f3b31add6cb10c`; README and CHANGELOG directly record 1.0.1.
- Target version: `1.0.2`
- Real versions after 1.0.1 through the target: `1.0.2`
- README directly recorded versions for this interval: `1.0.2`
- CHANGELOG/handoff directly recorded versions for this interval: `1.0.2`
- Release-body coverage: not applicable to Plan C/D; Plan G was not requested.
- README Release links: intentionally remain on the real 0.9.4 public Release until a separate Plan G run.
- Missing versions / 遗漏版本: None / 无

## 1.0.2 bilingual summary / 双语概要

- 中文：1.0.2 直接基于 1.0.1 Stable，补全 LR/MS Relative LINK，使 Ratio、Threshold、Makeup 与 Mix 在拖动、Shift 微调和直接数值输入时都保持相对差值并共享边界停止。DSP、参数与 state schema 不变。本机 Windows 构建、自测、Steinberg validator、安装与 SHA-256 一致性已通过；Cubase 最终交互仍由用户复核。
- English: 1.0.2 is built directly from 1.0.1 Stable and completes LR/MS Relative LINK across Ratio, Threshold, Makeup, and Mix for drag, Shift-fine adjustment, and direct numeric entry, with offset preservation and shared-boundary stopping. DSP, parameters, and the state schema are unchanged. Local Windows build, self-tests, Steinberg validator, installation, and SHA-256 parity passed; final Cubase interaction remains user-side verification.
