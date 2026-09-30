# QQ Super Compression 1.2.11 — Strict 1:1 Limiter Link Candidate

Base: user-verified 1.2.9 source. The rejected 1.2.10 algorithm-link candidate is not the development base.

Limiter LINK is simplified to an exact dB relationship:
- DOWN Threshold -1 dB -> Output Gain +1 dB; +1 dB -> Output -1 dB.
- Makeup +1 dB -> Output Gain -1 dB; -1 dB -> Output +1 dB.
- Direct Output Gain edits move the active DOWN Threshold by the same dB amount in the opposite direction.
- Ratio, Mix, Classic/Super algorithm selection, Input, UP gate, channel/mode and branch switches never move Output Gain.

Preserved from 1.2.9:
- Normal/Limiter banks are independent from first use.
- Normal Makeup range ±30 dB; Limiter Makeup range ±120 dB.
- Limiter Single/Dual switch carries the current DOWN Threshold and preserves Makeup/Mix/Output regardless of LINK.
- Normal Single/Dual threshold memories remain independent.

The full-reference Link calculation remains available internally for MATCH/calibration helpers only; ordinary Limiter LINK controls no longer use it.
