#pragma once

#include <JuceHeader.h>
#include "DynamicsLimits.h"
#include <cmath>
#include <cstdint>
#include <vector>

namespace qqsc
{
enum class CompressionAlgorithm { classic, super };
// QQ Super Compression transparent lookahead level engine.
//
// Original detector: align the audible sample t with two peak windows and use
// min(max |x[t-N .. t]|, max |x[t .. t+N]|). Both include the current sample.
// The optional bilateral detector uses max of the same two peaks. It covers
// transients on either side without adding an Attack/Release envelope.
//
// There is intentionally NO compressor Attack or Release envelope here.
// Gain is derived directly from the current lookahead-window level:
//
//     legacy gain = 1 / (1 + (ratio - 1) * level)
//
// Classic uses a finite -90 dB minimum throughout. Super uses the original
// rational law at both finite and -inf thresholds.
// Finite Down thresholds use constant dB ratio; finite Up gates use the same
// dB slope as reciprocal Down, anchored at Range/Dual Down/0dB. Existing static
// boundary blends and branch crossfades remain. The audible delay is unchanged;
// no Attack/Release envelope is introduced.
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
        peakHistory.assign (capacity, 0.0f);
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
        peakHistoryWrite = 0;
        peakHistoryCount = 0;
        currentLevel = 0.0f;
        currentFuturePeak = 0.0f;
        currentPastPeak = 0.0f;
        currentGain = 1.0f;
        currentGainReductionDb = 0.0f;
    }

    // Feed the current non-delayed domain sample. The returned gain belongs to
    // the sample delayed by lookaheadSamples. The queue gives its future peak;
    // the queue result from N samples ago gives its past peak without another
    // queue, allocation, or scan on the audio thread.
    float processSample (float sample, float ratio, float thresholdLinear, int64_t currentSampleIndex) noexcept
    {
        if (queueValues.empty())
            return 1.0f;
        processLevelSample (sample, currentSampleIndex);
        currentGain = gainForLevel (currentLevel, juce::jmax (1.0f, ratio), thresholdLinear);
        currentGainReductionDb = -juce::Decibels::gainToDecibels (juce::jmax (currentGain, 1.0e-9f), -180.0f);
        return currentGain;
    }

    // Processor gain curves are evaluated separately. Do not calculate a
    // discarded unity gain/GR for every oversampled detector sample.
    float processLevelSample (float sample, int64_t currentSampleIndex) noexcept
    {
        const auto magnitude = juce::jlimit (0.0f, 1.0f, std::abs (sample));
        if (queueValues.empty())
            return currentLevel;
        // A zero-length window is exactly this sample. Changing lookahead
        // resets the queues; the processor replays its retained key history.
        if (lookaheadSamples == 0)
        {
            currentFuturePeak = currentPastPeak = magnitude;
            return currentLevel = magnitude;
        }

        while (queueCount > 0 && backValue() <= magnitude)
            popBack();

        pushBack (currentSampleIndex, magnitude);

        const auto oldestAllowed = currentSampleIndex - static_cast<int64_t> (lookaheadSamples);
        while (queueCount > 0 && frontIndex() < oldestAllowed)
            popFront();

        const auto futurePeak = queueCount > 0 ? frontValue() : magnitude;
        peakHistory[peakHistoryWrite] = futurePeak;
        const auto delay = static_cast<size_t> (lookaheadSamples);
        const auto pastIndex = peakHistoryWrite >= delay ? peakHistoryWrite - delay
                                                        : peakHistoryWrite + peakHistory.size() - delay;
        const auto pastPeak = peakHistoryCount >= delay ? peakHistory[pastIndex] : 0.0f;
        currentFuturePeak = futurePeak;
        currentPastPeak = pastPeak;
        currentLevel = juce::jmin (futurePeak, pastPeak);
        if (++peakHistoryWrite == peakHistory.size()) peakHistoryWrite = 0;
        peakHistoryCount = juce::jmin (peakHistoryCount + 1, peakHistory.size());
        return currentLevel;
    }

    static float gainForLevel (float level, float ratio, float thresholdLinear,
                              CompressionAlgorithm algorithm = CompressionAlgorithm::classic) noexcept
    {
        level = juce::jlimit (0.0f, 1.0f, level);
        ratio = juce::jmax (1.0f, ratio);
        thresholdLinear = juce::jlimit (0.0f, 1.0f, thresholdLinear);

        // The shared host endpoint is clamped to -90 dB in Classic; only
        // Super interprets it as -inf and uses the old rational law.
        if (algorithm == CompressionAlgorithm::classic)
            thresholdLinear = juce::jmax (classicThresholdMinimumGain, thresholdLinear);

        if (thresholdLinear <= 0.0f)
            return 1.0f / (1.0f + (ratio - 1.0f) * level);

        if (level <= thresholdLinear)
            return 1.0f;

        if (algorithm == CompressionAlgorithm::super)
            return (1.0f + (ratio - 1.0f) * thresholdLinear)
                 / juce::jmax (1.0e-9f, 1.0f + (ratio - 1.0f) * level);

        // Fixed dB ratio above a finite threshold:
        // outputDb = thresholdDb + (levelDb - thresholdDb) / ratio.
        // p and T are detector amplitudes, not individual carrier samples.
        return std::pow (thresholdLinear / level, 1.0f - 1.0f / ratio);
    }

    // Shared audible/Display law. Boundaries are detector-linear values, with
    // lower==upper deliberately disabling the COMPLETE dynamic stage in both modes.
    // Boundary transitions stay inside the active interval. There is no temporal
    // envelope or extra latency; the future-window detector remains unchanged.
    static float singleGainForLevel (float level, float ratio, float lower, float upper,
                                    CompressionAlgorithm algorithm = CompressionAlgorithm::classic) noexcept
    {
        level = juce::jlimit (0.0f, 1.0f, level);
        lower = juce::jlimit (0.0f, 1.0f, lower);
        // Range OFF is +infinity: retain it through the operating gate. A
        // finite 0 dB Range is 1.0 and still restores unity at/above that level.
        upper = juce::jmax (0.0f, upper);
        ratio = juce::jlimit (minimumUpRatio, maximumDownRatio, ratio);
        if (algorithm == CompressionAlgorithm::classic)
        {
            lower = juce::jmax (classicThresholdMinimumGain, lower);
            upper = juce::jmax (classicThresholdMinimumGain, upper);
        }
        if (lower >= upper || level <= lower || level >= upper || ratio == 1.0f)
            return 1.0f;
        if (ratio > 1.0f)
        {
            const auto gain = gainForLevel (level, ratio, lower, algorithm);
            // Infinite Range leaves the selected downward law unbounded. For a
            // finite Range, return continuously to unity BEFORE its upper edge.
            if (! std::isfinite (upper)) return gain;
            const auto width = juce::jmin (upper * 0.5f, (upper - lower) * 0.5f);
            const auto blend = boundaryBlend ((upper - level) / width);
            return (1.0f - blend) + gain * blend;
        }
        return gatedUpwardGain (level, ratio, lower, juce::jmin (1.0f, upper), algorithm);
    }

    static float boundaryBlend (float position) noexcept
    {
        const auto t = juce::jlimit (0.0f, 1.0f, position);
        return t * t * (3.0f - 2.0f * t);
    }

    static float gatedUpwardGain (float level, float ratio, float lower, float anchor,
                                 CompressionAlgorithm algorithm = CompressionAlgorithm::classic) noexcept
    {
        if (algorithm == CompressionAlgorithm::classic)
        {
            lower = juce::jmax (classicThresholdMinimumGain, lower);
            anchor = juce::jmax (classicThresholdMinimumGain, anchor);
        }
        if (level <= lower || lower >= anchor) return 1.0f;
        // Only Super has a true -inf gate.
        if (lower <= 0.0f) return upwardGainForLevel (level, ratio, anchor);
        // Same dB slope as the reciprocal Down ratio, anchored at the upper
        // boundary: upGainDb = (anchorDb - levelDb) * (1 - ratio).
        // Matching Up/Down differs by a constant gain in the interior; the
        // retained gate/Range transitions intentionally affect the edges.
        ratio = juce::jlimit (minimumUpRatio, 1.0f, ratio);
        const auto gain = algorithm == CompressionAlgorithm::super
            ? upwardGainForLevel (level, ratio, anchor)
            : (level >= anchor || ratio == 1.0f ? 1.0f
                : juce::jmin (maximumUpwardGain, std::pow (anchor / level, 1.0f - ratio)));
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
                                   float upThreshold, float downThreshold,
                                   CompressionAlgorithm algorithm = CompressionAlgorithm::classic) noexcept
    {
        return dualGainForLevel(level,upRatio,downRatio,upThreshold,downThreshold,algorithm,algorithm);
    }

    static float dualGainForLevel (float level, float upRatio, float downRatio,
                                   float upThreshold, float downThreshold,
                                   CompressionAlgorithm upAlgorithm, CompressionAlgorithm downAlgorithm) noexcept
    {
        // UP is an enabling gate, never an invitation to raise sub-threshold
        // material. The upward branch ends at DOWN, where its gain reaches
        // unity and the retained downward branch takes over.
        if (upAlgorithm == CompressionAlgorithm::classic)
            upThreshold = juce::jmax (classicThresholdMinimumGain, upThreshold);
        if (downAlgorithm == CompressionAlgorithm::classic)
            downThreshold = juce::jmax (classicThresholdMinimumGain, downThreshold);
        if (upThreshold >= downThreshold || level <= upThreshold)
            return 1.0f;
        if (level < downThreshold)
            return gatedUpwardGain (level, upRatio, upThreshold, downThreshold, upAlgorithm);
        if (level > downThreshold)
            return gainForLevel (level, downRatio, downThreshold, downAlgorithm);
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
    float getCurrentFuturePeak() const noexcept { return currentFuturePeak; }
    float getCurrentPastPeak() const noexcept { return currentPastPeak; }
    float getCurrentBilateralPeak() const noexcept { return juce::jmax (currentFuturePeak, currentPastPeak); }
    float getCurrentGain() const noexcept { return currentGain; }
    float getCurrentGainReductionDb() const noexcept { return currentGainReductionDb; }
    int getLookaheadSamples() const noexcept { return lookaheadSamples; }

private:
    size_t physicalIndex (size_t logicalOffset) const noexcept
    {
        // logicalOffset is strictly smaller than the allocated ring.
        const auto index = queueHead + logicalOffset;
        return index < queueValues.size() ? index : index - queueValues.size();
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
        if (++queueHead == queueValues.size()) queueHead = 0;
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
    std::vector<float> peakHistory;
    size_t peakHistoryWrite = 0;
    size_t peakHistoryCount = 0;

    float currentLevel = 0.0f;
    float currentFuturePeak = 0.0f;
    float currentPastPeak = 0.0f;
    float currentGain = 1.0f;
    float currentGainReductionDb = 0.0f;
};
}
