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
    struct HistorySet
    {
        std::deque<float> input;
        std::deque<float> wet;
        std::deque<float> output;
    };

    void timerCallback() override;
    void pushHistory (std::deque<float>&, float value);
    juce::Path makePath (const std::deque<float>& values, juce::Rectangle<float> plot) const;
    float dbToY (float db, juce::Rectangle<float> plot) const noexcept;
    void drawDomainPanel (juce::Graphics&, juce::Rectangle<float> panel, int domainIndex,
                          const juce::String& domainName, int mode);
    float thresholdDbForDomain (int domainIndex, int mode) const noexcept;
    void clearHistories();

    QQSuperCompressionAudioProcessor& processor;
    std::array<HistorySet, 2> histories;
    int lastMode = -1;
    static constexpr int historyLength = 240;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicDisplay)
};
