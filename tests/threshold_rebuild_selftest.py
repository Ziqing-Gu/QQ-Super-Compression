#!/usr/bin/env python3
"""v1.0.1 Threshold regression for the restored future-window QQ gain law."""
import random


def gain_for_level(level, ratio, threshold):
    level = min(1.0, max(0.0, level))
    ratio = max(1.0, ratio)
    threshold = min(1.0, max(0.0, threshold))
    if threshold <= 0.0:
        return 1.0 / (1.0 + (ratio - 1.0) * level)
    if level <= threshold:
        return 1.0
    return (1.0 + (ratio - 1.0) * threshold) / (1.0 + (ratio - 1.0) * level)


random.seed(1001)
for _ in range(200000):
    level = random.random()
    ratio = 1.0 + 31.0 * random.random()
    legacy = 1.0 / (1.0 + (ratio - 1.0) * level)
    assert abs(gain_for_level(level, ratio, 0.0) - legacy) < 1e-15

for t in (0.001, 0.01, 0.1, 0.25, 0.5):
    for ratio in (1.0, 1.5, 3.0, 8.0, 16.0, 32.0):
        assert gain_for_level(t * 0.99, ratio, t) == 1.0
        assert gain_for_level(t, ratio, t) == 1.0
        right = gain_for_level(t + 1e-8, ratio, t)
        assert abs(right - 1.0) < 1e-6

print('PASS: v1.0.1 Threshold OFF is exactly the pre-Threshold QQ future-window gain law.')
print('PASS: finite Threshold is unity below/at the boundary and continuous above it.')
