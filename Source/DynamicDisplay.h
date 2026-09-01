#pragma once

#include <JuceHeader.h>
#include <array>
#include <deque>
#include "PluginProcessor.h"

class DynamicDisplay final : public juce::Component,
                             private juce::Timer
{
public:
    explicit DynamicDisplay (QQSuperCompressionAudioProcessor&);
    ~DynamicDisplay() override = default;

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    struct HistoryPoint
    {
        float inputDb = -120.0f;
        float detectorDb = -120.0f;
        float capturedInputGainDb = 0.0f;
    };

    struct HistorySet
    {
        std::deque<HistoryPoint> points;
    };

    struct ProjectedHistory
    {
        std::deque<float> input;
        std::deque<float> gainReductionBoundary;
        std::deque<float> output;
        std::deque<float> externalKey;
        std::deque<float> effectiveGainReduction;
    };

    void timerCallback() override;
    void pushHistory (HistorySet&, HistoryPoint);
    juce::Path makePath (const std::deque<float>& values, juce::Rectangle<float> plot) const;
    juce::Path makeBandPath (const std::deque<float>& upper, const std::deque<float>& lower,
                             juce::Rectangle<float> plot) const;
    float dbToY (float db, juce::Rectangle<float> plot) const noexcept;
    void drawDomainPanel (juce::Graphics&, juce::Rectangle<float> panel, int domainIndex,
                          const juce::String& domainName, int mode);
    float thresholdDbForDomain (int domainIndex, int mode) const noexcept;
    float ratioForDomain (int domainIndex, int mode) const noexcept;
    float makeupDbForDomain (int domainIndex, int mode) const noexcept;
    float mixForDomain (int domainIndex, int mode) const noexcept;
    ProjectedHistory projectHistory (int domainIndex, int mode, bool externalKey,
                                     bool bypassed) const;
    void clearHistories();

    QQSuperCompressionAudioProcessor& processor;
    std::array<HistorySet, 2> histories;
    int lastMode = -1;
    int lastKeySource = -1;
    static constexpr int historyLength = 240;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicDisplay)
};
