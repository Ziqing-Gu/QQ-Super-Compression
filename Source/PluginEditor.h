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
        std::function<void()> onGestureEnd;

        void mouseDown (const juce::MouseEvent& event) override
        {
            if (onGestureStart)
                onGestureStart();

            resetClickHandled = event.mods.isAltDown() && event.mods.isLeftButtonDown();
            linearLastDragY = event.position.y;
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

            // JUCE setMouseDragSensitivity() only affects rotary drag styles.
            // Threshold is LinearVertical, so implement an explicit relative
            // drag path: normal uses roughly one slider height for full range;
            // Shift is 8x finer and can reach the 0.01 dB parameter resolution.
            if (getSliderStyle() == juce::Slider::LinearVertical)
            {
                const auto deltaY = static_cast<double> (linearLastDragY - event.position.y);
                linearLastDragY = event.position.y;
                const auto pixelSpan = static_cast<double> (juce::jmax (120, getHeight()));
                const auto fineScale = event.mods.isShiftDown() ? 8.0 : 1.0;
                const auto deltaValue = deltaY * (getMaximum() - getMinimum()) / (pixelSpan * fineScale);
                setValue (juce::jlimit (getMinimum(), getMaximum(), getValue() + deltaValue),
                          juce::sendNotificationSync);
                return;
            }

            updateSensitivity (event.mods);
            juce::Slider::mouseDrag (event);
        }

        void mouseUp (const juce::MouseEvent& event) override
        {
            // mouseDown always begins the Slider/APVTS gesture, including an
            // Alt-reset, so always close that gesture here.
            juce::Slider::mouseUp (event);
            if (onGestureEnd)
                onGestureEnd();
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
        float linearLastDragY = 0.0f;
        static constexpr int normalSensitivity = 180;
        static constexpr int fineSensitivity = 1200;
    };

    static void configureKnob (FineKnob&, const juce::String& suffix = {});
    static void configureThresholdSlider (FineKnob&);
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

    enum class LinkedPair
    {
        none,
        ratioLR, ratioMS,
        thresholdLR, thresholdMS,
        makeupLR, makeupMS,
        mixLR, mixMS
    };
    void beginLinkedGesture (LinkedPair, FineKnob& source, FineKnob& target, const juce::String& undoName);
    void endLinkedGesture();
    void handleLinkedValueChange (LinkedPair, FineKnob& source, FineKnob& target);
    double handleLinkedTextEntry (LinkedPair, FineKnob& source, FineKnob& target,
                                  const juce::String& text, const juce::String& undoName);

    QQSuperCompressionAudioProcessor& processor;
    qqsc::UTF8LookAndFeel utf8LookAndFeel;
    std::unique_ptr<juce::PropertiesFile> uiProperties;

    // All UI widgets live in one fixed 1020x820 design-space root. The editor
    // scales this root uniformly, so user resizing is true 1:1 X/Y scaling
    // instead of independently stretching the layout.
    juce::Component contentRoot;

    DynamicDisplay display;
    LevelMeters meters;

    juce::Label title;
    juce::Label versionLabel;
    juce::Label inputGainLabel;
    juce::Label ratioLabel;
    juce::Label ratioChannel0Label;
    juce::Label ratioChannel1Label;
    juce::Label makeupLabel;
    juce::Label makeupChannel0Label;
    juce::Label makeupChannel1Label;
    juce::Label mixLabel;
    juce::Label mixChannel0Label;
    juce::Label mixChannel1Label;
    juce::Label outputGainLabel;
    juce::Label thresholdLabel;
    juce::Label thresholdChannel0Label;
    juce::Label thresholdChannel1Label;
    juce::Label modeLabel;
    juce::Label lookaheadLabel;
    juce::ComboBox lookaheadCombo;
    juce::Label oversamplingLabel;
    juce::TextButton oversamplingButton { "8x" };

    FineKnob inputGainSlider { 0.0 };
    FineKnob ratioSlider { 8.0 };
    FineKnob ratioLSlider { 8.0 };
    FineKnob ratioRSlider { 8.0 };
    FineKnob ratioMSlider { 8.0 };
    FineKnob ratioSSlider { 8.0 };
    FineKnob makeupSTSlider { 0.0 };
    FineKnob makeupLSlider { 0.0 };
    FineKnob makeupRSlider { 0.0 };
    FineKnob makeupMSlider { 0.0 };
    FineKnob makeupSSlider { 0.0 };
    FineKnob mixSlider { 100.0 };
    FineKnob mixLSlider { 100.0 };
    FineKnob mixRSlider { 100.0 };
    FineKnob mixMSlider { 100.0 };
    FineKnob mixSSlider { 100.0 };
    FineKnob outputGainSlider { 0.0 };
    FineKnob thresholdSlider { qqsc::params::thresholdOffDb };
    FineKnob thresholdLSlider { qqsc::params::thresholdOffDb };
    FineKnob thresholdRSlider { qqsc::params::thresholdOffDb };
    FineKnob thresholdMSlider { qqsc::params::thresholdOffDb };
    FineKnob thresholdSSlider { qqsc::params::thresholdOffDb };

    juce::TextButton modeButton { "ST" };
    juce::TextButton linkButton { "LINK" };
    juce::TextButton matchButton { "MATCH" };
    juce::TextButton bypassButton { "BYPASS" };
    juce::TextButton aButton { "A" };
    juce::TextButton bButton { "B" };
    juce::TextButton aToBButton;
    juce::TextButton bToAButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioLAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioRAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioMAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioSAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupSTAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupLAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupRAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupMAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupSAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixLAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixRAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixMAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixSAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdLAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdRAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdMAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdSAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linkAttachment;

    LinkedPair activeLinkedPair = LinkedPair::none;
    FineKnob* activeLinkSource = nullptr;
    FineKnob* activeLinkTarget = nullptr;
    double activeLinkSourceStart = 0.0;
    double activeLinkTargetStart = 0.0;
    bool linkedValueUpdateInProgress = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QQSuperCompressionAudioProcessorEditor)
};
