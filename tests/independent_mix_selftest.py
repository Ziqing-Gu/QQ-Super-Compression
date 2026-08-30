#!/usr/bin/env python3
import math
import random

random.seed(1001)

# Equal M/S Mix must collapse exactly to the legacy shared Mix behaviour.
for _ in range(200000):
    dry_m = random.uniform(-1.0, 1.0)
    dry_s = random.uniform(-1.0, 1.0)
    wet_m = random.uniform(-1.0, 1.0)
    wet_s = random.uniform(-1.0, 1.0)
    makeup_m = 10.0 ** (random.uniform(-18.0, 18.0) / 20.0)
    makeup_s = 10.0 ** (random.uniform(-18.0, 18.0) / 20.0)
    mix = random.random()

    dry_l = dry_m + dry_s
    dry_r = dry_m - dry_s
    processed_l = wet_m * makeup_m + wet_s * makeup_s
    processed_r = wet_m * makeup_m - wet_s * makeup_s
    legacy_l = dry_l + (processed_l - dry_l) * mix
    legacy_r = dry_r + (processed_r - dry_r) * mix

    mixed_m = dry_m + (wet_m * makeup_m - dry_m) * mix
    mixed_s = dry_s + (wet_s * makeup_s - dry_s) * mix
    split_l = mixed_m + mixed_s
    split_r = mixed_m - mixed_s

    assert math.isclose(legacy_l, split_l, rel_tol=0.0, abs_tol=1e-12)
    assert math.isclose(legacy_r, split_r, rel_tol=0.0, abs_tol=1e-12)

# Independent domain Mix must really be independent.
dry_m, dry_s = 0.2, -0.1
wet_m, wet_s = 0.8, 0.5
makeup_m, makeup_s = 1.0, 1.0
mix_m, mix_s = 1.0, 0.0
mixed_m = dry_m + (wet_m * makeup_m - dry_m) * mix_m
mixed_s = dry_s + (wet_s * makeup_s - dry_s) * mix_s
assert math.isclose(mixed_m, wet_m, abs_tol=1e-12)
assert math.isclose(mixed_s, dry_s, abs_tol=1e-12)

# LR domain follows the same per-domain rule.
dry_l, dry_r = 0.1, -0.2
wet_l, wet_r = 0.9, 0.7
mix_l, mix_r = 0.25, 0.75
out_l = dry_l + (wet_l - dry_l) * mix_l
out_r = dry_r + (wet_r - dry_r) * mix_r
assert math.isclose(out_l, 0.3, abs_tol=1e-12)
assert math.isclose(out_r, 0.475, abs_tol=1e-12)

print('PASS: independent LR/MS Mix math; equal M/S Mix is legacy-equivalent, unequal values remain independent.')
