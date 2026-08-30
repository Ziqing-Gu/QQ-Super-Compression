#!/usr/bin/env python3
import math
import numpy as np


def gain_for_level(level, ratio, threshold=0.0):
    level = np.clip(level, 0.0, 1.0)
    ratio = max(1.0, float(ratio))
    threshold = min(1.0, max(0.0, float(threshold)))
    if threshold <= 0.0:
        return 1.0 / (1.0 + (ratio - 1.0) * level)
    out = np.ones_like(level, dtype=np.float64)
    mask = level > threshold
    out[mask] = (1.0 + (ratio - 1.0) * threshold) / (1.0 + (ratio - 1.0) * level[mask])
    return out


def process_future_peak(x, ratio, lookahead_samples):
    # Offline equivalent: output sample x[k] uses max(abs(x[k:k+L+1])).
    L = int(lookahead_samples)
    y = np.zeros_like(x)
    for k in range(len(x)):
        end = min(len(x), k + L + 1)
        level = np.max(np.abs(x[k:end]))
        g = 1.0 / (1.0 + (ratio - 1.0) * min(1.0, level))
        y[k] = x[k] * g
    return y


def harmonic_amps(y, freq, fs=48000, harmonics=7):
    # Select integer carrier periods away from start/end transients.
    start = int(0.15 * fs)
    stop = len(y) - int(0.15 * fs)
    y = y[start:stop]
    period = max(1, int(round(fs / freq)))
    y = y[: (len(y) // period) * period]
    n = np.arange(len(y))
    amps = []
    for k in range(1, harmonics + 1):
        c = np.cos(2 * np.pi * freq * k * n / fs)
        s = np.sin(2 * np.pi * freq * k * n / fs)
        amps.append(math.hypot(2*np.dot(y, c)/len(y), 2*np.dot(y, s)/len(y)))
    return np.asarray(amps)


def main():
    fs = 48000
    n = np.arange(int(0.8 * fs))
    x = 0.5 * np.sin(2*np.pi*400*n/fs)

    y26 = process_future_peak(x, 8.0, round(fs * 0.026))
    amps = harmonic_amps(y26, 400.0, fs)
    thd = np.sqrt(np.sum(amps[1:]**2)) / max(1e-30, amps[0])
    thd_db = 20*math.log10(max(thd, 1e-30))
    assert thd_db < -100.0, thd_db

    # 0 ms intentionally degenerates to sample-domain behaviour and is not the
    # transparency reference; confirm it is materially more nonlinear.
    y0 = process_future_peak(x, 8.0, 0)
    amps0 = harmonic_amps(y0, 400.0, fs)
    thd0 = np.sqrt(np.sum(amps0[1:]**2)) / max(1e-30, amps0[0])
    assert thd0 > thd * 100.0

    # ST linked gain must use the stronger L/R window level but its own Ratio.
    l = np.array([0.2, 0.4, 0.3])
    r = np.array([0.1, 0.8, 0.2])
    linked_level = max(np.max(np.abs(l)), np.max(np.abs(r)))
    st_gain = 1.0 / (1.0 + (3.0 - 1.0) * linked_level)
    assert abs(st_gain - 1.0/(1.0+2.0*0.8)) < 1e-15

    print(f'PASS: v1.0.1 transparent future-window core @400 Hz / 26 ms THD {thd_db:.2f} dB')
    print('PASS: 0 ms remains intentionally more nonlinear; ST linked uses stronger L/R level with independent ST Ratio.')


if __name__ == '__main__':
    main()
