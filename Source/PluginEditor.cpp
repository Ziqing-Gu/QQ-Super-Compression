#include "PluginEditor.h"
#include "Parameters.h"
#include <cmath>

namespace
{
constexpr int defaultEditorWidth = 1020;
constexpr int defaultEditorHeight = 820;
constexpr double editorAspectRatio = static_cast<double> (defaultEditorWidth) / defaultEditorHeight;
constexpr int minEditorHeight = 720;
constexpr int minEditorWidth = static_cast<int> (minEditorHeight * editorAspectRatio + 0.5);
constexpr int maxEditorHeight = 1100;
constexpr int maxEditorWidth = static_cast<int> (maxEditorHeight * editorAspectRatio + 0.5);

juce::String arrowText (const juce::String& left, const juce::String& right)
{
    const auto arrow = juce::String::fromUTF8 (u8"\u2192");
    return left + arrow + right;
}
}

QQSuperCompressionAudioProcessorEditor::QQSuperCompressionAudioProcessorEditor (QQSuperCompressionAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      uiProperties (createUiProperties()),
      display (p),
      meters (p)
{
    setLookAndFeel (&utf8LookAndFeel);
    setResizable (true, true);
    setResizeLimits (minEditorWidth, minEditorHeight, maxEditorWidth, maxEditorHeight);
    if (auto* editorBoundsConstrainer = getConstrainer())
        editorBoundsConstrainer->setFixedAspectRatio (editorAspectRatio);

    int savedWidth = defaultEditorWidth;
    if (uiProperties != nullptr)
    {
        savedWidth = uiProperties->getIntValue ("editorWidth", defaultEditorWidth);
    }

    // v1.0.1 intentionally changes the design aspect ratio to make the Display
    // much taller. Preserve the user's prior *width scale* and let the new
    // aspect ratio add vertical space; fitting inside an old 1020x670 rectangle
    // would defeat the Display-first migration by reopening at ~833x670.
    const auto minScale = static_cast<double> (minEditorHeight) / defaultEditorHeight;
    const auto maxScale = static_cast<double> (maxEditorHeight) / defaultEditorHeight;
    const auto savedScale = juce::jlimit (minScale, maxScale,
                                         static_cast<double> (juce::jmax (1, savedWidth)) / defaultEditorWidth);
    setSize (static_cast<int> (std::lround (defaultEditorWidth * savedScale)),
             static_cast<int> (std::lround (defaultEditorHeight * savedScale)));

    contentRoot.setInterceptsMouseClicks (false, true);
    addAndMakeVisible (contentRoot);

    title.setText ("QQ Super Compression", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centredLeft);
    title.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::plain)));
    title.setColour (juce::Label::textColourId, qqsc::ui::text());
    contentRoot.addAndMakeVisible (title);

    // Version is intentionally small and subdued: useful for screenshots/build
    // identification without becoming another explanatory subtitle. The string
    // comes from the CMake/JUCE plug-in version so UI and binary metadata match.
    versionLabel.setText (juce::String ("v") + JucePlugin_VersionString, juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredLeft);
    versionLabel.setColour (juce::Label::textColourId, qqsc::ui::textMuted().withAlpha (0.62f));
    versionLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    contentRoot.addAndMakeVisible (versionLabel);

    // The old explanatory subtitle was deliberately removed in 0.1.3 at the
    // user's request. Only the small build/version identifier remains.
    contentRoot.addAndMakeVisible (display);
    contentRoot.addAndMakeVisible (meters);

    configureLabel (inputGainLabel, "INPUT GAIN");
    configureLabel (ratioLabel, "RATIO");
    configureLabel (ratioChannel0Label, "L");
    configureLabel (ratioChannel1Label, "R");
    configureLabel (makeupLabel, "MAKEUP");
    configureLabel (makeupChannel0Label, "L");
    configureLabel (makeupChannel1Label, "R");
    configureLabel (mixLabel, "MIX");
    configureLabel (mixChannel0Label, "L");
    configureLabel (mixChannel1Label, "R");
    configureLabel (outputGainLabel, "OUTPUT GAIN");
    configureLabel (thresholdLabel, "THRESHOLD");
    configureLabel (thresholdChannel0Label, "L");
    configureLabel (thresholdChannel1Label, "R");
    configureLabel (modeLabel, "MODE");
    configureLabel (monitorLabel, "MONITOR");
    configureLabel (lookaheadLabel, "LOOKAHEAD (ms)");
    configureLabel (oversamplingLabel, "OVERSAMPLING");

    inputGainLabel.setColour (juce::Label::textColourId, qqsc::ui::textMuted());
    ratioLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.36f));
    makeupLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.36f));
    mixLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.36f));
    outputGainLabel.setColour (juce::Label::textColourId, qqsc::ui::textMuted());
    thresholdLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.38f));
    thresholdLabel.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    modeLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.42f));
    monitorLabel.setColour (juce::Label::textColourId, qqsc::ui::cyanAccent().darker (0.42f));
    lookaheadLabel.setColour (juce::Label::textColourId, qqsc::ui::cyanAccent().darker (0.42f));
    oversamplingLabel.setColour (juce::Label::textColourId, qqsc::ui::cyanAccent().darker (0.42f));

    for (auto* label : { &inputGainLabel, &ratioLabel, &ratioChannel0Label, &ratioChannel1Label,
                          &makeupLabel, &makeupChannel0Label, &makeupChannel1Label,
                          &mixLabel, &mixChannel0Label, &mixChannel1Label, &outputGainLabel, &thresholdLabel, &thresholdChannel0Label, &thresholdChannel1Label,
                          &modeLabel, &monitorLabel, &lookaheadLabel, &oversamplingLabel })
        contentRoot.addAndMakeVisible (*label);

    configureKnob (inputGainSlider, " dB");
    for (auto* ratio : { &ratioSlider, &ratioLSlider, &ratioRSlider, &ratioMSlider, &ratioSSlider })
    {
        configureKnob (*ratio);
        ratio->textFromValueFunction = [] (double v)
        {
            return juce::String (v, v < 10.0 ? 2 : 1) + ":1";
        };
    }

    for (auto* makeup : { &makeupSTSlider, &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider })
        configureKnob (*makeup, " dB");
    for (auto* mix : { &mixSlider, &mixLSlider, &mixRSlider, &mixMSlider, &mixSSlider })
        configureKnob (*mix, " %");
    configureKnob (outputGainSlider, " dB");

    // Threshold uses compact vertical faders beside the Display. ST shows one;
    // LR/MS show two independent faders. FineKnob provides true Shift fine drag.
    for (auto* threshold : { &thresholdSlider, &thresholdLSlider, &thresholdRSlider, &thresholdMSlider, &thresholdSSlider })
        configureThresholdSlider (*threshold);

    // Input/Output are secondary trims: smaller controls so Ratio / Makeup / Mix
    // remain the visual focus. Their text boxes stay directly editable.
    inputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 22);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 22);

    // Warm/transparent UI: musical gain controls use the warm lamp colour.
    inputGainSlider.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
    for (auto* ratio : { &ratioSlider, &ratioLSlider, &ratioRSlider, &ratioMSlider, &ratioSSlider })
        ratio->setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
    for (auto* mix : { &mixSlider, &mixLSlider, &mixRSlider, &mixMSlider, &mixSSlider })
        mix->setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
    outputGainSlider.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
    for (auto* makeup : { &makeupSTSlider, &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider })
        makeup->setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());

    for (auto* slider : { &inputGainSlider, &ratioSlider, &ratioLSlider, &ratioRSlider, &ratioMSlider, &ratioSSlider,
                          &makeupSTSlider, &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider,
                          &mixSlider, &mixLSlider, &mixRSlider, &mixMSlider, &mixSSlider, &outputGainSlider, &thresholdSlider, &thresholdLSlider, &thresholdRSlider,
                          &thresholdMSlider, &thresholdSSlider })
    {
        contentRoot.addAndMakeVisible (*slider);
        registerKeyboardListener (*slider);
    }

    lookaheadCombo.addItemList (qqsc::params::lookaheadChoices(), 1);
    lookaheadCombo.setJustificationType (juce::Justification::centred);
    lookaheadCombo.setColour (juce::ComboBox::backgroundColourId, qqsc::ui::panel());
    lookaheadCombo.setColour (juce::ComboBox::outlineColourId, qqsc::ui::border());
    lookaheadCombo.setColour (juce::ComboBox::textColourId, qqsc::ui::text());
    lookaheadCombo.setColour (juce::ComboBox::arrowColourId, qqsc::ui::cyanAccent().darker (0.30f));
    lookaheadCombo.setSelectedItemIndex (
        qqsc::params::lookaheadChoiceIndexForMs (
            processor.getAPVTS().getRawParameterValue (qqsc::params::lookaheadMs)->load()),
        juce::dontSendNotification);
    lookaheadCombo.onChange = [this] { commitLookaheadChoice(); };
    contentRoot.addAndMakeVisible (lookaheadCombo);
    registerKeyboardListener (lookaheadCombo);

    configureActionButton (modeButton);
    configureActionButton (linkButton);
    configureActionButton (monitorAllButton);
    configureActionButton (monitorFirstButton);
    configureActionButton (monitorSecondButton);
    configureActionButton (matchButton);
    configureActionButton (bypassButton);
    configureActionButton (aButton);
    configureActionButton (bButton);
    configureActionButton (aToBButton);
    configureActionButton (bToAButton);
    configureActionButton (oversamplingButton);

    // Current Mode is always an active state, so it gets the subtle warm lamp
    // treatment even though it is a cycle button rather than a toggle. The 0 ms
    // Oversampling selector uses cyan as a technical/analysis accent.
    modeButton.getProperties().set (juce::Identifier ("qqscAlwaysLit"), true);
    modeButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    oversamplingButton.getProperties().set (juce::Identifier ("qqscAlwaysLit"), true);
    oversamplingButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    aButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    bButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    bypassButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::outputAccent());

    aToBButton.setButtonText (arrowText ("A", "B"));
    bToAButton.setButtonText (arrowText ("B", "A"));

    bypassButton.setClickingTogglesState (true);
    linkButton.setClickingTogglesState (true);
    linkButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    linkButton.setTooltip ("Relative Link: Ratio / Threshold / Makeup / Mix");

    for (auto* monitorButton : { &monitorAllButton, &monitorFirstButton, &monitorSecondButton })
    {
        monitorButton->setClickingTogglesState (false);
        monitorButton->setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    }
    monitorAllButton.setTooltip ("Monitor the normal stereo result");
    monitorFirstButton.setTooltip ("Centered audition of L or M");
    monitorSecondButton.setTooltip ("Centered audition of R or S");

    aButton.setClickingTogglesState (false);
    bButton.setClickingTogglesState (false);

    for (auto* button : { &modeButton, &linkButton, &monitorAllButton, &monitorFirstButton, &monitorSecondButton,
                          &matchButton, &bypassButton, &aButton, &bButton, &aToBButton, &bToAButton, &oversamplingButton })
    {
        contentRoot.addAndMakeVisible (*button);
        registerKeyboardListener (*button);
    }

    auto& state = processor.getAPVTS();
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::inputGainDb, inputGainSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::ratio, ratioSlider);
    ratioLAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::ratioL, ratioLSlider);
    ratioRAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::ratioR, ratioRSlider);
    ratioMAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::ratioM, ratioMSlider);
    ratioSAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::ratioS, ratioSSlider);
    makeupSTAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainDb, makeupSTSlider);
    makeupLAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainLDb, makeupLSlider);
    makeupRAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainRDb, makeupRSlider);
    makeupMAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainMDb, makeupMSlider);
    makeupSAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainSDb, makeupSSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::mix, mixSlider);
    mixLAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::mixL, mixLSlider);
    mixRAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::mixR, mixRSlider);
    mixMAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::mixM, mixMSlider);
    mixSAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::mixS, mixSSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::outputGainDb, outputGainSlider);
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::thresholdDb, thresholdSlider);
    thresholdLAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::thresholdLDb, thresholdLSlider);
    thresholdRAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::thresholdRDb, thresholdRSlider);
    thresholdMAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::thresholdMDb, thresholdMSlider);
    thresholdSAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::thresholdSDb, thresholdSSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, qqsc::params::bypass, bypassButton);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, qqsc::params::domainLink, linkButton);

    inputGainSlider.onGestureStart = [this] { beginUndoTransaction ("Input Gain"); };
    ratioSlider.onGestureStart = [this] { beginUndoTransaction ("Ratio ST"); };
    makeupSTSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup ST"); };
    mixSlider.onGestureStart = [this] { beginUndoTransaction ("Mix ST"); };
    outputGainSlider.onGestureStart = [this] { beginUndoTransaction ("Output Gain"); };
    thresholdSlider.onGestureStart = [this] { beginUndoTransaction ("Threshold ST"); };

    ratioLSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::ratioLR, ratioLSlider, ratioRSlider, "Ratio L/R"); };
    ratioRSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::ratioLR, ratioRSlider, ratioLSlider, "Ratio L/R"); };
    ratioMSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::ratioMS, ratioMSlider, ratioSSlider, "Ratio M/S"); };
    ratioSSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::ratioMS, ratioSSlider, ratioMSlider, "Ratio M/S"); };

    thresholdLSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::thresholdLR, thresholdLSlider, thresholdRSlider, "Threshold L/R"); };
    thresholdRSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::thresholdLR, thresholdRSlider, thresholdLSlider, "Threshold L/R"); };
    thresholdMSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::thresholdMS, thresholdMSlider, thresholdSSlider, "Threshold M/S"); };
    thresholdSSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::thresholdMS, thresholdSSlider, thresholdMSlider, "Threshold M/S"); };

    makeupLSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::makeupLR, makeupLSlider, makeupRSlider, "Makeup L/R"); };
    makeupRSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::makeupLR, makeupRSlider, makeupLSlider, "Makeup L/R"); };
    makeupMSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::makeupMS, makeupMSlider, makeupSSlider, "Makeup M/S"); };
    makeupSSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::makeupMS, makeupSSlider, makeupMSlider, "Makeup M/S"); };

    mixLSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::mixLR, mixLSlider, mixRSlider, "Mix L/R"); };
    mixRSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::mixLR, mixRSlider, mixLSlider, "Mix L/R"); };
    mixMSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::mixMS, mixMSlider, mixSSlider, "Mix M/S"); };
    mixSSlider.onGestureStart = [this] { beginLinkedGesture (LinkedPair::mixMS, mixSSlider, mixMSlider, "Mix M/S"); };

    for (auto* slider : { &ratioLSlider, &ratioRSlider, &ratioMSlider, &ratioSSlider,
                          &thresholdLSlider, &thresholdRSlider, &thresholdMSlider, &thresholdSSlider,
                          &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider,
                          &mixLSlider, &mixRSlider, &mixMSlider, &mixSSlider })
        slider->onGestureEnd = [this] { endLinkedGesture(); };

    ratioLSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::ratioLR, ratioLSlider, ratioRSlider); };
    ratioRSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::ratioLR, ratioRSlider, ratioLSlider); };
    ratioMSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::ratioMS, ratioMSlider, ratioSSlider); };
    ratioSSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::ratioMS, ratioSSlider, ratioMSlider); };
    thresholdLSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::thresholdLR, thresholdLSlider, thresholdRSlider); };
    thresholdRSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::thresholdLR, thresholdRSlider, thresholdLSlider); };
    thresholdMSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::thresholdMS, thresholdMSlider, thresholdSSlider); };
    thresholdSSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::thresholdMS, thresholdSSlider, thresholdMSlider); };
    makeupLSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::makeupLR, makeupLSlider, makeupRSlider); };
    makeupRSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::makeupLR, makeupRSlider, makeupLSlider); };
    makeupMSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::makeupMS, makeupMSlider, makeupSSlider); };
    makeupSSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::makeupMS, makeupSSlider, makeupMSlider); };
    mixLSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::mixLR, mixLSlider, mixRSlider); };
    mixRSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::mixLR, mixRSlider, mixLSlider); };
    mixMSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::mixMS, mixMSlider, mixSSlider); };
    mixSSlider.onValueChange = [this] { handleLinkedValueChange (LinkedPair::mixMS, mixSSlider, mixMSlider); };

    // A JUCE Slider text edit does not travel through mouseDown/mouseDrag, so
    // the normal gesture-start snapshot used by relative LINK is not available.
    // Route the four paired parameter families through the same relative-delta
    // law at text-parse/commit time. This makes direct numeric entry behave like
    // normal dragging and Shift-fine dragging instead of silently bypassing LINK.
    ratioLSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::ratioLR, ratioLSlider, ratioRSlider, text, "Ratio L/R"); };
    ratioRSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::ratioLR, ratioRSlider, ratioLSlider, text, "Ratio L/R"); };
    ratioMSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::ratioMS, ratioMSlider, ratioSSlider, text, "Ratio M/S"); };
    ratioSSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::ratioMS, ratioSSlider, ratioMSlider, text, "Ratio M/S"); };

    thresholdLSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::thresholdLR, thresholdLSlider, thresholdRSlider, text, "Threshold L/R"); };
    thresholdRSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::thresholdLR, thresholdRSlider, thresholdLSlider, text, "Threshold L/R"); };
    thresholdMSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::thresholdMS, thresholdMSlider, thresholdSSlider, text, "Threshold M/S"); };
    thresholdSSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::thresholdMS, thresholdSSlider, thresholdMSlider, text, "Threshold M/S"); };

    makeupLSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::makeupLR, makeupLSlider, makeupRSlider, text, "Makeup L/R"); };
    makeupRSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::makeupLR, makeupRSlider, makeupLSlider, text, "Makeup L/R"); };
    makeupMSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::makeupMS, makeupMSlider, makeupSSlider, text, "Makeup M/S"); };
    makeupSSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::makeupMS, makeupSSlider, makeupMSlider, text, "Makeup M/S"); };

    mixLSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::mixLR, mixLSlider, mixRSlider, text, "Mix L/R"); };
    mixRSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::mixLR, mixRSlider, mixLSlider, text, "Mix L/R"); };
    mixMSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::mixMS, mixMSlider, mixSSlider, text, "Mix M/S"); };
    mixSSlider.valueFromTextFunction = [this] (const juce::String& text) { return handleLinkedTextEntry (LinkedPair::mixMS, mixSSlider, mixMSlider, text, "Mix M/S"); };

    modeButton.onClick = [this] { cycleMode(); };
    monitorAllButton.onClick = [this] { selectMonitor (qqsc::params::monitorAll); };
    monitorFirstButton.onClick = [this] { selectMonitor (qqsc::params::monitorFirst); };
    monitorSecondButton.onClick = [this] { selectMonitor (qqsc::params::monitorSecond); };
    oversamplingButton.onClick = [this] { cycleOversampling(); };
    matchButton.onClick = [this] { processor.applyMatchForCurrentMode(); };
    aButton.onClick = [this] { processor.selectABSlot (0); };
    bButton.onClick = [this] { processor.selectABSlot (1); };
    aToBButton.onClick = [this] { processor.copyAToB(); };
    bToAButton.onClick = [this] { processor.copyBToA(); };

    registerKeyboardListener (*this);
    setWantsKeyboardFocus (true);

    updateModeUi();
    updateOversamplingUi();
    startTimerHz (15);
}

