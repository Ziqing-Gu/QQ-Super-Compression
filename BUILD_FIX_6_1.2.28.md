# QQ Super Compression 1.2.28 — Build Fix 6

This build fix changes tests only. Product DSP/UI source is unchanged from Build Fix 5.

## Fix

`tests/dual_algorithm_checks.inc` previously called `baseline()` before entering Limiter. `baseline()` pins the Normal bank to 26 ms lookahead, but Limiter owns an independent `limiterLookaheadMs` parameter whose new-instance default comes from the user's saved last-lookahead preference. On a machine where that preference is 0 ms, the 400 Hz sine used by the Dual branch-isolation test legitimately crosses the Up/Down regions every cycle; changing the Up algorithm therefore changes the rendered RMS even though branch selection is correct.

Build Fix 6 explicitly pins the ACTIVE bank's lookahead to 26 ms after Limiter entry. Limiter tests still run through TP OFF / 8x Hard Clip and retain the original strict 1e-5 final-audio branch-isolation tolerance.

The `DUAL-INDEPENDENCE` diagnostic now prints `lookahead=26ms` so the test condition is visible in Windows build logs.
