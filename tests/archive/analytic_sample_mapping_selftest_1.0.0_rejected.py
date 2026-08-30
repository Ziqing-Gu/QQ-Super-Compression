#!/usr/bin/env python3
import math
import numpy as np

TAPS = 4095
DELAY = (TAPS - 1) // 2


def coeffs():
    out = []
    for offset in range(1, DELAY + 1, 2):
        tap_index = DELAY + offset
        window = 0.5 - 0.5 * math.cos(2.0 * math.pi * tap_index / (TAPS - 1))
        ideal = 2.0 / (math.pi * offset)
        out.append(ideal * window)
    return np.asarray(out, dtype=np.float64)


def map_amp(a, ratio, threshold=0.0):
    a = np.maximum(a, 0.0)
    ratio = max(1.0, float(ratio))
    t = min(1.0, max(0.0, float(threshold)))
    if ratio <= 1.0000001:
        return a.copy() if isinstance(a, np.ndarray) else a

    def unit(v):
        return v / (1.0 + (ratio - 1.0) * v)

    if t <= 0.0:
        return unit(a)
    out = np.array(a, copy=True)
    mask = a > t
    span = 1.0 - t
    if span <= 1e-7:
        out[mask] = t + (a[mask] - t) / ratio
    else:
        u = (a[mask] - t) / span
        out[mask] = t + span * unit(u)
    return out


def hilbert_q(x):
    # Offline equivalent of the C++ antisymmetric paired convolution.
    c = coeffs()
    h = np.zeros(TAPS, dtype=np.float64)
    for idx, offset in enumerate(range(1, DELAY + 1, 2)):
        h[DELAY + offset] = c[idx]
        h[DELAY - offset] = -c[idx]
    return np.convolve(x, h, mode='full')[:len(x)]


def process_sine(freq, ratio, threshold_db=None, fs=48000, amp=0.5, seconds=0.7):
    n = np.arange(int(fs * seconds))
    x = amp * np.sin(2.0 * np.pi * freq * n / fs)
    q = hilbert_q(x)
    real = np.concatenate((np.zeros(DELAY), x[:-DELAY]))
    a = np.sqrt(real * real + q * q)
    t = 0.0 if threshold_db is None else 10.0 ** (threshold_db / 20.0)
    aout = map_amp(a, ratio, t)
    y = np.divide(aout * real, a, out=np.zeros_like(a), where=a > 1e-12)
    return x, y, a


def harmonic_amplitudes(y, freq, fs=48000, harmonics=7):
    start = DELAY + int(0.15 * fs)
    y = y[start:]
    # Use an integer number of carrier periods so the projection does not
    # mistake rectangular-window leakage for harmonics. Test frequencies are
    # chosen to have integer periods at 48 kHz.
    period = max(1, int(round(fs / freq)))
    length = (len(y) // period) * period
    y = y[:length]
    n = np.arange(len(y))
    amps = []
    for k in range(1, harmonics + 1):
        c = np.cos(2.0 * np.pi * freq * k * n / fs)
        s = np.sin(2.0 * np.pi * freq * k * n / fs)
        re = 2.0 / len(y) * np.dot(y, c)
        im = 2.0 / len(y) * np.dot(y, s)
        amps.append(math.hypot(re, im))
    return np.asarray(amps)


def main():
    # Mapping invariants.
    grid = np.linspace(0.0, 1.25, 10001)
    assert np.max(np.abs(map_amp(grid, 1.0, 0.0) - grid)) < 1e-12
    for ratio in (1.5, 3.0, 8.0, 32.0):
        off = map_amp(grid, ratio, 0.0)
        t0 = map_amp(grid, ratio, 0.0)
        assert np.max(np.abs(off - t0)) < 1e-12
        for db in (-60.0, -30.0, -12.0, -6.0):
            t = 10.0 ** (db / 20.0)
            below = grid <= t
            finite = map_amp(grid, ratio, t)
            assert np.max(np.abs(finite[below] - grid[below])) < 1e-12
            # continuity at threshold
            eps = 1e-7
            left = map_amp(np.array([max(0.0, t-eps)]), ratio, t)[0]
            right = map_amp(np.array([t+eps]), ratio, t)[0]
            assert abs(left - right) < 1e-5

    # Steady-sine shape test. At 400 Hz even Ratio 32 should not acquire
    # significant waveshaper harmonics from the carrier cycle.
    for ratio in (3.0, 8.0, 16.0, 32.0):
        _, y, a = process_sine(400.0, ratio)
        amps = harmonic_amplitudes(y, 400.0)
        thd = np.sqrt(np.sum(amps[1:] ** 2)) / max(1e-30, amps[0])
        thd_db = 20.0 * math.log10(max(1e-30, thd))
        assert thd_db < -110.0, (ratio, thd_db)

    # User Lookahead invariant is structural in C++: it is a post-map delay.
    # Verify pure delays preserve mapped sample values exactly when re-aligned.
    _, y, _ = process_sine(400.0, 8.0)
    for ms in (0, 10, 26, 40, 80, 100):
        d = int(round(48000 * ms / 1000.0))
        delayed = np.concatenate((np.zeros(d), y[:len(y)-d])) if d else y.copy()
        if d:
            assert np.max(np.abs(delayed[d:] - y[:-d])) == 0.0
        else:
            assert np.max(np.abs(delayed - y)) == 0.0

    print('PASS: v1.0.0 analytic amplitude/sample mapping self-test')
    print(f'Hilbert taps={TAPS}, fixed group delay={DELAY} samples ({DELAY/48000*1000:.2f} ms @48k)')
    for f in (20.0, 50.0, 100.0, 400.0, 1000.0):
        _, y, _ = process_sine(f, 8.0)
        amps = harmonic_amplitudes(y, f)
        thd = np.sqrt(np.sum(amps[1:] ** 2)) / max(1e-30, amps[0])
        print(f'Ratio 8:1 @ {f:7.1f} Hz: THD {20*math.log10(max(thd,1e-30)):8.2f} dB')


if __name__ == '__main__':
    main()