QQSuperCompressionAudioProcessorEditor::~QQSuperCompressionAudioProcessorEditor()
{
    stopTimer();

    if (uiProperties != nullptr)
    {
        uiProperties->setValue ("editorWidth", getWidth());
        uiProperties->setValue ("editorHeight", getHeight());
        uiProperties->saveIfNeeded();
    }

    setLookAndFeel (nullptr);
}

std::unique_ptr<juce::PropertiesFile> QQSuperCompressionAudioProcessorEditor::createUiProperties()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "QQSuperCompression";
    options.filenameSuffix = ".settings";
    options.folderName = "Qing Audio";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
   #if JUCE_MAC
    options.osxLibrarySubFolder = "Application Support";
   #endif
    return std::make_unique<juce::PropertiesFile> (options);
}

void QQSuperCompressionAudioProcessorEditor::configureKnob (FineKnob& slider, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 24);
    slider.setTextValueSuffix (suffix);
    slider.setNumDecimalPlacesToDisplay (2);
    slider.setMouseDragSensitivity (180);
    slider.setColour (juce::Slider::textBoxTextColourId, qqsc::ui::text());
    slider.setColour (juce::Slider::textBoxBackgroundColourId, qqsc::ui::panel().withAlpha (0.76f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, qqsc::ui::border().withAlpha (0.78f));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, qqsc::ui::border());
}

