#pragma once

#include <JuceHeader.h>
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
    void timerCallback() override;
    juce::Rectangle<float> getPlotArea() const noexcept;
    float dbToY (float db) const noexcept;
    void pushHistory (std::deque<float>&, float value);
    juce::Path makePath (const std::deque<float>& values) const;

    QQSuperCompressionAudioProcessor& processor;
    std::deque<float> inputHistory;
    std::deque<float> wetHistory;
    std::deque<float> outputHistory;
    static constexpr int historyLength = 240;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicDisplay)
};
