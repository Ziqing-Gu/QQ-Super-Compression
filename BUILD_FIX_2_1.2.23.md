# QQ Super Compression 1.2.23 Build Fix 2

This is a **test/build-only correction**. Product `Source/` is byte-identical to 1.2.23 Build Fix 1.

The previous Windows regression tried to disable Dual Ratio LINK by writing the APVTS parameter before the first editor was opened. For a genuinely new instance, the editor intentionally applies the saved last-choice Dual Ratio LINK preference on first open, so that raw pre-editor write was overwritten and the absolute 1:8 UI-range test accidentally ran with LINK enabled.

Build Fix 2 initialises the new-instance Dual Ratio LINK preference to OFF before constructing the editor, then explicitly verifies that both the processor parameter and UI button are OFF before testing numeric entry.

No DSP, parameter range, TP, Display, Link, Classic/Super, state or UI product code changed.

Default Windows build directory: `D:\Codex\Temp\QQSC1223-BF2-Build`.