void QQSuperCompressionAudioProcessorEditor::configureThresholdSlider (FineKnob& slider)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setSliderSnapsToMousePosition (false);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 21);
    slider.setNumDecimalPlacesToDisplay (2);
    slider.setColour (juce::Slider::backgroundColourId, qqsc::ui::panelAlt().withAlpha (0.72f));
    slider.setColour (juce::Slider::trackColourId, qqsc::ui::warmAccentSoft().withAlpha (0.72f));
    slider.setColour (juce::Slider::thumbColourId, qqsc::ui::warmAccent());
    slider.setColour (juce::Slider::textBoxTextColourId, qqsc::ui::text());
    slider.setColour (juce::Slider::textBoxBackgroundColourId, qqsc::ui::panel().withAlpha (0.80f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, qqsc::ui::border().withAlpha (0.78f));
    slider.textFromValueFunction = [] (double v)
    {
        return qqsc::params::isThresholdEnabled (static_cast<float> (v))
            ? juce::String (v, 2) + " dB" : juce::String ("OFF");
    };
    slider.valueFromTextFunction = [] (const juce::String& text)
    {
        return (text.containsIgnoreCase ("off") || text.containsIgnoreCase ("-inf"))
            ? static_cast<double> (qqsc::params::thresholdOffDb)
            : juce::jlimit (static_cast<double> (qqsc::params::thresholdOffDb), 0.0, text.getDoubleValue());
    };
}

