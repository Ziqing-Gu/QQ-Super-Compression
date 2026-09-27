#pragma once

#include <algorithm>

namespace qqsc
{
// Only boundary transfer policy: no gain calculation, Link flag, or parameter
// IDs. The processor calls this for each independent ST/L/R/M/S domain.
struct LimiterModeBoundaryPair
{
    float lowerDb = -120.0f;
    float upperDb = 0.0f;
};

// Single's Threshold (lower) and Dual's DOWN Threshold (upper) represent the
// threshold that must survive a Single/Dual switch in Limiter mode. Copy its
// current value, not -Output: an unlinked/manual Output offset is legitimate.
// Keep the destination's companion boundary unless it crosses the threshold;
// in that case retain the existing boundary-push/equality rule. In particular,
// do not invent a new Range or UP-gate law in this parameter-continuity fix.
inline constexpr LimiterModeBoundaryPair continueLimiterModeBoundaries (
    bool destinationIsDual, LimiterModeBoundaryPair source,
    LimiterModeBoundaryPair destination) noexcept
{
    if (destinationIsDual)
        return { std::min (destination.lowerDb, source.lowerDb), source.lowerDb };

    return { source.upperDb, std::max (destination.upperDb, source.upperDb) };
}
}
