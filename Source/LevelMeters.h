#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class LevelMeters final : public juce::Component,
                          private juce::Timer
{
public:
    explicit LevelMeters (QQSuperCompressionAudioProcessor& p)
        : processor (p)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }

    void drawDualMeter (juce::Graphics&,
                        juce::Rectangle<float>,
                        const juce::String& title,
                        const juce::String& channel0,
                        const juce::String& channel1,
                        float value0Db,
                        float value1Db,
                        float minDb,
                        float maxDb,
                        bool reductionMeter,
                        float hold0Db,
                        float hold1Db);

    void drawSingleBar (juce::Graphics&,
                        juce::Rectangle<float>,
                        const juce::String& channelName,
                        float valueDb,
                        float minDb,
                        float maxDb,
                        bool reductionMeter,
                        float holdDb);

    QQSuperCompressionAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeters)
};