void QQSuperCompressionAudioProcessorEditor::beginLinkedGesture (LinkedPair pair, FineKnob& source,
                                                                   FineKnob& target, const juce::String& undoName)
{
    beginUndoTransaction (undoName);
    activeLinkedPair = pair;
    activeLinkSource = &source;
    activeLinkTarget = &target;
    activeLinkSourceStart = source.getValue();
    activeLinkTargetStart = target.getValue();
}

void QQSuperCompressionAudioProcessorEditor::endLinkedGesture()
{
    activeLinkedPair = LinkedPair::none;
    activeLinkSource = nullptr;
    activeLinkTarget = nullptr;
}

void QQSuperCompressionAudioProcessorEditor::handleLinkedValueChange (LinkedPair pair, FineKnob& source, FineKnob& target)
{
    if (linkedValueUpdateInProgress || ! linkButton.getToggleState())
        return;

    // Link never equalises values. It preserves the pair's numeric difference
    // captured at gesture start: 3:1 / 5:1 -> +1 becomes 4:1 / 6:1;
    // -20 / -10 dB -> +2 becomes -18 / -8 dB. The same rule applies to Makeup
    // and Mix percentage points.
    if (activeLinkedPair != pair || activeLinkSource != &source || activeLinkTarget != &target)
        return;

    const bool thresholdPair = pair == LinkedPair::thresholdLR || pair == LinkedPair::thresholdMS;
    if (thresholdPair)
    {
        // OFF means -infinity conceptually, not -120 dB. A finite relative
        // offset to -infinity is undefined, so if exactly one side starts OFF
        // leave the OFF side untouched for that gesture. If both are OFF they
        // can be raised together normally.
        const bool sourceOff = ! qqsc::params::isThresholdEnabled (static_cast<float> (activeLinkSourceStart));
        const bool targetOff = ! qqsc::params::isThresholdEnabled (static_cast<float> (activeLinkTargetStart));
        if (sourceOff != targetOff)
            return;
    }

    const auto requestedDelta = source.getValue() - activeLinkSourceStart;
    const auto minDelta = juce::jmax (source.getMinimum() - activeLinkSourceStart,
                                      target.getMinimum() - activeLinkTargetStart);
    const auto maxDelta = juce::jmin (source.getMaximum() - activeLinkSourceStart,
                                      target.getMaximum() - activeLinkTargetStart);
    const auto appliedDelta = juce::jlimit (minDelta, maxDelta, requestedDelta);

    const auto newSource = activeLinkSourceStart + appliedDelta;
    const auto newTarget = activeLinkTargetStart + appliedDelta;

    const juce::ScopedValueSetter<bool> guard (linkedValueUpdateInProgress, true);
    if (std::abs (source.getValue() - newSource) > 1.0e-9)
        source.setValue (newSource, juce::sendNotificationSync);
    if (std::abs (target.getValue() - newTarget) > 1.0e-9)
        target.setValue (newTarget, juce::sendNotificationSync);
}

