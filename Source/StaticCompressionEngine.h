#pragma once

#include <JuceHeader.h>
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
// Threshold is only a lower operating boundary around that existing law.
// Threshold OFF maps to linear 0 (-inf) and therefore executes the exact legacy
// equation. A finite Threshold leaves levels at/below it untouched and re-anchors
// the same curve to unity at the boundary. It does not change detector/window
// semantics and does not introduce Attack, Release, knee or segmentation.
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
        currentGainReductionDb = juce::jmax (0.0f,
            -juce::Decibels::gainToDecibels (juce::jmax (currentGain, 1.0e-9f), -180.0f));
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

        // Same law re-anchored at Threshold so the transition is continuous and
        // exactly unity at the boundary.
        const auto numerator = 1.0f + (ratio - 1.0f) * thresholdLinear;
        const auto denominator = 1.0f + (ratio - 1.0f) * level;
        return numerator / juce::jmax (1.0e-9f, denominator);
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
