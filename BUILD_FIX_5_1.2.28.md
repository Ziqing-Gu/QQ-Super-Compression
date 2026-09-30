# QQ Super Compression 1.2.28 — Build Fix 5

Build Fix 5 changes tests only. No files under `Source/` are modified.

## Why Build Fix 4 still failed

The Dual-algorithm final-output independence regression ran Limiter mode with the default TP Ceiling enabled. TP is deliberately stateful: during a sine wave's startup the detector first crosses the Up region, so different Up laws can produce different early peaks and therefore different TP recovery state. The later Down-region audio can consequently differ for a finite time even though the compressor's Down branch itself is independent. Build Fix 4 exposed this as ~0.0067 RMS in Limiter/ST while all Normal cases and direct branch-law projections were exactly independent.

## Fix

- Limiter cases in `dual_algorithm_checks.inc` now explicitly select TP OFF.
- The test still traverses the 1.2.28 Limiter-only fixed 8x Ceiling pipeline, using its stateless 8x Hard Clipper endpoint.
- The final-audio branch-isolation tolerance is restored from the temporary 1e-4 diagnostic guard to the original strict 1e-5.
- TP/Recovery history remains covered by the dedicated Ceiling / TP Recovery regressions.
- No DSP, latency, Ceiling, Display or processor source is changed.
