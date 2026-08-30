#!/usr/bin/env python3
import math

THRESHOLD_OFF = -120.0


def enabled(db):
    return db > THRESHOLD_OFF + 1e-4


def linked_pair(source_start, target_start, requested_source, lo, hi, threshold=False):
    if threshold and (enabled(source_start) != enabled(target_start)):
        return requested_source, target_start
    requested_delta = requested_source - source_start
    min_delta = max(lo - source_start, lo - target_start)
    max_delta = min(hi - source_start, hi - target_start)
    delta = min(max(requested_delta, min_delta), max_delta)
    return source_start + delta, target_start + delta


def main():
    # Relative means preserve the existing numerical difference, never equalise.
    assert linked_pair(3.0, 5.0, 4.0, 1.0, 32.0) == (4.0, 6.0)
    assert linked_pair(-20.0, -10.0, -18.0, -120.0, 0.0, threshold=True) == (-18.0, -8.0)
    assert linked_pair(-3.0, 1.0, -2.0, -36.0, 36.0) == (-2.0, 2.0)

    # Pair boundary stops both controls so the difference is preserved.
    assert linked_pair(30.0, 31.0, 32.0, 1.0, 32.0) == (31.0, 32.0)
    assert linked_pair(-35.0, -30.0, -40.0, -36.0, 36.0) == (-36.0, -31.0)

    # Threshold OFF is conceptual -infinity. If exactly one side is OFF, it
    # stays OFF during that gesture because no finite relative offset exists.
    assert linked_pair(-20.0, THRESHOLD_OFF, -18.0, THRESHOLD_OFF, 0.0, threshold=True) == (-18.0, THRESHOLD_OFF)
    # If both are OFF they can be raised together.
    assert linked_pair(THRESHOLD_OFF, THRESHOLD_OFF, -30.0, THRESHOLD_OFF, 0.0, threshold=True) == (-30.0, -30.0)

    print('PASS: v1.0.1 relative Link preserves Ratio/Threshold/Makeup offsets and shared boundaries.')


if __name__ == '__main__':
    main()
