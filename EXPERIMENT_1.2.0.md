# QQ Super Compression 1.2.0 Rev4 — approved Stable specification

User promoted this revision to Stable on 2026-09-10. The historical filename is
retained for existing links. See `STABLE_1.2.0.md` for the Plan B checkpoint.

2026-09-10. Windows x64, JUCE 8.0.15. Plug-in version remains 1.2.0;
state schema is 14. Rev3 corrected Rev2's Dual UP direction; Rev4 adds boundary continuity and independent branch enables.
Source: `D:\Codex\Workspaces\QQSuperCompression-1.2.0-UpDown`.
Output: `D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev4 Candidate`.

## Rev4 additions

Rev4 is now the accepted Stable revision. The previous Rev3 hold is resolved.
Finite Single downward Range blends the retained gain to unity inside the
interval: width=min(upper/2,(upper-lower)/2), amount=smoothstep((upper-p)/width),
gain=1+(oldGain-1)*amount, with the smoothstep argument clamped to [0,1].
A finite upward gate uses width=min(lower,(anchor-lower)/2) and
gain=1+(oldGain-1)*smoothstep((p-lower)/width). At/below the gate and at/above
Single Range, gain is unity. Range OFF retains the exact original downward law.
These static transitions add no detector window, Attack/Release or latency.

Dual UP/DOWN enables are appended automatable bools in all five domains, default
ON, saved with projects and A/B, with legacy missing values ON. Each branch gain
crossfades to/from unity over 10ms at the internal sample rate. Reversals continue
from the current fade; 8x/16x oversampling retains the host-time duration. Ratio
values and LINK relationships are preserved. Small adjacent buttons and the
Display projection follow the enabled state. State schema is 14.

## Confirmed processing definition

- Single: process only above Threshold and below a finite Range. At/below
  Threshold or at/above finite Range, dynamic gain is unity. Range defaults to
  OFF, an independent unbounded endpoint; saved finite Range0 remains finite.
- Dual: UP is a lower enabling gate. At/below UP, leave the audio unchanged.
  Above UP and below DOWN, raise eligible material. At DOWN, gain is unity.
  Above DOWN, stop upward processing and use Down Ratio only. The two gain
  branches do not overlap or cascade. UP=-inf opens the gate for nonzero audio;
  it does not disable upward processing.
- New Dual thresholds remain UP=-inf, DOWN=0dB. All 15 Ratio defaults (Single,
  UP, DOWN across ST/LR/MS) are now1:1. Existing saved Ratio numbers are retained.
- Single Ratio range1/32..32; Dual UP1/32..1; Dual DOWN1..32. Boundaries push
  their partner on collision. Equal boundaries disable the entire dynamic stage.
- Keep the approved future-window peak/lookahead detector, delayed carrier,
  0ms-only oversampling and the Range-OFF downward law. No attack/release was added.

## Gain law shared by audio and Display

Let p be the linear detector peak, T the lower gate and A the upward anchor.
Upward gain inside the eligible region is `1 / (r + (1-r)*p/A)`, with r<=1.
For Single, A is finite Range or1 when Range is OFF. For Dual, A is DOWN;
UP is only the lower gate. Gain tends to unity as p reaches A and is bounded
by1/r. A detector10dB below A at Ratio1:8 receives +7.92198dB, provided p>T.

The retained downward law is `(1+(r-1)*T)/(1+(r-1)*p)` above its threshold.
Single uses Threshold as T; Dual uses DOWN. The existing detector caps p at1.
Do not restore Rev2's `p<UP` activation or use UP as Dual's upper anchor.

## Ratio LINK

- A small LINK between the UP/DOWN columns starts ON on first use. ST uses
  36x17 at(278,644); LR/MS uses30x14 at(281,636). It is hidden in Single.
- Coupling preserves each pair's product UP*DOWN. If DOWN changes D0->D1,
  UP becomes U0*D0/D1, and conversely for UP edits. Starting4 and1/2, changing
  DOWN to8 produces UP1/4, not1/8. Toggling LINK never changes either ratio.
- Shared range limits stop the edit before any linked member exceeds its
  range; reversing resumes travel. Drag, fine drag, Alt reset and numeric
  commit share this rule. A linked Alt reset also respects the shared limits.
- The existing LR/MS domain LINK remains separate: its driven branch retains
  the additive relative delta, while each UP/DOWN pair preserves its own product.
- Explicit editor edits record host gestures for all affected parameters.
  Host automation and state restores load independent values without recoupling.
- The last user click is stored in the existing UI settings for new instances.
  Project state includes dualRatioLink and takes precedence on restoration.
  Reopening an editor keeps its instance state. LINK is excluded from sound A/B.

## Layout and illumination

- In ST/Single, the five primary dials use identical94x94 logical drawing areas
  and centres80,296,512,728,944 (216px spacing), at the same height. All three
  themes use this common geometry; paired LR/MS and Dual ratios remain smaller.
- SINGLE/DUAL moves to(380,620,60,21), closer to Ratio. Full-length boundary
  rails and shared Display coordinates from Rev2 remain unchanged.
- Single Ratio lights outward from1:1 at the centre; upward ratios light left,
  downward ratios right. Dual UP lights from the right endpoint toward the left;
  Dual DOWN retains left-to-right lighting. Neutral has no active arc and keeps
  its position indicator. Renderer caches include the light origin, so switching
  modes refreshes the arc even when the normalised position is unchanged.
- Preserve480-point history, bounded signed shading and one final Output path.

## Validation and scope

See `VERIFICATION_REV4_1.2.0.md` for current DSP, state, editor and rendering
results, including finite-boundary continuity and independent branch fades.
The preceding base checks are retained in `VERIFICATION_1.2.0.md`. Preview images
use synthetic history/meter fixtures for layout checks. The existing 0ms colouring
mode is separate from the lookahead transparency reference.
Old normalised Single Ratio automation has a different mapping from1.1.9's
narrower range. Experiment in new instances and retain the1.1.9 rollback.

The1.1.9 workspace, frozen backups and multiband project are untouched. Rev2
source checkpoint: `D:\Codex\Archives\QQSuperCompression-1.2.0-Rev2-before-Rev3-20260910`.
This revision is user-promoted Stable. After the original frozen Plan B, the user
approved the Chinese manual and requested the English edition, then Plan C and
Plan D. See RELEASE_NOTES_1.2.0.md for the public release definition. The original
frozen checkpoint remains untouched; release-bound changes use a new snapshot.
