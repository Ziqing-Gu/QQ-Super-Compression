# QQ Super Compression 1.2.17 — Build Fix 1

This build fix changes **test code only**. Product `Source/` files are unchanged from the original 1.2.17 candidate.

## Windows failure fixed

The first `revision1217` regression directly accessed private members of `DynamicDisplay` (`HistoryPoint`, `histories`, `clearHistories`, `refreshRenderCaches`, and `renderCaches`) from `QQSCLimiterCheck`, which MSVC correctly rejected with C2248.

Build Fix 1 keeps the product encapsulation unchanged. The limiter regression now uses the already-authorized test friend `QQSCVisualCheck` as a test-only accessor, copies only the projected blue/output values needed by the assertion, and leaves all product APIs private.

The default Windows build directory is now `D:\Codex\Temp\QQSC1217-BF1-Build` to avoid reusing the failed CMake cache.