double QQSuperCompressionAudioProcessorEditor::handleLinkedTextEntry (LinkedPair pair, FineKnob& source,
                                                                        FineKnob& target,
                                                                        const juce::String& text,
                                                                        const juce::String& undoName)
{
    if (text.trim().isEmpty())
        return source.getValue();

    const bool thresholdPair = pair == LinkedPair::thresholdLR || pair == LinkedPair::thresholdMS;

    double requestedSource = 0.0;
    if (thresholdPair && (text.containsIgnoreCase ("off") || text.containsIgnoreCase ("-inf")))
        requestedSource = static_cast<double> (qqsc::params::thresholdOffDb);
    else
        requestedSource = text.getDoubleValue();

    requestedSource = juce::jlimit (source.getMinimum(), source.getMaximum(), requestedSource);

    // With LINK off, direct entry should remain an ordinary one-parameter edit.
    if (! linkButton.getToggleState())
        return requestedSource;

    const auto sourceStart = source.getValue();
    const auto targetStart = target.getValue();

    beginUndoTransaction (undoName);

    if (thresholdPair)
    {
        const bool sourceOff = ! qqsc::params::isThresholdEnabled (static_cast<float> (sourceStart));
        const bool targetOff = ! qqsc::params::isThresholdEnabled (static_cast<float> (targetStart));

        // OFF is conceptual -infinity. If only one side is OFF there is no
        // finite dB offset to preserve, so direct entry changes only the edited
        // side, matching the established drag-gesture rule.
        if (sourceOff != targetOff)
            return requestedSource;

        // If both sides are OFF, their finite difference is effectively zero.
        // Entering a finite Threshold on either side therefore brings both out
        // together at the entered value. Entering OFF simply leaves both OFF.
        if (sourceOff && targetOff)
        {
            const juce::ScopedValueSetter<bool> guard (linkedValueUpdateInProgress, true);
            if (std::abs (target.getValue() - requestedSource) > 1.0e-9)
                target.setValue (requestedSource, juce::sendNotificationSync);
            return requestedSource;
        }
    }

    // Numeric entry follows the identical relative-delta/boundary law as a
    // drag gesture. If the typed source value would push the paired member past
    // its range, clamp the *shared delta* rather than clipping just one member.
    const auto requestedDelta = requestedSource - sourceStart;
    const auto minDelta = juce::jmax (source.getMinimum() - sourceStart,
                                      target.getMinimum() - targetStart);
    const auto maxDelta = juce::jmin (source.getMaximum() - sourceStart,
                                      target.getMaximum() - targetStart);
    const auto appliedDelta = juce::jlimit (minDelta, maxDelta, requestedDelta);
    const auto newSource = sourceStart + appliedDelta;
    const auto newTarget = targetStart + appliedDelta;

    const juce::ScopedValueSetter<bool> guard (linkedValueUpdateInProgress, true);
    if (std::abs (target.getValue() - newTarget) > 1.0e-9)
        target.setValue (newTarget, juce::sendNotificationSync);

    return newSource;
}

void QQSuperCompressionAudioProcessorEditor::configureLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, qqsc::ui::textMuted().withAlpha (0.92f));
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
}

void QQSuperCompressionAudioProcessorEditor::configureActionButton (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, qqsc::ui::panel());
    button.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    button.setColour (juce::TextButton::textColourOffId, qqsc::ui::text().withAlpha (0.86f));
    button.setColour (juce::TextButton::textColourOnId, qqsc::ui::text());
}

void QQSuperCompressionAudioProcessorEditor::beginUndoTransaction (const juce::String& name)
{
    processor.getUndoManager().beginNewTransaction (name);
}

void QQSuperCompressionAudioProcessorEditor::registerKeyboardListener (juce::Component& component)
{
    component.addKeyListener (this);
}

bool QQSuperCompressionAudioProcessorEditor::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    const auto mods = key.getModifiers();
    const auto code = key.getKeyCode();

    if (mods.isCommandDown() && (code == 'Z' || code == 'z'))
    {
        if (mods.isShiftDown())
            processor.getUndoManager().redo();
        else
            processor.getUndoManager().undo();
        return true;
    }

    return false;
}

void QQSuperCompressionAudioProcessorEditor::setChoiceParameter (const char* parameterID, int value)
{
    if (auto* parameter = processor.getAPVTS().getParameter (parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (value)));
        parameter->endChangeGesture();
    }
}

void QQSuperCompressionAudioProcessorEditor::commitLookaheadChoice()
{
    const auto index = lookaheadCombo.getSelectedItemIndex();
    if (index < 0)
        return;

    const auto value = qqsc::params::lookaheadMsForChoiceIndex (index);
    const auto current = qqsc::params::snapLookaheadMs (
        processor.getAPVTS().getRawParameterValue (qqsc::params::lookaheadMs)->load());

    if (std::abs (current - value) > 0.0001f)
    {
        beginUndoTransaction ("Lookahead");
        if (auto* parameter = processor.getAPVTS().getParameter (qqsc::params::lookaheadMs))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
            parameter->endChangeGesture();
            processor.notifyHostProcessingLatency();
        }
    }

    updateOversamplingUi();

    // User preference, not project state: new instances use the last manually
    // selected preset, while an existing project still restores its own APVTS
    // Lookahead value when the host reloads the instance.
    if (uiProperties != nullptr)
    {
        uiProperties->setValue ("lastLookaheadMs", value);
        uiProperties->saveIfNeeded();
    }
}

void QQSuperCompressionAudioProcessorEditor::cycleOversampling()
{
    // This button is visible only at 0 ms. It deliberately cycles 1x -> 8x ->
    // 16x -> 1x. 2x/4x were tested and intentionally omitted because aliasing
    // remained severe while the latency cost of 8x/16x was small.
    const auto current = juce::jlimit (0, 2, juce::roundToInt (
        processor.getAPVTS().getRawParameterValue (qqsc::params::oversampling)->load()));
    const auto next = (current + 1) % 3;

    beginUndoTransaction ("0 ms Oversampling");
    setChoiceParameter (qqsc::params::oversampling, next);
    processor.notifyHostProcessingLatency();
    updateOversamplingUi();
}

void QQSuperCompressionAudioProcessorEditor::updateOversamplingUi()
{
    const auto currentLookaheadMs = qqsc::params::snapLookaheadMs (
        processor.getAPVTS().getRawParameterValue (qqsc::params::lookaheadMs)->load());
    const bool zeroMs = currentLookaheadMs < 0.0001f;

    const auto oversamplingIndex = juce::jlimit (0, 2, juce::roundToInt (
        processor.getAPVTS().getRawParameterValue (qqsc::params::oversampling)->load()));
    oversamplingButton.setButtonText (qqsc::params::oversamplingNameForChoiceIndex (oversamplingIndex));

    // Established transparent-engine rule: Oversampling is only meaningful at
    // 0 ms. 10/26/40/80/100 ms run 1x internally and hide the control while
    // preserving the user's remembered 0 ms choice.
    oversamplingLabel.setVisible (zeroMs);
    oversamplingButton.setVisible (zeroMs);
}

void QQSuperCompressionAudioProcessorEditor::selectMonitor (int selection)
{
    const auto mode = juce::jlimit (0, 2,
        juce::roundToInt (processor.getAPVTS().getRawParameterValue (qqsc::params::processingMode)->load()));

    if (mode == qqsc::params::leftRight || mode == qqsc::params::midSide)
        processor.setDomainMonitorSelection (mode, selection);

    updateMonitorUi();
}

