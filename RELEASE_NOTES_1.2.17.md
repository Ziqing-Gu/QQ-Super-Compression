# QQ Super Compression 1.2.17

- Restores the Dynamic Display's defining retrospective projection model. Historical points store evidence/shape metadata rather than captured rendered Output/TP results.
- Limiter Display re-applies current Input, detector, dynamics, Mix, Makeup, Output, Ceiling and TP selection to the whole visible history.
- TP uses an 8x pre-Ceiling inter-sample excess descriptor captured without extra oversampling; toggling TP reprojects existing history immediately.
- In Limiter mode, blue is the current post-Dynamics/Mix/Makeup/TP projection before the final fixed Output/Ceiling shift; orange is the same contour after that shift.
- The exact realtime GAIN +/- meter and Hold continue to include actual OutputCeiling attenuation.
- Normal (non-Limiter) Input Gain and Output Gain are both +/-24 dB. Limiter Output remains +/-120 dB.
- No audible compressor/OutputCeiling DSP law was changed.
