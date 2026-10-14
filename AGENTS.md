# QQ Super Compression maintenance rules

Read docs/BEHAVIOR_CONTRACTS.md and the current README before changing behavior. User instructions override historical tests and notes. Preserve unconditional Limiter Output LINK and the ST -> LR -> MS cycle.

Every product change delivered after 1.4.1 must receive the next numeric version in CMake, UI and all plugin metadata. Do not introduce a mandatory Candidate stage. Only the user may choose Stable. Documentation-only updates, packaging and publication of an unchanged product do not require a version bump or rebuild. Completed Plan stages are reused; frozen backups must not be read or updated.

Enable QQSC_BUILD_DYNAMICS_CHECK and run QQSCContractCheck for behavior changes. Do not weaken assertions or recapture reference audio to conceal failures. Keep macOS Chinese text UTF-8-safe. Preserve other contributors' changes and fetch/integrate the current remote before publishing.