void QQSuperCompressionAudioProcessorEditor::updateMonitorUi()
{
    const auto mode = juce::jlimit (0, 2,
        juce::roundToInt (processor.getAPVTS().getRawParameterValue (qqsc::params::processingMode)->load()));
    const bool lr = mode == qqsc::params::leftRight;
    const bool ms = mode == qqsc::params::midSide;
    const bool visible = lr || ms;

    monitorLabel.setVisible (visible);
    monitorAllButton.setVisible (visible);
    monitorFirstButton.setVisible (visible);
    monitorSecondButton.setVisible (visible);

    if (! visible)
        return;

    monitorFirstButton.setButtonText (lr ? "L" : "M");
    monitorSecondButton.setButtonText (lr ? "R" : "S");

    const auto selected = processor.getDomainMonitorSelection (mode);
    monitorAllButton.setToggleState (selected == qqsc::params::monitorAll, juce::dontSendNotification);
    monitorFirstButton.setToggleState (selected == qqsc::params::monitorFirst, juce::dontSendNotification);
    monitorSecondButton.setToggleState (selected == qqsc::params::monitorSecond, juce::dontSendNotification);
}

void QQSuperCompressionAudioProcessorEditor::cycleMode()
{
    const auto current = juce::roundToInt (processor.getAPVTS().getRawParameterValue (qqsc::params::processingMode)->load());
    const auto next = (juce::jlimit (0, 2, current) + 1) % 3;
    beginUndoTransaction ("Processing Mode");
    setChoiceParameter (qqsc::params::processingMode, next);
    updateModeUi();
}

void QQSuperCompressionAudioProcessorEditor::timerCallback()
{
    updateModeUi();

    bool latencyChoiceChangedOutsideUiGesture = false;

    const auto lookaheadIndex = qqsc::params::lookaheadChoiceIndexForMs (
        processor.getAPVTS().getRawParameterValue (qqsc::params::lookaheadMs)->load());
    if (lookaheadCombo.getSelectedItemIndex() != lookaheadIndex)
    {
        lookaheadCombo.setSelectedItemIndex (lookaheadIndex, juce::dontSendNotification);
        latencyChoiceChangedOutsideUiGesture = true;
    }

    const auto oversamplingIndex = juce::jlimit (0, 2, juce::roundToInt (
        processor.getAPVTS().getRawParameterValue (qqsc::params::oversampling)->load()));
    const auto currentButtonText = qqsc::params::oversamplingNameForChoiceIndex (oversamplingIndex);
    if (oversamplingButton.getButtonText() != currentButtonText)
        latencyChoiceChangedOutsideUiGesture = true;

    updateOversamplingUi();

    // Direct UI control changes notify immediately. This additional path covers
    // Undo/Redo, A/B recall and host-side parameter changes while the editor is
    // open, including when transport is stopped and no audio callback is running.
    if (latencyChoiceChangedOutsideUiGesture)
        processor.notifyHostProcessingLatency();
}

void QQSuperCompressionAudioProcessorEditor::updateModeUi()
{
    const auto mode = juce::jlimit (0, 2,
        juce::roundToInt (processor.getAPVTS().getRawParameterValue (qqsc::params::processingMode)->load()));

    modeButton.setButtonText (qqsc::params::modeName (mode));

    const bool st = mode == qqsc::params::stereoLinked;
    const bool lr = mode == qqsc::params::leftRight;
    const bool ms = mode == qqsc::params::midSide;

    ratioSlider.setVisible (st);
    ratioLSlider.setVisible (lr);
    ratioRSlider.setVisible (lr);
    ratioMSlider.setVisible (ms);
    ratioSSlider.setVisible (ms);
    ratioChannel0Label.setVisible (! st);
    ratioChannel1Label.setVisible (! st);

    thresholdSlider.setVisible (st);
    thresholdLSlider.setVisible (lr);
    thresholdRSlider.setVisible (lr);
    thresholdMSlider.setVisible (ms);
    thresholdSSlider.setVisible (ms);
    thresholdChannel0Label.setVisible (! st);
    thresholdChannel1Label.setVisible (! st);

    makeupSTSlider.setVisible (st);
    makeupLSlider.setVisible (lr);
    makeupRSlider.setVisible (lr);
    makeupMSlider.setVisible (ms);
    makeupSSlider.setVisible (ms);
    makeupChannel0Label.setVisible (! st);
    makeupChannel1Label.setVisible (! st);

    mixSlider.setVisible (st);
    mixLSlider.setVisible (lr);
    mixRSlider.setVisible (lr);
    mixMSlider.setVisible (ms);
    mixSSlider.setVisible (ms);
    mixChannel0Label.setVisible (! st);
    mixChannel1Label.setVisible (! st);

    // Keep Mode geometry fixed. v1.0.1 originally mutated Mode/LINK bounds from
    // timer-driven updateModeUi(), which could leave the buttons effectively
    // missing after mode changes in some hosts. resized() is now the sole owner
    // of their bounds; updateModeUi() only controls text/visibility/state.
    modeButton.setVisible (true);
    linkButton.setVisible (! st);
    updateMonitorUi();

    if (lr)
    {
        for (auto* label : { &ratioChannel0Label, &makeupChannel0Label, &mixChannel0Label, &thresholdChannel0Label })
            label->setText ("L", juce::dontSendNotification);
        for (auto* label : { &ratioChannel1Label, &makeupChannel1Label, &mixChannel1Label, &thresholdChannel1Label })
            label->setText ("R", juce::dontSendNotification);
    }
    else if (ms)
    {
        for (auto* label : { &ratioChannel0Label, &makeupChannel0Label, &mixChannel0Label, &thresholdChannel0Label })
            label->setText ("M", juce::dontSendNotification);
        for (auto* label : { &ratioChannel1Label, &makeupChannel1Label, &mixChannel1Label, &thresholdChannel1Label })
            label->setText ("S", juce::dontSendNotification);
    }

    matchButton.setEnabled (processor.hasMatchData());

    const auto active = processor.getActiveABSlot();
    aButton.setToggleState (active == 0, juce::dontSendNotification);
    bButton.setToggleState (active == 1, juce::dontSendNotification);
}

void QQSuperCompressionAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (qqsc::ui::canvas());

    const auto uiScale = static_cast<float> (getWidth()) / defaultEditorWidth;
    const auto scaled = [uiScale] (juce::Rectangle<float> r)
    {
        return juce::Rectangle<float> (r.getX() * uiScale, r.getY() * uiScale,
                                       r.getWidth() * uiScale, r.getHeight() * uiScale);
    };

    const auto headerHeight = 70.0f * uiScale;
    g.setColour (qqsc::ui::panel().withAlpha (0.92f));
    g.fillRect (0.0f, 0.0f, static_cast<float> (getWidth()), headerHeight);

    g.setColour (juce::Colours::black.withAlpha (0.035f));
    g.drawHorizontalLine (static_cast<int> (std::lround (headerHeight)),
                          0.0f, static_cast<float> (getWidth()));

    // Two quiet chassis panels behind the visual analysis and control rows.
    // They create the warm "instrument under glass" feeling without darkening
    // the product or suggesting heavy distortion/saturation.
    const auto visualPanel = scaled ({ 16.0f, 74.0f, 988.0f, 562.0f });
    const auto controlPanel = scaled ({ 16.0f, 640.0f, 988.0f, 162.0f });

    for (const auto panelRect : { visualPanel, controlPanel })
    {
        const auto corner = 15.0f * uiScale;

        // 0.9.1 material refinement: keep the exact panel geometry, but give the
        // warm ivory surface enough depth that the nearby lamp glows have a real
        // material to illuminate. The contrast remains deliberately very low.
        g.setColour (juce::Colours::black.withAlpha (0.040f));
        g.fillRoundedRectangle (panelRect.translated (0.0f, 2.4f * uiScale), corner);

        juce::ColourGradient panelGradient (qqsc::ui::panel().brighter (0.018f),
                                             panelRect.getCentreX(), panelRect.getY(),
                                             qqsc::ui::panelAlt().interpolatedWith (qqsc::ui::panel(), 0.77f),
                                             panelRect.getCentreX(), panelRect.getBottom(), false);
        g.setGradientFill (panelGradient);
        g.fillRoundedRectangle (panelRect, corner);

        // Thin bright upper rim + softer lower edge creates the translucent /
        // ceramic chassis character seen in the approved concept without adding
        // any new layout ornament.
        g.setColour (juce::Colours::white.withAlpha (0.58f));
        g.drawRoundedRectangle (panelRect.reduced (1.0f * uiScale),
                                juce::jmax (2.0f, corner - 1.0f * uiScale),
                                juce::jmax (0.7f, 0.9f * uiScale));
        g.setColour (qqsc::ui::border().withAlpha (0.55f));
        g.drawRoundedRectangle (panelRect, corner, juce::jmax (0.75f, uiScale));
    }
}

