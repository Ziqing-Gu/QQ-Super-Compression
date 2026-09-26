#pragma once

#include <JuceHeader.h>
#include "DynamicsLimits.h"
#include <cmath>
#include <cstdint>
#include <vector>

namespace qqsc
{
// QQ Super Compression transparent lookahead level engine.
//
// v1.0.1 returns to the approved v0.9.4/v0.9.7 FUTURE-WINDOW PEAK core.
// The audible path is delayed by N samples while the detector sees the complete
// future window for the delayed sample and uses that window's peak as the level
// estimate. This deliberately trades a small microscopic pre-influence around
// abrupt level changes for much lower carrier-following harmonic distortion.
//
// There is intentionally NO compressor Attack or Release envelope here.
// Gain is derived directly from the current lookahead-window level:
//
//     legacy gain = 1 / (1 + (ratio - 1) * level)
//
// This independent dB experiment preserves the legacy equation only at -inf.
// Finite Down thresholds use constant dB ratio; finite Up gates use the same
// dB slope as reciprocal Down, anchored at Range/Dual Down/0dB. Existing static
// boundary blends and branch crossfades remain. Detector/window semantics and
// the audible delay are unchanged; no Attack/Release envelope is introduced.
//
// Lookahead=0 degenerates to a one-sample window. The established product logic
// therefore keeps the old 0 ms-only 1x/8x/16x Oversampling choices to reduce
// alias fold-back without changing the intended nonlinear character.
class StaticCompressionEngine
{
public:
    void prepare (int maxLookaheadSamplesIn)
    {
        maxLookaheadSamples = juce::jmax (0, maxLookaheadSamplesIn);
        const auto capacity = static_cast<size_t> (maxLookaheadSamples + 4);
        queueValues.assign (capacity, 0.0f);
        queueIndices.assign (capacity, 0);
        lookaheadSamples = juce::jlimit (0, maxLookaheadSamples, lookaheadSamples);
        reset();
    }

    void setLookaheadSamples (int newLookaheadSamples) noexcept
    {
        lookaheadSamples = juce::jlimit (0, maxLookaheadSamples, newLookaheadSamples);
        reset();
    }

    void reset() noexcept
    {
        queueHead = 0;
        queueCount = 0;
        currentLevel = 0.0f;
        currentGain = 1.0f;
        currentGainReductionDb = 0.0f;
    }

    // Feed the current non-delayed domain sample. The returned gain belongs to
    // the sample delayed by lookaheadSamples, because the monotonic queue contains
    // the complete future window ending at currentSampleIndex.
    float processSample (float sample, float ratio, float thresholdLinear, int64_t currentSampleIndex) noexcept
    {
        const auto magnitude = juce::jlimit (0.0f, 1.0f, std::abs (sample));
        ratio = juce::jmax (1.0f, ratio);

        if (queueValues.empty())
            return 1.0f;

        while (queueCount > 0 && backValue() <= magnitude)
            popBack();

        pushBack (currentSampleIndex, magnitude);

        const auto oldestAllowed = currentSampleIndex - static_cast<int64_t> (lookaheadSamples);
        while (queueCount > 0 && frontIndex() < oldestAllowed)
            popFront();

        currentLevel = queueCount > 0 ? frontValue() : magnitude;
        currentGain = gainForLevel (currentLevel, ratio, thresholdLinear);
        currentGainReductionDb = -juce::Decibels::gainToDecibels (juce::jmax (currentGain, 1.0e-9f), -180.0f);
        return currentGain;
    }

    static float gainForLevel (float level, float ratio, float thresholdLinear) noexcept
    {
        level = juce::jlimit (0.0f, 1.0f, level);
        ratio = juce::jmax (1.0f, ratio);
        thresholdLinear = juce::jlimit (0.0f, 1.0f, thresholdLinear);

        // Threshold OFF: exact pre-Threshold QQ law.
        if (thresholdLinear <= 0.0f)
            return 1.0f / (1.0f + (ratio - 1.0f) * level);

        if (level <= thresholdLinear)
            return 1.0f;

        // Fixed dB ratio above a finite threshold:
        // outputDb = thresholdDb + (levelDb - thresholdDb) / ratio.
        // p and T are detector amplitudes, not individual carrier samples.
        return std::pow (thresholdLinear / level, 1.0f - 1.0f / ratio);
    }

    // Shared audible/Display law. Boundaries are detector-linear values, with
    // lower==upper deliberately disabling the COMPLETE dynamic stage in both modes.
    // Boundary transitions stay inside the active interval. There is no temporal
    // envelope or extra latency; the future-window detector remains unchanged.
    static float singleGainForLevel (float level, float ratio, float lower, float upper) noexcept
    {
        level = juce::jlimit (0.0f, 1.0f, level);
        lower = juce::jlimit (0.0f, 1.0f, lower);
        // Range OFF is +infinity: retain it through the operating gate. A
        // finite 0 dB Range is 1.0 and still restores unity at/above that level.
        upper = juce::jmax (0.0f, upper);
        ratio = juce::jlimit (minimumUpRatio, maximumDownRatio, ratio);
        if (lower >= upper || level <= lower || level >= upper || ratio == 1.0f)
            return 1.0f;
        if (ratio > 1.0f)
        {
            const auto gain = gainForLevel (level, ratio, lower);
            // Infinite Range leaves the selected downward law unbounded. For a
            // finite Range, return continuously to unity BEFORE its upper edge.
            if (! std::isfinite (upper)) return gain;
            const auto width = juce::jmin (upper * 0.5f, (upper - lower) * 0.5f);
            const auto blend = boundaryBlend ((upper - level) / width);
            return (1.0f - blend) + gain * blend;
        }
        return gatedUpwardGain (level, ratio, lower, juce::jmin (1.0f, upper));
    }

