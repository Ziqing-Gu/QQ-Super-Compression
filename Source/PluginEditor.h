#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DynamicDisplay.h"
#include "LevelMeters.h"
#include "UTF8LookAndFeel.h"
#include "DarkPanelMaterial.h"

class QQSuperCompressionAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                      private juce::Timer,
                                                      private juce::KeyListener
{
public:
    explicit QQSuperCompressionAudioProcessorEditor (QQSuperCompressionAudioProcessor&,
        std::unique_ptr<juce::PropertiesFile> settingsOverride = {});
    ~QQSuperCompressionAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    friend struct QQSCVisualCheck;
    qqsc::dark::BottomPanelMaterial darkBottomPanel;
    class FineKnob final : public juce::Slider
    {
    public:
        explicit FineKnob (double defaultValueIn) : defaultValue (defaultValueIn) {}

        std::function<void()> onGestureStart;
        std::function<void()> onGestureEnd;
        std::function<juce::Rectangle<float>()> boundaryPlotBounds;
        std::function<float(float)> boundaryDbToY;
        std::function<float(float)> boundaryYToDb;
        float getBoundaryThumbY() const
        {
            return boundaryDbToY ? boundaryDbToY (static_cast<float> (getValue())) : 0.0f;
        }
        void setResetValue (double value) noexcept { defaultValue = value; }
        bool hasActiveNativeGesture() const noexcept { return nativeGestureActive; }
        bool isDragCancelledUntilMouseUp() const noexcept { return dragCancelledUntilMouseUp; }

        void cancelNativeDragForParameterRebind()
        {
            if (! nativeGestureActive || lastMouseDownEvent == nullptr)
                return;

            // Finish JUCE's real ScopedDragNotification while the old APVTS
            // attachment is still listening. A later physical mouseUp must not
            // send the end event to a newly attached parameter.
            dragCancelledUntilMouseUp = true;
            juce::Slider::mouseUp (*lastMouseDownEvent);
            if (onGestureEnd)
                onGestureEnd();
        }

        void startedDragging() override
        {
            nativeGestureActive = true;
            juce::Slider::startedDragging();
        }

        void stoppedDragging() override
        {
            nativeGestureActive = false;
            juce::Slider::stoppedDragging();
        }

        void mouseDown (const juce::MouseEvent& event) override
        {
            dragCancelledUntilMouseUp = false;
            lastMouseDownEvent = std::make_unique<juce::MouseEvent> (event);
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

            if (resetClickHandled && ! dragCancelledUntilMouseUp)
                setValue (defaultValue, juce::sendNotificationSync);
        }

        void mouseDrag (const juce::MouseEvent& event) override
        {
            if (resetClickHandled || dragCancelledUntilMouseUp)
                return;

            // JUCE setMouseDragSensitivity() only affects rotary drag styles.
            // Threshold is LinearVertical, so implement an explicit relative
            // drag path: normal uses roughly one slider height for full range;
            // Shift is 8x finer and can reach the 0.01 dB parameter resolution.
            if (getSliderStyle() == juce::Slider::LinearVertical)
            {
                const auto deltaY = static_cast<double> (linearLastDragY - event.position.y);
                linearLastDragY = event.position.y;
                const auto fineScale = event.mods.isShiftDown() ? 8.0 : 1.0;
                const auto plot = boundaryPlotBounds ? boundaryPlotBounds() : getLocalBounds().toFloat();
                const auto pixelSpan = static_cast<double> (juce::jmax (1.0f, plot.getHeight()));
                const auto deltaValue = deltaY * (boundaryPlotBounds ? 90.0 : getMaximum() - getMinimum()) / (pixelSpan * fineScale);
                auto current = getValue();
                if (boundaryYToDb && deltaY > 0.0 && current <= boundaryYToDb (plot.getBottom()))
                    current = boundaryYToDb (plot.getBottom());
                if (current > 0.0 && deltaY < 0.0)
                    current = 0.0; // Leave Range OFF through its finite 0 dB endpoint.
                auto requested = current + deltaValue;
                if (boundaryYToDb && deltaY < 0.0 && requested <= boundaryYToDb (plot.getBottom()))
                    requested = getMinimum(); // The visible bottom detent remains -infinity.
                if (getMaximum() > 0.0 && requested > 0.0)
                    requested = getMaximum(); // Range's extra top detent is OFF, not +1 dB.
                setValue (juce::jlimit (getMinimum(), getMaximum(), requested),
                          juce::sendNotificationSync);
                return;
            }

            updateSensitivity (event.mods);
            juce::Slider::mouseDrag (event);
        }

        void mouseUp (const juce::MouseEvent& event) override
        {
            if (dragCancelledUntilMouseUp)
            {
                dragCancelledUntilMouseUp = false;
                resetClickHandled = false;
                lastMouseDownEvent.reset();
                setMouseDragSensitivity (normalSensitivity);
                return;
            }
            // mouseDown always begins the Slider/APVTS gesture, including an
            // Alt-reset, so always close that gesture here.
            juce::Slider::mouseUp (event);
            if (onGestureEnd)
                onGestureEnd();
            resetClickHandled = false;
            lastMouseDownEvent.reset();
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
        bool nativeGestureActive = false;
        bool dragCancelledUntilMouseUp = false;
        std::unique_ptr<juce::MouseEvent> lastMouseDownEvent;
        float linearLastDragY = 0.0f;
        static constexpr int normalSensitivity = 180;
        static constexpr int fineSensitivity = 1200;
    };

    class SidechainPanelBackground final : public juce::Component
    {
    public:
        SidechainPanelBackground() { setInterceptsMouseClicks (false, false); }

        void paint (juce::Graphics& g) override
        {
            const auto r = getLocalBounds().toFloat().reduced (1.0f);
            if (qqsc::ui::isDarkTheme())
            {
                g.setColour (juce::Colours::black.withAlpha (0.26f));
                g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 12.0f);
                qqsc::dark::panel (g, r, 12.0f);
                return;
            }
            if (! qqsc::ui::isClassicTheme())
            {
                qqsc::warm::backplate (g, r, 12.0f);
                return;
            }
            g.setColour (juce::Colours::black.withAlpha (0.16f));
            g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 12.0f);

            juce::ColourGradient gradient (qqsc::ui::panel().brighter (0.02f), r.getCentreX(), r.getY(),
                                            qqsc::ui::panelAlt(), r.getCentreX(), r.getBottom(), false);
            g.setGradientFill (gradient);
            g.fillRoundedRectangle (r, 12.0f);
            g.setColour (qqsc::ui::border().withAlpha (0.92f));
            g.drawRoundedRectangle (r, 12.0f, 1.0f);
        }
    };

    static void configureKnob (FineKnob&, const juce::String& suffix = {});
    class BoundaryLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                               juce::Slider::SliderStyle, juce::Slider&) override;
    };
    static void configureThresholdSlider (FineKnob&);
    void initialiseCompressionControls();
    void updateCompressionUi();
    void reattachCompressionControls (bool dual);
    void finishCompressionControlGestures();
    void beginBoundaryGesture (int domain, bool upper);
    void handleBoundaryChange (int domain, bool upper);
    void refreshBoundaryReadouts();
    std::array<FineKnob*, 5> lowerBoundaryControls();
    std::array<FineKnob*, 5> mainRatioControls();
    static void configureLabel (juce::Label&, const juce::String& text);
    static void configureActionButton (juce::TextButton&);
    static std::unique_ptr<juce::PropertiesFile> createUiProperties();
    void applyTheme();
    void toggleTheme();
    void toggleSidechainPanel();
    void setKeySource (int source);
    void updateSidechainUi();


    void timerCallback() override;
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;
    void updateModeUi();
    void updateMonitorUi();
    void selectMonitor (int selection);
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
        inputOutput,
        ratioLR, ratioMS,
        thresholdLR, thresholdMS,
        upperBoundaryLR, upperBoundaryMS,
        downRatioLR, downRatioMS,
        makeupLR, makeupMS,
        mixLR, mixMS
    };
    void beginLinkedGesture (LinkedPair, FineKnob& source, FineKnob& target, const juce::String& undoName);
    void endLinkedGesture();
    void beginDualRatioGesture (int domain, bool upward);
    void handleDualRatioChange (int domain, bool upward);
    double applyDualRatioChange (double requested, bool writeSource);
    double dualRatioFromText (int domain, bool upward, const juce::String&);
    void handleLinkedValueChange (LinkedPair, FineKnob& source, FineKnob& target);
    void setLinkedControlValue (FineKnob&, double value);
    double handleLinkedTextEntry (LinkedPair, FineKnob& source, FineKnob& target,
                                  const juce::String& text, const juce::String& undoName);

    QQSuperCompressionAudioProcessor& processor;
    qqsc::UTF8LookAndFeel utf8LookAndFeel;
    BoundaryLookAndFeel boundaryLookAndFeel;
    std::unique_ptr<juce::PropertiesFile> uiProperties;

    // All UI widgets live in one fixed 1200x800 design-space root. The editor
    // scales this root uniformly, so user resizing is true 1:1 X/Y scaling
    // instead of independently stretching the layout.
    juce::Component contentRoot;

    DynamicDisplay display;
    LevelMeters meters;
    SidechainPanelBackground sidechainPanelBackground;

    juce::Label title;
    juce::Label versionLabel;
    juce::Label inputGainLabel;
    juce::Label ratioLabel;
    juce::TextButton dualRatioLinkButton { "LINK" };
    juce::TextButton inputOutputLinkButton { "LINK" };
    std::array<juce::TextButton, 5> upEnabledButtons, downEnabledButtons;
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
    juce::Label monitorLabel;
    juce::Label lookaheadLabel;
    juce::ComboBox lookaheadCombo;
    juce::Label oversamplingLabel;
    juce::TextButton oversamplingButton { "8x" };
    juce::Label keySourceLabel;
    juce::Label keyGainLabel;
    juce::Label keyHpfLabel;
    juce::Label keyMeterLabel;

    FineKnob inputGainSlider { 0.0 };
    FineKnob ratioSlider { 1.0 };
    FineKnob ratioLSlider { 1.0 };
    FineKnob ratioRSlider { 1.0 };
    FineKnob ratioMSlider { 1.0 };
    FineKnob ratioSSlider { 1.0 };
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
    FineKnob keyGainSlider { 0.0 };
    FineKnob keyHpfSlider { qqsc::params::keyHpfOffHz };

    juce::TextButton modeButton { "ST" };
    juce::TextButton compressionModeButton { "SINGLE" };
    std::array<std::unique_ptr<FineKnob>, 5> upperBoundarySliders;
    std::array<std::unique_ptr<FineKnob>, 5> downRatioSliders;
    std::array<juce::Label, 5> lowerBoundaryNames, upperBoundaryNames;
    std::array<juce::Label, 5> lowerBoundaryValues, upperBoundaryValues;
    std::array<juce::Label, 5> upRatioNames, downRatioNames;
    juce::TextButton linkButton { "LINK" };
    juce::TextButton monitorAllButton { "ALL" };
    juce::TextButton monitorFirstButton { "L" };
    juce::TextButton monitorSecondButton { "R" };
    juce::TextButton matchButton { "MATCH" };
    juce::TextButton bypassButton { "BYPASS" };
    juce::TextButton aButton { "A" };
    juce::TextButton bButton { "B" };
    juce::TextButton aToBButton;
    juce::TextButton bToAButton;
    juce::TextButton themeButton { "LIGHT" };
    juce::TextButton sidechainButton { "SC: INT" };
    juce::TextButton keyInternalButton { "INT" };
    juce::TextButton keyExternalButton { "EXT" };
    juce::TextButton sidechainListenButton { "SC LISTEN" };
    double keyMeterProgress = 0.0;
    juce::ProgressBar keyMeter { keyMeterProgress };


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
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> keyGainAttachment;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> upperBoundaryAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> downRatioAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> keyHpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> linkAttachment;

    LinkedPair activeLinkedPair = LinkedPair::none;
    qqsc::ui::Theme theme = qqsc::ui::Theme::light;
    bool sidechainPanelOpen = false;
    juce::Rectangle<int> sidechainPanelBounds;

    FineKnob* activeLinkSource = nullptr;
    FineKnob* activeLinkTarget = nullptr;
    double activeLinkSourceStart = 0.0;
    double activeLinkTargetStart = 0.0;
    bool linkedValueUpdateInProgress = false;
    bool boundaryValueUpdateInProgress = false;
    bool boundaryGestureActive = false;
    int attachedCompressionMode = -1;
    int laidOutProcessingMode = -1;
    float boundaryReferenceInputDb = std::numeric_limits<float>::quiet_NaN();
    int boundaryReferenceKeySource = -1;
    std::vector<juce::RangedAudioParameter*> companionGestureParameters;
    bool dualRatioGestureActive = false;
    bool dualRatioValueUpdateInProgress = false;
    bool dualRatioGestureUpward = true;
    bool dualRatioGestureCoupled = true;
    int dualRatioGestureDomain = 0;
    int dualRatioGesturePartner = 0;
    std::array<double, 5> dualRatioStartUp {}, dualRatioStartDown {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QQSuperCompressionAudioProcessorEditor)
};
