#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <cstdint>
#include <vector>

namespace qqsc
{
// QQ Super Compression lookahead level engine.
//
// 0.1.4+ FUTURE-WINDOW CORE (0.1.5+ uses fixed Lookahead presets):
// The rejected 0.1.0 engine directly waveshaped each sample. The 0.1.1-0.1.3
// replacement used a fixed 20 ms rolling RMS detector, which user PluginDoctor
// tests later showed still had two unwanted behaviours: a visible time-window
// attack/release-like response and residual harmonic distortion caused by the
// RMS window moving against the carrier waveform.
//
// This engine deliberately uses a different experiment: while the audio path is
// delayed by N samples, it looks at the complete [current-N ... current] future
// window for the delayed sample and takes the WINDOW PEAK as the level estimate.
// The window length is exactly the user Lookahead value. A steady sine whose
// lookahead window contains enough waveform cycles should therefore produce a
// nearly constant level estimate instead of a carrier-following RMS ripple.
//
// There is intentionally NO compressor Attack or Release envelope in this class.
// Gain is derived directly from the current lookahead-window level:
//
//     gain = 1 / (1 + (ratio - 1) * level)
//
// where level is the 0..1 window peak. Ratio=1 is unity; larger Ratio creates
// more attenuation. The gain curve itself is the existing 0.1.1 user-approved
// direction and is not redesigned here.
//
// NOTE: Lookahead=0 means a one-sample analysis window and therefore intentionally
// degenerates toward instantaneous sample-domain behaviour. It is kept only as
// an experimental comparison point; it is not expected to be distortion-free.
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
        currentGain = 1.0f;
        currentGainReductionDb = 0.0f;
    }

    // Feed the *current, non-delayed* domain sample. The returned gain belongs to
    // the sample delayed by lookaheadSamples, because the queue now contains the
    // future window that starts at that delayed sample and ends at currentSampleIndex.
    float processSample (float sample, float ratio, int64_t currentSampleIndex) noexcept
    {
        const auto magnitude = juce::jlimit (0.0f, 1.0f, std::abs (sample));
        ratio = juce::jmax (1.0f, ratio);

        if (queueValues.empty())
            return 1.0f;

        // Monotonic maximum queue. No allocations occur on the audio thread.
        while (queueCount > 0 && backValue() <= magnitude)
            popBack();

        pushBack (currentSampleIndex, magnitude);

        const auto oldestAllowed = currentSampleIndex - static_cast<int64_t> (lookaheadSamples);
        while (queueCount > 0 && frontIndex() < oldestAllowed)
            popFront();

        const auto level = queueCount > 0 ? frontValue() : magnitude;
        currentGain = 1.0f / (1.0f + (ratio - 1.0f) * level);
        currentGainReductionDb = juce::jmax (0.0f,
            -juce::Decibels::gainToDecibels (juce::jmax (currentGain, 1.0e-9f), -180.0f));
        return currentGain;
    }

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
        // Capacity is maxLookahead + 4 while the logical queue can contain at
        // most lookahead + 1 items, so this should never overflow.
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

    float currentGain = 1.0f;
    float currentGainReductionDb = 0.0f;
};
}
