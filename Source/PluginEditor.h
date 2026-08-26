#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DynamicDisplay.h"
#include "LevelMeters.h"
#include "UTF8LookAndFeel.h"

class QQSuperCompressionAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                      private juce::Timer,
                                                      private juce::KeyListener
{
public:
    explicit QQSuperCompressionAudioProcessorEditor (QQSuperCompressionAudioProcessor&);
    ~QQSuperCompressionAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class FineKnob final : public juce::Slider
    {
    public:
        explicit FineKnob (double defaultValueIn) : defaultValue (defaultValueIn) {}

        std::function<void()> onGestureStart;

        void mouseDown (const juce::MouseEvent& event) override
        {
            if (onGestureStart)
                onGestureStart();

            resetClickHandled = event.mods.isAltDown() && event.mods.isLeftButtonDown();
            updateSensitivity (event.mods);

            // Always let Slider begin its normal drag gesture first. This keeps
            // the APVTS attachment/host gesture valid even for Alt-reset, so the
            // reset participates in the same UndoManager path as a normal knob
            // movement. Dragging is suppressed below while resetClickHandled is
            // true, so Alt+click remains a single reset action.
            juce::Slider::mouseDown (event);

            if (resetClickHandled)
                setValue (defaultValue, juce::sendNotificationSync);
        }

        void mouseDrag (const juce::MouseEvent& event) override
        {
            if (resetClickHandled)
                return;

            updateSensitivity (event.mods);
            juce::Slider::mouseDrag (event);
        }

        void mouseUp (const juce::MouseEvent& event) override
        {
            // mouseDown always begins the Slider/APVTS gesture, including an
            // Alt-reset, so always close that gesture here.
            juce::Slider::mouseUp (event);
            resetClickHandled = false;
            setMouseDragSensitivity (normalSensitivity);
        }

    private:
        void updateSensitivity (juce::ModifierKeys mods)
        {
            // Holding Shift makes rotary dragging substantially finer. This is
            // intentionally UI-only and does not alter parameter step sizes.
            setMouseDragSensitivity (mods.isShiftDown() ? fineSensitivity : normalSensitivity);
        }

        double defaultValue = 0.0;
        bool resetClickHandled = false;
        static constexpr int normalSensitivity = 180;
        static constexpr int fineSensitivity = 1200;
    };

    static void configureKnob (FineKnob&, const juce::String& suffix = {});
    static void configureLabel (juce::Label&, const juce::String& text);
    static void configureActionButton (juce::TextButton&);
    static std::unique_ptr<juce::PropertiesFile> createUiProperties();

    void timerCallback() override;
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;
    void updateModeUi();
    void beginUndoTransaction (const juce::String& name);
    void cycleMode();
    void commitLookaheadChoice();
    void cycleOversampling();
    void updateOversamplingUi();
    void setChoiceParameter (const char* parameterID, int value);
    void registerKeyboardListener (juce::Component&);

    QQSuperCompressionAudioProcessor& processor;
    qqsc::UTF8LookAndFeel utf8LookAndFeel;
    std::unique_ptr<juce::PropertiesFile> uiProperties;

    // All UI widgets live in one fixed 1020x670 design-space root. The editor
    // scales this root uniformly, so user resizing is true 1:1 X/Y scaling
    // instead of independently stretching the layout.
    juce::Component contentRoot;

    DynamicDisplay display;
    LevelMeters meters;

    juce::Label title;
    juce::Label versionLabel;
    juce::Label ratioLabel;
    juce::Label makeupLabel;
    juce::Label makeupChannel0Label;
    juce::Label makeupChannel1Label;
    juce::Label mixLabel;
    juce::Label modeLabel;
    juce::Label lookaheadLabel;
    juce::ComboBox lookaheadCombo;
    juce::Label oversamplingLabel;
    juce::TextButton oversamplingButton { "8x" };

    FineKnob ratioSlider { 8.0 };
    FineKnob makeupSTSlider { 0.0 };
    FineKnob makeupLSlider { 0.0 };
    FineKnob makeupRSlider { 0.0 };
    FineKnob makeupMSlider { 0.0 };
    FineKnob makeupSSlider { 0.0 };
    FineKnob mixSlider { 100.0 };

    juce::TextButton modeButton { "ST" };
    juce::TextButton matchButton { "MATCH" };
    juce::TextButton bypassButton { "BYPASS" };
    juce::TextButton aButton { "A" };
    juce::TextButton bButton { "B" };
    juce::TextButton aToBButton;
    juce::TextButton bToAButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupSTAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupLAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupRAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupMAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupSAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QQSuperCompressionAudioProcessorEditor)
};
