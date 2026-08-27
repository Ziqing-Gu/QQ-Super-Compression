# Knob Assets

`QQSC_MainKnob_128Frames_256_VERTICAL.png` is the runtime 128-frame rotary filmstrip introduced in v0.9.2.

Frame N is located at source rectangle:

```text
x = 0
y = N * 256
width = 256
height = 256
```

N is `round(normalizedValue * 127)`.

`QQSC_MainKnob_128Frames_PREVIEW.jpg` and `QQSC_MainKnob_KEYFRAMES.jpg` are inspection/reference files only and are not embedded into the plug-in.

Numeric values and units are deliberately NOT part of the PNG. See `UI_ASSET_ARCHITECTURE.md`.