    static float boundaryBlend (float position) noexcept
    {
        const auto t = juce::jlimit (0.0f, 1.0f, position);
        return t * t * (3.0f - 2.0f * t);
    }

    static float gatedUpwardGain (float level, float ratio, float lower, float anchor) noexcept
    {
        if (level <= lower || lower >= anchor) return 1.0f;
        // -inf explicitly preserves the original QQ upward family.
        if (lower <= 0.0f) return upwardGainForLevel (level, ratio, anchor);
        // Same dB slope as the reciprocal Down ratio, anchored at the upper
        // boundary: upGainDb = (anchorDb - levelDb) * (1 - ratio).
        // Matching Up/Down differs by a constant gain in the interior; the
        // retained gate/Range transitions intentionally affect the edges.
        ratio = juce::jlimit (minimumUpRatio, 1.0f, ratio);
        const auto gain = level >= anchor || ratio == 1.0f ? 1.0f
            : juce::jmin (maximumUpwardGain, std::pow (anchor / level, 1.0f - ratio));
        const auto width = juce::jmin (lower, (anchor - lower) * 0.5f);
        return 1.0f + (gain - 1.0f) * boundaryBlend ((level - lower) / width);
    }

    static float upwardGainForLevel (float level, float ratio, float anchor) noexcept
    {
        level = juce::jlimit (0.0f, 1.0f, level);
        anchor = juce::jlimit (0.0f, 1.0f, anchor);
        ratio = juce::jlimit (minimumUpRatio, 1.0f, ratio);
        if (anchor <= 0.0f || level >= anchor || ratio == 1.0f)
            return 1.0f;
        // Threshold-relative QQ rational family: an equal distance below the
        // anchor receives equal lift regardless of the absolute threshold dB.
        // Gain is unity at the anchor, bounded by 1/ratio <= 1000, and increases
        // towards quieter levels. Output remains monotonic (no level inversion).
        const auto relativeLevel = level / anchor;
        return 1.0f / (ratio + (1.0f - ratio) * relativeLevel);
    }

    static float dualGainForLevel (float level, float upRatio, float downRatio,
                                   float upThreshold, float downThreshold) noexcept
    {
        // UP is an enabling gate, never an invitation to raise sub-threshold
        // material. The upward branch ends at DOWN, where its gain reaches
        // unity and the retained downward branch takes over.
        if (upThreshold >= downThreshold || level <= upThreshold)
            return 1.0f;
        if (level < downThreshold)
            return gatedUpwardGain (level, upRatio, upThreshold, downThreshold);
        if (level > downThreshold)
            return gainForLevel (level, downRatio, downThreshold);
        return 1.0f;
    }

    // In QQ Super Compression, Mix is part of the user-facing compression
    // depth. Dry and compressed Wet are blended in the linear gain domain, so
    // effective GR is deliberately not core GR dB multiplied by Mix.
    static float effectiveGainForMix (float compressedGain, float wetMix) noexcept
    {
        compressedGain = juce::jlimit (0.0f, maximumUpwardGain, compressedGain);
        wetMix = juce::jlimit (0.0f, 1.0f, wetMix);
        // Weighted sum retains tiny wet gains at 100% Mix; subtracting a
        // near-unity value from unity loses precision for deep dB reduction.
        return (1.0f - wetMix) + compressedGain * wetMix;
    }

    static float effectiveGainReductionDb (float compressedGain, float wetMix) noexcept
    {
        const auto effectiveGain = effectiveGainForMix (compressedGain, wetMix);
        return -juce::Decibels::gainToDecibels (juce::jmax (effectiveGain, 1.0e-9f), -180.0f);
    }

    float getCurrentLevel() const noexcept { return currentLevel; }
    float getCurrentGain() const noexcept { return currentGain; }
    float getCurrentGainReductionDb() const noexcept { return currentGainReductionDb; }
    int getLookaheadSamples() const noexcept { return lookaheadSamples; }

private:
    size_t physicalIndex (size_t logicalOffset) const noexcept
    {
        return (queueHead + logicalOffset) % queueValues.size();
    }

    float frontValue() const noexcept { return queueValues[queueHead]; }
    int64_t frontIndex() const noexcept { return queueIndices[queueHead]; }

    float backValue() const noexcept
    {
        return queueValues[physicalIndex (queueCount - 1)];
    }

    void popFront() noexcept
    {
        if (queueCount == 0)
            return;
        queueHead = (queueHead + 1) % queueValues.size();
        --queueCount;
    }

    void popBack() noexcept
    {
        if (queueCount > 0)
            --queueCount;
    }

    void pushBack (int64_t index, float value) noexcept
    {
        if (queueCount >= queueValues.size())
            popFront();

        const auto p = physicalIndex (queueCount);
        queueValues[p] = value;
        queueIndices[p] = index;
        ++queueCount;
    }

    int maxLookaheadSamples = 0;
    int lookaheadSamples = 0;

    std::vector<float> queueValues;
    std::vector<int64_t> queueIndices;
    size_t queueHead = 0;
    size_t queueCount = 0;

    float currentLevel = 0.0f;
    float currentGain = 1.0f;
    float currentGainReductionDb = 0.0f;
};
}