void QQSuperCompressionAudioProcessorEditor::resized()
{
    // Layout is calculated in the enlarged 1020x820 v1.0.1 design space. The
    // single parent transform still scales every child, font, stroke and meter
    // uniformly, preserving the established 1:1 X/Y resize behaviour.
    const auto uiScale = static_cast<float> (getWidth()) / defaultEditorWidth;
    contentRoot.setTransform (juce::AffineTransform());
    contentRoot.setBounds (0, 0, defaultEditorWidth, defaultEditorHeight);
    contentRoot.setTransform (juce::AffineTransform::scale (uiScale));

    const int margin = 22;
    const int headerButtonY = 20;
    const int headerButtonH = 30;
    const int smallGap = 6;

    int right = defaultEditorWidth - margin;

    bypassButton.setBounds (right - 88, headerButtonY, 88, headerButtonH);
    right -= 88 + 12;

    bToAButton.setBounds (right - 58, headerButtonY, 58, headerButtonH);
    right -= 58 + smallGap;
    aToBButton.setBounds (right - 58, headerButtonY, 58, headerButtonH);
    right -= 58 + smallGap;
    bButton.setBounds (right - 36, headerButtonY, 36, headerButtonH);
    right -= 36 + smallGap;
    aButton.setBounds (right - 36, headerButtonY, 36, headerButtonH);
    right -= 36 + 12;

    title.setBounds (margin, 16, juce::jmax (260, right - margin), 34);
    versionLabel.setBounds (margin + 2, 50, 72, 14);

    auto area = juce::Rectangle<int> (0, 0, defaultEditorWidth, defaultEditorHeight).reduced (margin);
    area.removeFromTop (58);

    // v1.0.1 is deliberately Display-first. Threshold decisions require enough
    // vertical resolution to read dynamics clearly, especially when LR/MS split
    // the history into two stacked domains.
    auto visualRow = area.removeFromTop (550);

    const int meterWidth = juce::jlimit (180, 200, visualRow.getWidth() / 5);
    meters.setBounds (visualRow.removeFromRight (meterWidth));
    visualRow.removeFromRight (8);

    // Threshold sits between Display and meters. ST uses one full-height fader.
    // LR/MS follow the Display's stacked visual grammar: top-domain Threshold
    // (L/M) above bottom-domain Threshold (R/S), instead of side-by-side faders.
    auto thresholdArea = visualRow.removeFromRight (76);
    thresholdLabel.setBounds (thresholdArea.removeFromTop (18));

    auto thresholdSTArea = thresholdArea;
    thresholdSlider.setBounds (thresholdSTArea.withSizeKeepingCentre (58, thresholdSTArea.getHeight()));

    auto thresholdFirst = thresholdArea.removeFromTop (thresholdArea.getHeight() / 2);
    auto thresholdSecond = thresholdArea;
    thresholdChannel0Label.setBounds (thresholdFirst.removeFromTop (14));
    thresholdChannel1Label.setBounds (thresholdSecond.removeFromTop (14));
    const auto thresholdFirstBounds = thresholdFirst.withSizeKeepingCentre (58, thresholdFirst.getHeight());
    const auto thresholdSecondBounds = thresholdSecond.withSizeKeepingCentre (58, thresholdSecond.getHeight());
    thresholdLSlider.setBounds (thresholdFirstBounds);
    thresholdMSlider.setBounds (thresholdFirstBounds);
    thresholdRSlider.setBounds (thresholdSecondBounds);
    thresholdSSlider.setBounds (thresholdSecondBounds);

    visualRow.removeFromRight (6);
    display.setBounds (visualRow);

    area.removeFromTop (10);

    // Controls remain compact and functionally unchanged; the extra editor height
    // is intentionally spent on Display/Meters rather than larger knobs.
    auto controls = area.removeFromTop (158);
    constexpr int controlGap = 6;
    constexpr int smallTrimW = 116;
    constexpr int modeW = 170;
    const int mainW = (controls.getWidth() - smallTrimW * 2 - modeW - controlGap * 5) / 3;

    auto inputArea = controls.removeFromLeft (smallTrimW);
    inputGainLabel.setBounds (inputArea.removeFromTop (18));
    inputGainSlider.setBounds (inputArea.withSizeKeepingCentre (100, 116));
    controls.removeFromLeft (controlGap);

    auto ratioArea = controls.removeFromLeft (mainW);
    ratioLabel.setBounds (ratioArea.removeFromTop (18));
    ratioSlider.setBounds (ratioArea.withSizeKeepingCentre (130, 119));

    auto dualRatio = ratioArea;
    auto ratioFirst = dualRatio.removeFromLeft (dualRatio.getWidth() / 2);
    auto ratioSecond = dualRatio;
    ratioChannel0Label.setBounds (ratioFirst.removeFromTop (14));
    ratioChannel1Label.setBounds (ratioSecond.removeFromTop (14));
    const auto ratioFirstKnob = ratioFirst.withSizeKeepingCentre (76, 100);
    const auto ratioSecondKnob = ratioSecond.withSizeKeepingCentre (76, 100);
    ratioLSlider.setBounds (ratioFirstKnob);
    ratioMSlider.setBounds (ratioFirstKnob);
    ratioRSlider.setBounds (ratioSecondKnob);
    ratioSSlider.setBounds (ratioSecondKnob);
    controls.removeFromLeft (controlGap);

    auto makeupArea = controls.removeFromLeft (mainW);
    auto makeupHeader = makeupArea.removeFromTop (20);
    auto matchArea = makeupHeader.removeFromRight (58).reduced (2, 0);
    makeupLabel.setBounds (makeupHeader);
    matchButton.setBounds (matchArea);

    makeupSTSlider.setBounds (makeupArea.withSizeKeepingCentre (130, 117));

    auto dualMakeup = makeupArea;
    const int dualW = dualMakeup.getWidth() / 2;
    auto first = dualMakeup.removeFromLeft (dualW);
    auto second = dualMakeup;

    makeupChannel0Label.setBounds (first.removeFromTop (14));
    makeupChannel1Label.setBounds (second.removeFromTop (14));

    const auto firstKnob = first.withSizeKeepingCentre (76, 100);
    const auto secondKnob = second.withSizeKeepingCentre (76, 100);
    makeupLSlider.setBounds (firstKnob);
    makeupMSlider.setBounds (firstKnob);
    makeupRSlider.setBounds (secondKnob);
    makeupSSlider.setBounds (secondKnob);
    controls.removeFromLeft (controlGap);

    auto mixArea = controls.removeFromLeft (mainW);
    mixLabel.setBounds (mixArea.removeFromTop (18));
    mixSlider.setBounds (mixArea.withSizeKeepingCentre (130, 119));

    auto dualMix = mixArea;
    auto mixFirst = dualMix.removeFromLeft (dualMix.getWidth() / 2);
    auto mixSecond = dualMix;
    mixChannel0Label.setBounds (mixFirst.removeFromTop (14));
    mixChannel1Label.setBounds (mixSecond.removeFromTop (14));
    const auto mixFirstKnob = mixFirst.withSizeKeepingCentre (76, 100);
    const auto mixSecondKnob = mixSecond.withSizeKeepingCentre (76, 100);
    mixLSlider.setBounds (mixFirstKnob);
    mixMSlider.setBounds (mixFirstKnob);
    mixRSlider.setBounds (mixSecondKnob);
    mixSSlider.setBounds (mixSecondKnob);
    controls.removeFromLeft (controlGap);

    auto outputArea = controls.removeFromLeft (smallTrimW);
    outputGainLabel.setBounds (outputArea.removeFromTop (18));
    outputGainSlider.setBounds (outputArea.withSizeKeepingCentre (100, 116));
    controls.removeFromLeft (controlGap);

    auto modeArea = controls;

    // v1.0.1 UI polish: Mode and Lookahead are the two primary controls in this
    // column, so their main control rectangles must be visually identical.
    // LINK is an auxiliary control that sits to the right of Mode and must not
    // steal width from the Mode button. Keep these fixed bounds owned only by
    // resized(); updateModeUi() changes visibility/state only.
    constexpr int primaryChoiceW = 108;
    constexpr int primaryChoiceH = 23;
    constexpr int linkChoiceW = 34;
    constexpr int linkChoiceGap = 6;
    const int choiceGroupW = primaryChoiceW + linkChoiceGap + linkChoiceW;
    const int choiceX = modeArea.getX() + juce::jmax (0, (modeArea.getWidth() - choiceGroupW) / 2);

    auto modeLabelArea = modeArea.removeFromTop (12);
    modeLabel.setBounds (choiceX, modeLabelArea.getY(), primaryChoiceW, modeLabelArea.getHeight());
    auto modeButtonRow = modeArea.removeFromTop (25);
    modeButton.setBounds (choiceX, modeButtonRow.getY() + 1, primaryChoiceW, primaryChoiceH);
    linkButton.setBounds (choiceX + primaryChoiceW + linkChoiceGap,
                          modeButtonRow.getY() + 1, linkChoiceW, primaryChoiceH);

    auto monitorLabelArea = modeArea.removeFromTop (12);
    monitorLabel.setBounds (choiceX, monitorLabelArea.getY(), primaryChoiceW, monitorLabelArea.getHeight());
    auto monitorButtonRow = modeArea.removeFromTop (25);
    constexpr int monitorButtonW = 34;
    constexpr int monitorButtonGap = 3;
    monitorAllButton.setBounds (choiceX, monitorButtonRow.getY() + 1, monitorButtonW, primaryChoiceH);
    monitorFirstButton.setBounds (choiceX + monitorButtonW + monitorButtonGap, monitorButtonRow.getY() + 1, monitorButtonW, primaryChoiceH);
    monitorSecondButton.setBounds (choiceX + (monitorButtonW + monitorButtonGap) * 2, monitorButtonRow.getY() + 1, monitorButtonW, primaryChoiceH);

    auto lookaheadLabelArea = modeArea.removeFromTop (12);
    lookaheadLabel.setBounds (choiceX, lookaheadLabelArea.getY(), primaryChoiceW, lookaheadLabelArea.getHeight());
    auto lookaheadComboRow = modeArea.removeFromTop (25);
    lookaheadCombo.setBounds (choiceX, lookaheadComboRow.getY() + 1, primaryChoiceW, primaryChoiceH);

    auto oversamplingLabelArea = modeArea.removeFromTop (12);
    oversamplingLabel.setBounds (choiceX, oversamplingLabelArea.getY(), primaryChoiceW, oversamplingLabelArea.getHeight());
    auto oversamplingButtonRow = modeArea.removeFromTop (25);
    oversamplingButton.setBounds (choiceX, oversamplingButtonRow.getY() + 1, primaryChoiceW, primaryChoiceH);
}
