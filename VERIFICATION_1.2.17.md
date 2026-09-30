# QQ Super Compression 1.2.17 — Verification Notes

## Product contract

Dynamic Display is retrospective: changing current parameters re-projects the full visible history. Limiter history no longer uses captured `measuredOutputDb` or captured Ceiling GR as rendered results. A small per-history inter-sample excess descriptor lets TP ON/OFF also participate in current-parameter reprojection without adding another oversampler.

Limiter Display:

```text
current pre-Ceiling projection = historical input/detector evidence + CURRENT parameters
current TP peak projection     = pre-Ceiling projection + stored inter-sample excess (TP ON only)
current post-Ceiling projection= current Ceiling rule applied retrospectively
blue                            = post-Ceiling projection - current fixed Output/Ceiling shift
orange                          = post-Ceiling projection
```

Therefore blue and orange have the same contour point-for-point while final level controls keep their expected vertical offset. The dedicated GAIN +/- meter is unchanged from 1.2.15 Build Fix 1 and continues to use the actual realtime OutputCeiling envelope, not the retrospective display approximation.

Normal Input and Output are both +/-24 dB. Limiter Input remains its bank setting and Limiter Output remains +/-120 dB. State/A-B restore clamps only the Normal I/O values to the new range.

## Local checks

- `python tests/revision1217_source_audit.py`
- Existing strict 1:1 Link and Limiter continuity isolated tests remain unchanged and should be rerun.

## Windows acceptance

`BUILD_WINDOWS.cmd` runs `revision1211`, `revision1217`, `continuity`, `dual`, then `QQSCCeilingCheck`. In Cubase verify that changing Ratio/Threshold/Mix/Makeup/Input/Output/algorithm/TP re-shapes already-visible history rather than only new points.
