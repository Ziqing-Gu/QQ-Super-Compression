#include "PluginEditor.h"
#include "Parameters.h"
#include <cmath>

namespace
{
constexpr int defaultEditorWidth = 1200;
constexpr int defaultEditorHeight = 800;
constexpr double editorAspectRatio = static_cast<double> (defaultEditorWidth) / defaultEditorHeight;
constexpr int minEditorHeight = 672;
constexpr int minEditorWidth = static_cast<int> (minEditorHeight * editorAspectRatio + 0.5);
constexpr int maxEditorHeight = 1200;
constexpr int maxEditorWidth = static_cast<int> (maxEditorHeight * editorAspectRatio + 0.5);

juce::String arrowText (const juce::String& left, const juce::String& right)
{
    const auto arrow = juce::String::fromUTF8 (u8"\u2192");
    return left + arrow + right;
}

juce::String ratioText (double value)
{
    return value < 1.0 ? "1:" + juce::String (1.0 / value, 2)
                       : juce::String (value, value < 10.0 ? 2 : 1) + ":1";
}

double ratioFromText (const juce::String& text)
{
    const auto trimmed = text.trim();
    for (const auto separator : { ':', '/' })
        if (trimmed.containsChar (separator))
        {
            const auto numerator = trimmed.upToFirstOccurrenceOf (juce::String::charToString (separator), false, false).getDoubleValue();
            const auto denominator = trimmed.fromFirstOccurrenceOf (juce::String::charToString (separator), false, false).getDoubleValue();
            return denominator > 0.0 ? numerator / denominator : 1.0;
        }
    return trimmed.getDoubleValue();
}

juce::Colour boundaryColour (bool dual, bool upper)
{
    if (upper) return qqsc::ui::grAccent();
    if (! dual) return qqsc::ui::warmAccent();
    return juce::Colour (qqsc::ui::isDarkTheme() ? 0xff80c897u : qqsc::ui::isClassicTheme() ? 0xff80d59cu : 0xff2c8452u);
}
}

QQSuperCompressionAudioProcessorEditor::QQSuperCompressionAudioProcessorEditor (QQSuperCompressionAudioProcessor& p,
    std::unique_ptr<juce::PropertiesFile> settingsOverride)
    : AudioProcessorEditor (&p),
      processor (p),
      uiProperties (settingsOverride != nullptr ? std::move (settingsOverride) : createUiProperties()),
      display (p),
      meters (p)
{
    processor.initialiseDualRatioLinkPreference (uiProperties == nullptr || uiProperties->getBoolValue ("dualRatioLink", true));
    setLookAndFeel (&utf8LookAndFeel);
    setResizable (true, true);
    setResizeLimits (minEditorWidth, minEditorHeight, maxEditorWidth, maxEditorHeight);
    if (auto* editorBoundsConstrainer = getConstrainer())
        editorBoundsConstrainer->setFixedAspectRatio (editorAspectRatio);

    int savedWidth = defaultEditorWidth;
    if (uiProperties != nullptr)
    {
        savedWidth = uiProperties->getIntValue ("landscapeEditorWidth", defaultEditorWidth);
    }

    // Give the 3:2 layout its own size preference. First open uses 1200x800;
    // subsequent opens preserve its uniform scale without changing legacy sizes.
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
    configureLabel (keySourceLabel, "SOURCE");
    configureLabel (keyGainLabel, "KEY GAIN");
    configureLabel (keyHpfLabel, "HPF");
    configureLabel (keyMeterLabel, "KEY LEVEL");
    keySourceLabel.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    keyGainLabel.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    keyHpfLabel.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
    keyMeterLabel.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));

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

    for (auto* label : { &keySourceLabel, &keyGainLabel, &keyHpfLabel, &keyMeterLabel })
        contentRoot.addAndMakeVisible (*label);

    configureKnob (inputGainSlider, " dB");
    for (auto* ratio : { &ratioSlider, &ratioLSlider, &ratioRSlider, &ratioMSlider, &ratioSSlider })
    {
        configureKnob (*ratio);
        ratio->textFromValueFunction = [] (double v)
        {
            return ratioText (v);
        };
    }

    for (auto* makeup : { &makeupSTSlider, &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider })
        configureKnob (*makeup, " dB");
    for (auto* mix : { &mixSlider, &mixLSlider, &mixRSlider, &mixMSlider, &mixSSlider })
        configureKnob (*mix, " %");
    configureKnob (outputGainSlider, " dB");
    configureKnob (keyGainSlider, " dB");
    keyGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
    configureKnob (keyHpfSlider);
    keyHpfSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
    keyHpfSlider.setNumDecimalPlacesToDisplay (0);
    keyHpfSlider.textFromValueFunction = [] (double value)
    {
        return qqsc::params::isKeyHpfEnabled (static_cast<float> (value))
            ? juce::String (std::round (value), 0) + " Hz"
            : juce::String ("OFF");
    };
    keyHpfSlider.valueFromTextFunction = [] (const juce::String& text)
    {
        if (text.containsIgnoreCase ("off"))
            return static_cast<double> (qqsc::params::keyHpfOffHz);

        const auto value = static_cast<float> (text.getDoubleValue());
        return static_cast<double> (qqsc::params::isKeyHpfEnabled (value)
            ? qqsc::params::clampKeyHpfHz (value)
            : qqsc::params::keyHpfOffHz);
    };
    keyHpfSlider.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::cyanAccent());
    keyHpfSlider.setTooltip ("Detector-only high-pass filter; OFF preserves the unfiltered key");

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
    contentRoot.addAndMakeVisible (keyGainSlider);
    registerKeyboardListener (keyGainSlider);
    contentRoot.addAndMakeVisible (keyHpfSlider);
    registerKeyboardListener (keyHpfSlider);

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
    configureActionButton (themeButton);
    configureActionButton (oversamplingButton);
    configureActionButton (sidechainButton);
    configureActionButton (keyInternalButton);
    configureActionButton (keyExternalButton);
    configureActionButton (sidechainListenButton);

    // Current Mode is always an active state, so it gets the subtle warm lamp
    // treatment even though it is a cycle button rather than a toggle. The 0 ms
    // Oversampling selector uses cyan as a technical/analysis accent.
    modeButton.getProperties().set (juce::Identifier ("qqscAlwaysLit"), true);
    modeButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    oversamplingButton.getProperties().set (juce::Identifier ("qqscAlwaysLit"), true);
    oversamplingButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    for (size_t d = 0; d < 5; ++d)
    {
        configureActionButton (upEnabledButtons[d]);
        configureActionButton (downEnabledButtons[d]);
    }
    aButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    bButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    bypassButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::outputAccent());
    themeButton.getProperties().set (juce::Identifier ("qqscAlwaysLit"), true);
    themeButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    themeButton.setTooltip ("Switch Light / Dark / Classic. Your last theme is remembered.");
    sidechainButton.getProperties().set (juce::Identifier ("qqscAlwaysLit"), true);
    sidechainButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    sidechainButton.setTooltip ("Open Side Chain source, gain, HPF and listen controls");
    keyInternalButton.setTooltip ("Use the post-Input-Gain main signal as detector key");
    keyExternalButton.setTooltip ("Use the optional external sidechain bus as detector key");
    sidechainListenButton.setClickingTogglesState (true);
    sidechainListenButton.setTooltip ("Audition the selected detector key at plug-in latency");

    keyMeter.setPercentageDisplay (false);
    keyMeter.setColour (juce::ProgressBar::backgroundColourId, qqsc::ui::panelAlt());
    keyMeter.setColour (juce::ProgressBar::foregroundColourId, qqsc::ui::cyanAccent());

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
    contentRoot.addAndMakeVisible (themeButton);
    registerKeyboardListener (themeButton);
    contentRoot.addAndMakeVisible (sidechainPanelBackground);
    for (auto* button : { &sidechainButton, &keyInternalButton, &keyExternalButton, &sidechainListenButton })
    {
        contentRoot.addAndMakeVisible (*button);
        registerKeyboardListener (*button);
    }
    contentRoot.addAndMakeVisible (keyMeter);

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
    keyGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::keyGainDb, keyGainSlider);
    keyHpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::keyHpfHz, keyHpfSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, qqsc::params::bypass, bypassButton);
    linkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, qqsc::params::domainLink, linkButton);

    inputGainSlider.onGestureStart = [this] { beginUndoTransaction ("Input Gain"); };
    ratioSlider.onGestureStart = [this] { beginUndoTransaction ("Ratio ST"); };
    makeupSTSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup ST"); };
    mixSlider.onGestureStart = [this] { beginUndoTransaction ("Mix ST"); };
    outputGainSlider.onGestureStart = [this] { beginUndoTransaction ("Output Gain"); };
    thresholdSlider.onGestureStart = [this] { beginUndoTransaction ("Threshold ST"); };
    keyGainSlider.onGestureStart = [this] { beginUndoTransaction ("Key Gain"); };
    keyHpfSlider.onGestureStart = [this]
    {
        beginUndoTransaction ("Side Chain HPF");
        display.beginKeyHpfGesture();
    };
    keyHpfSlider.onGestureEnd = [this] { display.endKeyHpfGesture(); };

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

    initialiseCompressionControls();
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
    themeButton.onClick = [this] { toggleTheme(); };
    sidechainButton.onClick = [this] { toggleSidechainPanel(); };
    keyInternalButton.onClick = [this] { setKeySource (qqsc::params::keyInternal); };
    keyExternalButton.onClick = [this] { setKeySource (qqsc::params::keyExternal); };
    sidechainListenButton.onClick = [this]
    {
        processor.setSidechainListenEnabled (sidechainListenButton.getToggleState());
        updateSidechainUi();
    };

    registerKeyboardListener (*this);
    setWantsKeyboardFocus (true);
    theme = uiProperties != nullptr ? qqsc::ui::themeFromPreferences (*uiProperties) : qqsc::ui::Theme::light;
    applyTheme();


    updateModeUi();
    updateOversamplingUi();
    updateSidechainUi();
    startTimerHz (15);
}

QQSuperCompressionAudioProcessorEditor::~QQSuperCompressionAudioProcessorEditor()
{
    finishCompressionControlGestures();
    stopTimer();
    processor.setSidechainListenEnabled (false);

    if (uiProperties != nullptr)
    {
        uiProperties->reload();
        uiProperties->setValue ("landscapeEditorWidth", getWidth());
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
void QQSuperCompressionAudioProcessorEditor::applyTheme()
{
    utf8LookAndFeel.setTheme (theme);
    const bool classicTheme = theme == qqsc::ui::Theme::classic;
    const bool darkTheme = theme == qqsc::ui::Theme::dark;

    const auto neutral = qqsc::ui::textMuted();
    const auto musical = (classicTheme || darkTheme) ? qqsc::ui::warmAccent()
                                      : qqsc::ui::warmAccent().darker (0.36f);
    const auto technical = (classicTheme || darkTheme) ? qqsc::ui::cyanAccent()
                                        : qqsc::ui::cyanAccent().darker (0.38f);

    title.setColour (juce::Label::textColourId, qqsc::ui::text());
    versionLabel.setColour (juce::Label::textColourId, neutral.withAlpha (0.62f));
   #if JUCE_WINDOWS
    const juce::String warmTitleFace = "Century Gothic";
   #elif JUCE_MAC
    const juce::String warmTitleFace = "Avenir Next";
   #else
    const juce::String warmTitleFace = juce::Font::getDefaultSansSerifFontName();
   #endif
    title.setFont (classicTheme ? juce::Font (juce::FontOptions (27.0f, juce::Font::plain))
                               : juce::Font (juce::FontOptions (warmTitleFace, 27.0f, juce::Font::plain)));

    for (auto* label : { &inputGainLabel, &ratioLabel, &ratioChannel0Label, &ratioChannel1Label,
                         &makeupLabel, &makeupChannel0Label, &makeupChannel1Label,
                         &mixLabel, &mixChannel0Label, &mixChannel1Label, &outputGainLabel,
                         &thresholdLabel, &thresholdChannel0Label, &thresholdChannel1Label,
                         &modeLabel, &monitorLabel, &lookaheadLabel, &oversamplingLabel,
                         &keySourceLabel, &keyGainLabel, &keyHpfLabel, &keyMeterLabel })
    {
        label->setColour (juce::Label::textColourId, neutral.withAlpha (0.92f));
        label->setFont (label->getFont().withStyle (classicTheme ? juce::Font::bold : juce::Font::plain));
    }

    inputGainLabel.setColour (juce::Label::textColourId, neutral);
    outputGainLabel.setColour (juce::Label::textColourId, neutral);
    ratioLabel.setColour (juce::Label::textColourId, musical);
    makeupLabel.setColour (juce::Label::textColourId, musical);
    mixLabel.setColour (juce::Label::textColourId, musical);
    thresholdLabel.setColour (juce::Label::textColourId, musical);
    modeLabel.setColour (juce::Label::textColourId, musical);
    monitorLabel.setColour (juce::Label::textColourId, technical);
    lookaheadLabel.setColour (juce::Label::textColourId, technical);
    oversamplingLabel.setColour (juce::Label::textColourId, technical);
    if (darkTheme)
        for (auto* label : { &ratioLabel, &makeupLabel, &mixLabel, &modeLabel })
            label->setColour (juce::Label::textColourId, qqsc::ui::text().withAlpha (0.90f));

    for (auto* slider : { &inputGainSlider, &ratioSlider, &ratioLSlider, &ratioRSlider, &ratioMSlider, &ratioSSlider,
                           &makeupSTSlider, &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider,
                           &mixSlider, &mixLSlider, &mixRSlider, &mixMSlider, &mixSSlider, &outputGainSlider,
                           &keyGainSlider, &keyHpfSlider })
    {
        slider->setColour (juce::Slider::textBoxTextColourId, qqsc::ui::text());
        slider->setColour (juce::Slider::textBoxBackgroundColourId, qqsc::ui::panel().withAlpha (classicTheme ? 0.96f : 0.76f));
        slider->setColour (juce::Slider::textBoxOutlineColourId, qqsc::ui::border().withAlpha (classicTheme ? 0.92f : 0.78f));
        slider->setColour (juce::Slider::rotarySliderOutlineColourId, qqsc::ui::border());
        slider->setColour (juce::Slider::rotarySliderFillColourId,
                            darkTheme ? juce::Colour (0xff9bd8ed) : qqsc::ui::warmAccent());
    }
    keyHpfSlider.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::cyanAccent());

    for (auto* threshold : { &thresholdSlider, &thresholdLSlider, &thresholdRSlider, &thresholdMSlider, &thresholdSSlider })
    {
        threshold->setColour (juce::Slider::backgroundColourId, qqsc::ui::panelAlt().withAlpha (classicTheme ? 0.90f : 0.72f));
        threshold->setColour (juce::Slider::trackColourId, qqsc::ui::warmAccentSoft().withAlpha (0.72f));
        threshold->setColour (juce::Slider::thumbColourId, qqsc::ui::warmAccent());
        threshold->setColour (juce::Slider::textBoxTextColourId, qqsc::ui::text());
        threshold->setColour (juce::Slider::textBoxBackgroundColourId, qqsc::ui::panel().withAlpha (classicTheme ? 0.96f : 0.80f));
        threshold->setColour (juce::Slider::textBoxOutlineColourId, qqsc::ui::border().withAlpha (classicTheme ? 0.92f : 0.78f));
    }

    lookaheadCombo.setColour (juce::ComboBox::backgroundColourId, qqsc::ui::panel());
    lookaheadCombo.setColour (juce::ComboBox::outlineColourId, qqsc::ui::border());
    lookaheadCombo.setColour (juce::ComboBox::textColourId, qqsc::ui::text());
    lookaheadCombo.setColour (juce::ComboBox::arrowColourId, technical);

    for (auto* button : { &modeButton, &linkButton, &dualRatioLinkButton, &monitorAllButton, &monitorFirstButton, &monitorSecondButton,
                          &matchButton, &bypassButton, &aButton, &bButton, &aToBButton, &bToAButton,
                          &themeButton, &oversamplingButton, &sidechainButton, &keyInternalButton,
                          &keyExternalButton, &sidechainListenButton })
    {
        button->setColour (juce::TextButton::buttonColourId, qqsc::ui::panel());
        button->setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
        button->setColour (juce::TextButton::textColourOffId, qqsc::ui::text().withAlpha (0.86f));
        button->setColour (juce::TextButton::textColourOnId, qqsc::ui::text());
    }

    oversamplingButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    monitorAllButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    monitorFirstButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    monitorSecondButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    themeButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    sidechainButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    keyInternalButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    keyExternalButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());
    sidechainListenButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::outputAccent());
    compressionModeButton.setColour (juce::TextButton::buttonColourId, qqsc::ui::panel());
    compressionModeButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());
    compressionModeButton.setColour (juce::TextButton::textColourOffId, qqsc::ui::text());
    compressionModeButton.setColour (juce::TextButton::textColourOnId, qqsc::ui::text());
    for (size_t i = 0; i < upperBoundarySliders.size(); ++i)
    {
        if (upperBoundarySliders[i] == nullptr) continue;
        for (auto* label : { &lowerBoundaryNames[i], &upperBoundaryNames[i], &upRatioNames[i], &downRatioNames[i] })
            label->setColour (juce::Label::textColourId, neutral);
        for (auto* label : { &lowerBoundaryValues[i], &upperBoundaryValues[i] })
        {
            label->setColour (juce::Label::textColourId, qqsc::ui::text());
            label->setColour (juce::Label::backgroundColourId, qqsc::ui::panel().withAlpha (0.80f));
            label->setColour (juce::Label::outlineColourId, qqsc::ui::border().withAlpha (0.78f));
            label->setColour (juce::Label::textWhenEditingColourId, qqsc::ui::text());
            label->setColour (juce::Label::backgroundWhenEditingColourId, qqsc::ui::panel());
        }
        auto& ratio = *downRatioSliders[i];
        ratio.setColour (juce::Slider::textBoxTextColourId, qqsc::ui::text());
        ratio.setColour (juce::Slider::textBoxBackgroundColourId, qqsc::ui::panel().withAlpha (0.76f));
        ratio.setColour (juce::Slider::textBoxOutlineColourId, qqsc::ui::border().withAlpha (0.78f));
        ratio.setColour (juce::Slider::rotarySliderOutlineColourId, qqsc::ui::border());
        ratio.setColour (juce::Slider::rotarySliderFillColourId, darkTheme ? juce::Colour (0xff9bd8ed) : qqsc::ui::warmAccent());
    }
    bypassButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::outputAccent());
    themeButton.setButtonText (qqsc::ui::themeLabel (theme));
    keyMeter.setColour (juce::ProgressBar::backgroundColourId, qqsc::ui::panelAlt());
    keyMeter.setColour (juce::ProgressBar::foregroundColourId, qqsc::ui::cyanAccent());

    display.repaint();
    meters.repaint();
    contentRoot.repaint();
    updateSidechainUi();
    repaint();
}

void QQSuperCompressionAudioProcessorEditor::toggleTheme()
{
    theme = theme == qqsc::ui::Theme::light ? qqsc::ui::Theme::dark
          : (theme == qqsc::ui::Theme::dark ? qqsc::ui::Theme::classic : qqsc::ui::Theme::light);
    if (uiProperties != nullptr)
    {
        uiProperties->setValue ("uiTheme", qqsc::ui::themeKey (theme));
        // Retain a useful preference if the user rolls back to a two-skin build.
        uiProperties->setValue ("classicTheme", theme != qqsc::ui::Theme::light);
        uiProperties->saveIfNeeded();
    }

    applyTheme();
}

void QQSuperCompressionAudioProcessorEditor::toggleSidechainPanel()
{
    sidechainPanelOpen = ! sidechainPanelOpen;

    // Never leave an audition override hidden behind a closed popup.
    if (! sidechainPanelOpen)
        processor.setSidechainListenEnabled (false);

    updateSidechainUi();
}

void QQSuperCompressionAudioProcessorEditor::setKeySource (int source)
{
    source = juce::jlimit (static_cast<int> (qqsc::params::keyInternal),
                           static_cast<int> (qqsc::params::keyExternal), source);
    const auto current = juce::roundToInt (
        processor.getAPVTS().getRawParameterValue (qqsc::params::keySource)->load());

    if (source != current)
    {
        beginUndoTransaction ("Key Source");
        setChoiceParameter (qqsc::params::keySource, source);
    }

    updateSidechainUi();
}

void QQSuperCompressionAudioProcessorEditor::updateSidechainUi()
{
    const auto source = juce::jlimit (0, 1, juce::roundToInt (
        processor.getAPVTS().getRawParameterValue (qqsc::params::keySource)->load()));
    const bool external = source == qqsc::params::keyExternal;
    const bool available = processor.isExternalSidechainBusAvailable();
    const bool listening = processor.isSidechainListenEnabled();

    sidechainButton.setButtonText (external ? "SC: EXT" : "SC: INT");
    sidechainButton.setToggleState (sidechainPanelOpen, juce::dontSendNotification);
    keyInternalButton.setToggleState (! external, juce::dontSendNotification);
    keyExternalButton.setToggleState (external, juce::dontSendNotification);
    sidechainListenButton.setToggleState (listening, juce::dontSendNotification);
    keyGainSlider.setEnabled (external);

    const auto keyDb = processor.getMeterState().keyInputDb.load (std::memory_order_relaxed);
    if (external && ! available)
    {
        keyMeterProgress = 0.0;
        keyMeter.setTextToDisplay ("N/A");
    }
    else
    {
        keyMeterProgress = juce::jlimit (0.0, 1.0, (static_cast<double> (keyDb) + 60.0) / 60.0);
        keyMeter.setTextToDisplay (keyDb <= -119.0f ? "-inf dB" : juce::String (keyDb, 1) + " dB");
    }

    sidechainPanelBackground.setVisible (sidechainPanelOpen);
    if (sidechainPanelOpen)
        sidechainPanelBackground.toFront (false);

    for (auto* component : { static_cast<juce::Component*> (&keySourceLabel),
                              static_cast<juce::Component*> (&keyGainLabel),
                              static_cast<juce::Component*> (&keyHpfLabel),
                              static_cast<juce::Component*> (&keyMeterLabel),
                              static_cast<juce::Component*> (&keyInternalButton),
                              static_cast<juce::Component*> (&keyExternalButton),
                              static_cast<juce::Component*> (&sidechainListenButton),
                              static_cast<juce::Component*> (&keyGainSlider),
                              static_cast<juce::Component*> (&keyHpfSlider),
                              static_cast<juce::Component*> (&keyMeter) })
    {
        component->setVisible (sidechainPanelOpen);
        if (sidechainPanelOpen)
            component->toFront (false);
    }

    sidechainButton.toFront (false);
    themeButton.toFront (false);
    repaint();
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
    const auto targetID = target.getProperties()["qqscParameterID"].toString();
    if (linkButton.getToggleState() && targetID.isNotEmpty())
        if (auto* parameter = processor.getAPVTS().getParameter (targetID))
        {
            parameter->beginChangeGesture();
            companionGestureParameters.push_back (parameter);
        }
}

void QQSuperCompressionAudioProcessorEditor::endLinkedGesture()
{
    dualRatioGestureActive = false;
    for (auto* parameter : companionGestureParameters)
        parameter->endChangeGesture();
    companionGestureParameters.clear();
    boundaryGestureActive = false;
    activeLinkedPair = LinkedPair::none;
    activeLinkSource = nullptr;
    activeLinkTarget = nullptr;
}

void QQSuperCompressionAudioProcessorEditor::beginDualRatioGesture (int domain, bool upward)
{
    endLinkedGesture();
    beginUndoTransaction ("Dual Ratio Link");
    dualRatioGestureActive = true;
    dualRatioGestureDomain = domain;
    dualRatioGestureUpward = upward;
    dualRatioGestureCoupled = dualRatioLinkButton.getToggleState();
    dualRatioGesturePartner = domain != 0 && linkButton.getToggleState()
        ? (domain % 2 == 1 ? domain + 1 : domain - 1) : domain;
    const auto up = mainRatioControls();
    for (size_t d = 0; d < 5; ++d)
    {
        dualRatioStartUp[d] = up[d]->getValue();
        dualRatioStartDown[d] = downRatioSliders[d]->getValue();
    }
    for (int d = 0; d < 5; ++d)
        if (d == domain || d == dualRatioGesturePartner)
            for (bool isUp : {true, false})
                if ((isUp == upward || dualRatioGestureCoupled) && !(d == domain && isUp == upward))
                {
                    auto* slider = isUp ? up[static_cast<size_t> (d)] : downRatioSliders[static_cast<size_t> (d)].get();
                    if (auto* parameter = processor.getAPVTS().getParameter (slider->getProperties()["qqscParameterID"].toString()))
                    {
                        parameter->beginChangeGesture();
                        companionGestureParameters.push_back (parameter);
                    }
                }
}

double QQSuperCompressionAudioProcessorEditor::applyDualRatioChange (double requested, bool writeSource)
{
    const juce::ScopedValueSetter<bool> guard (dualRatioValueUpdateInProgress, true);
    const auto up = mainRatioControls();
    const auto& starts = dualRatioGestureUpward ? dualRatioStartUp : dualRatioStartDown;
    double minDelta = -std::numeric_limits<double>::infinity();
    double maxDelta = std::numeric_limits<double>::infinity();
    for (int d = 0; d < 5; ++d)
        if (d == dualRatioGestureDomain || d == dualRatioGesturePartner)
        {
            const auto i = static_cast<size_t> (d);
            auto* driven = dualRatioGestureUpward ? up[i] : downRatioSliders[i].get();
            auto* opposite = dualRatioGestureUpward ? downRatioSliders[i].get() : up[i];
            auto minimum = driven->getMinimum(), maximum = driven->getMaximum();
            if (dualRatioGestureCoupled)
            {
                // Preserve UP*DOWN. Bound the shared edit before writing any
                // member, including an independently offset LR/MS partner.
                const auto product = dualRatioStartUp[i] * dualRatioStartDown[i];
                minimum = juce::jmax (minimum, product / opposite->getMaximum());
                maximum = juce::jmin (maximum, product / opposite->getMinimum());
            }
            minDelta = juce::jmax (minDelta, minimum - starts[i]);
            maxDelta = juce::jmin (maxDelta, maximum - starts[i]);
        }
    const auto delta = juce::jlimit (minDelta, maxDelta, requested - starts[static_cast<size_t> (dualRatioGestureDomain)]);
    for (int d = 0; d < 5; ++d)
        if (d == dualRatioGestureDomain || d == dualRatioGesturePartner)
        {
            const auto i = static_cast<size_t> (d);
            auto* driven = dualRatioGestureUpward ? up[i] : downRatioSliders[i].get();
            auto* opposite = dualRatioGestureUpward ? downRatioSliders[i].get() : up[i];
            const auto next = starts[i] + delta;
            if (writeSource || d != dualRatioGestureDomain)
                driven->setValue (next, juce::sendNotificationSync);
            if (dualRatioGestureCoupled)
                opposite->setValue (dualRatioStartUp[i] * dualRatioStartDown[i] / next, juce::sendNotificationSync);
        }
    return starts[static_cast<size_t> (dualRatioGestureDomain)] + delta;
}

void QQSuperCompressionAudioProcessorEditor::handleDualRatioChange (int domain, bool upward)
{
    // Host automation/state restoration does not rewrite the other parameter.
    // Coupling applies to explicit editor gestures and numeric commits only.
    if (dualRatioValueUpdateInProgress || ! dualRatioGestureActive
        || domain != dualRatioGestureDomain || upward != dualRatioGestureUpward) return;
    auto* source = upward ? mainRatioControls()[static_cast<size_t> (domain)] : downRatioSliders[static_cast<size_t> (domain)].get();
    applyDualRatioChange (source->getValue(), true);
}

double QQSuperCompressionAudioProcessorEditor::dualRatioFromText (int domain, bool upward, const juce::String& text)
{
    auto* source = upward ? mainRatioControls()[static_cast<size_t> (domain)] : downRatioSliders[static_cast<size_t> (domain)].get();
    const auto requested = juce::jlimit (source->getMinimum(), source->getMaximum(), ratioFromText (text));
    if (dualRatioValueUpdateInProgress) return requested;
    beginDualRatioGesture (domain, upward);
    const auto result = applyDualRatioChange (requested, false);
    endLinkedGesture();
    return result;
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

    const bool rangePair = attachedCompressionMode == 0
        && (pair == LinkedPair::upperBoundaryLR || pair == LinkedPair::upperBoundaryMS);
    if (rangePair)
    {
        const bool sourceOff = ! qqsc::params::isRangeEnabled (static_cast<float> (activeLinkSourceStart));
        const bool targetOff = ! qqsc::params::isRangeEnabled (static_cast<float> (activeLinkTargetStart));
        // OFF is an absent ceiling, not +1 physical dB. A finite offset to
        // that endpoint is undefined, just like the lower -infinity endpoint.
        if (sourceOff != targetOff) return;
        if (sourceOff || ! qqsc::params::isRangeEnabled (static_cast<float> (source.getValue())))
        {
            const juce::ScopedValueSetter<bool> guard (linkedValueUpdateInProgress, true);
            target.setValue (source.getValue(), juce::sendNotificationSync);
            return;
        }
    }

    const bool thresholdPair = attachedCompressionMode == 0
        && (pair == LinkedPair::thresholdLR || pair == LinkedPair::thresholdMS);
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

    const bool thresholdPair = attachedCompressionMode == 0
        && (pair == LinkedPair::thresholdLR || pair == LinkedPair::thresholdMS);
    const bool ratioPair = pair == LinkedPair::ratioLR || pair == LinkedPair::ratioMS
        || pair == LinkedPair::downRatioLR || pair == LinkedPair::downRatioMS;

    double requestedSource = 0.0;
    if (thresholdPair && (text.containsIgnoreCase ("off") || text.containsIgnoreCase ("-inf")))
        requestedSource = static_cast<double> (qqsc::params::thresholdOffDb);
    else
        requestedSource = ratioPair ? ratioFromText (text) : text.getDoubleValue();

    requestedSource = juce::jlimit (source.getMinimum(), source.getMaximum(), requestedSource);

    // With LINK off, direct entry should remain an ordinary one-parameter edit.
    if (! linkButton.getToggleState())
        return requestedSource;

    const auto sourceStart = source.getValue();
    const auto targetStart = target.getValue();

    beginUndoTransaction (undoName);
    const auto setLinkedTarget = [this, &target] (double value)
    {
        if (std::abs (target.getValue() - value) <= 1.0e-9)
            return;

        // Native Slider text parsing occurs before the source's automatic
        // ScopedDragNotification. Give the linked target its own balanced
        // touch gesture so hosts also record this parameter's numeric edit.
        const auto targetID = target.getProperties()["qqscParameterID"].toString();
        auto* parameter = targetID.isNotEmpty() ? processor.getAPVTS().getParameter (targetID) : nullptr;
        const bool companionAlreadyActive = parameter != nullptr
            && std::find (companionGestureParameters.begin(), companionGestureParameters.end(), parameter) != companionGestureParameters.end();
        const bool openGesture = parameter != nullptr && ! companionAlreadyActive && ! target.hasActiveNativeGesture();
        if (openGesture) parameter->beginChangeGesture();
        target.setValue (value, juce::sendNotificationSync);
        if (openGesture) parameter->endChangeGesture();
    };

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
            setLinkedTarget (requestedSource);
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
    setLinkedTarget (newTarget);

    return newSource;
}

void QQSuperCompressionAudioProcessorEditor::BoundaryLookAndFeel::drawLinearSlider (
    juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float, float,
    juce::Slider::SliderStyle, juce::Slider& slider)
{
    const auto upper = static_cast<bool> (slider.getProperties()["qqscUpperBoundary"]);
    const auto accent = boundaryColour (static_cast<bool> (slider.getProperties()["qqscDualBoundary"]), upper);
    const auto cx = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    auto track = juce::Rectangle<float> (cx - 2.2f, static_cast<float> (y), 4.4f, static_cast<float> (height));
    if (auto* boundary = dynamic_cast<FineKnob*> (&slider); boundary != nullptr && boundary->boundaryPlotBounds)
    {
        const auto plot = boundary->boundaryPlotBounds();
        track.setY (plot.getY());
        track.setHeight (plot.getHeight());
        sliderPos = boundary->getBoundaryThumbY();
    }
    g.setColour (qqsc::ui::isDarkTheme() ? juce::Colour (0xff414445) : juce::Colour (0xffd8d0c5));
    g.fillRoundedRectangle (track, 2.2f);
    g.setColour (accent.withAlpha (qqsc::ui::isDarkTheme() ? 0.42f : 0.70f));
    g.fillRoundedRectangle (upper ? track.withBottom (sliderPos) : track.withTop (sliderPos), 2.0f);
    const auto thumb = juce::Rectangle<float> (19.0f, 11.0f).withCentre ({ cx, sliderPos });
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (thumb.translated (0.0f, 1.0f).expanded (0.6f), 4.0f);
    if (qqsc::ui::isDarkTheme()) qqsc::dark::surface (g, thumb, 4.0f);
    else if (! qqsc::ui::isClassicTheme()) qqsc::warm::surface (g, thumb, 4.0f);
    else
    {
        g.setColour (qqsc::ui::panel());
        g.fillRoundedRectangle (thumb, 3.0f);
        g.setColour (qqsc::ui::border());
        g.drawRoundedRectangle (thumb, 3.0f, 1.0f);
    }
    g.setColour (accent);
    g.drawLine (cx - 5.5f, sliderPos, cx + 5.5f, sliderPos, 1.3f);
}

std::array<QQSuperCompressionAudioProcessorEditor::FineKnob*, 5>
QQSuperCompressionAudioProcessorEditor::lowerBoundaryControls()
{
    return { &thresholdSlider, &thresholdLSlider, &thresholdRSlider, &thresholdMSlider, &thresholdSSlider };
}

std::array<QQSuperCompressionAudioProcessorEditor::FineKnob*, 5>
QQSuperCompressionAudioProcessorEditor::mainRatioControls()
{
    return { &ratioSlider, &ratioLSlider, &ratioRSlider, &ratioMSlider, &ratioSSlider };
}

void QQSuperCompressionAudioProcessorEditor::initialiseCompressionControls()
{
    configureActionButton (dualRatioLinkButton);
    dualRatioLinkButton.getProperties().set ("qqscSmallLink", true);
    dualRatioLinkButton.setTooltip ("Link UP/DOWN by inverse relative changes. Enabling LINK keeps current values. LR/MS domain LINK remains separate.");
    contentRoot.addAndMakeVisible (dualRatioLinkButton);
    registerKeyboardListener (dualRatioLinkButton);
    dualRatioLinkButton.onClick = [this]
    {
        finishCompressionControlGestures();
        beginUndoTransaction ("Up / Down Ratio Link");
        const bool enabled = processor.getAPVTS().getRawParameterValue (qqsc::params::dualRatioLink)->load() < 0.5f;
        setChoiceParameter (qqsc::params::dualRatioLink, enabled ? 1 : 0);
        if (uiProperties != nullptr)
        {
            uiProperties->reload();
            uiProperties->setValue ("dualRatioLink", enabled);
            uiProperties->saveIfNeeded();
        }
        updateCompressionUi();
    };
    for (size_t d = 0; d < 5; ++d)
        for (bool upward : { true, false })
        {
            auto& button = upward ? upEnabledButtons[d] : downEnabledButtons[d];
            configureActionButton (button);
            button.getProperties().set ("qqscSmallLink", true);
            button.setTooltip (upward ? "Enable upward processing; keeps Up Ratio and both thresholds."
                                     : "Enable downward processing; keeps Down Ratio and both thresholds.");
            contentRoot.addAndMakeVisible (button);
            registerKeyboardListener (button);
            button.onClick = [this, d, upward]
            {
                const auto* id = (upward ? qqsc::params::upEnabledIds : qqsc::params::downEnabledIds)[d];
                beginUndoTransaction (upward ? "Up processing On/Off" : "Down processing On/Off");
                const bool enabled = processor.getAPVTS().getRawParameterValue (id)->load() < 0.5f;
                setChoiceParameter (id, enabled ? 1 : 0);
                updateCompressionUi();
            };
        }

    configureActionButton (compressionModeButton);
    compressionModeButton.getProperties().set ("qqscAlwaysLit", true);
    compressionModeButton.setTooltip ("Single: process between Threshold and Range. Dual: independent upward and downward compression.");
    contentRoot.addAndMakeVisible (compressionModeButton);
    registerKeyboardListener (compressionModeButton);
    compressionModeButton.onClick = [this]
    {
        endLinkedGesture();
        beginUndoTransaction ("Single / Dual Compression");
        setChoiceParameter ("compressionMode", attachedCompressionMode == 1 ? 0 : 1);
        updateCompressionUi();
        updateModeUi();
    };
    linkButton.setTooltip ("Relative Link: Ratio / Threshold / Range / Makeup / Mix");
    const auto lower = lowerBoundaryControls();
    const auto mainRatios = mainRatioControls();
    for (size_t i = 0; i < lower.size(); ++i)
    {
        upperBoundarySliders[i] = std::make_unique<FineKnob> (0.0);
        downRatioSliders[i] = std::make_unique<FineKnob> (1.0);
        auto& upper = *upperBoundarySliders[i];
        configureThresholdSlider (upper);
        configureKnob (*downRatioSliders[i]);
        downRatioSliders[i]->textFromValueFunction = ratioText;
        downRatioSliders[i]->valueFromTextFunction = ratioFromText;
        for (auto* slider : { lower[i], &upper })
        {
            slider->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            slider->setLookAndFeel (&boundaryLookAndFeel);
            slider->getProperties().set ("qqscUpperBoundary", slider == &upper);
            slider->boundaryPlotBounds = [this, i, slider]
            {
                return display.getBoundaryPlotForDomain (static_cast<int> (i)).translated (
                    static_cast<float> (display.getX() - slider->getX()),
                    static_cast<float> (display.getY() - slider->getY()));
            };
            slider->boundaryDbToY = [this, i, slider] (float db)
            {
                return display.getBoundaryYForDomainDb (static_cast<int> (i), db)
                    + static_cast<float> (display.getY() - slider->getY());
            };
            slider->boundaryYToDb = [this, i, slider] (float localY)
            {
                return display.getBoundaryDbForY (static_cast<int> (i), localY
                    + static_cast<float> (slider->getY() - display.getY()));
            };
        }
        for (auto* slider : { &upper, downRatioSliders[i].get() })
        {
            contentRoot.addAndMakeVisible (*slider);
            registerKeyboardListener (*slider);
        }
        for (auto* label : { &lowerBoundaryNames[i], &upperBoundaryNames[i], &upRatioNames[i], &downRatioNames[i] })
        {
            configureLabel (*label, {});
            label->setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
            contentRoot.addAndMakeVisible (*label);
        }
        for (const auto isUpper : { false, true })
        {
            auto& value = isUpper ? upperBoundaryValues[i] : lowerBoundaryValues[i];
            configureLabel (value, {});
            value.setFont (juce::Font (juce::FontOptions (10.5f)));
            value.setEditable (false, true, false);
            value.setTooltip ("Double-click to enter dB. Equal boundaries disable dynamic gain. Dragging one boundary into the other pushes both.");
            contentRoot.addAndMakeVisible (value);
            registerKeyboardListener (value);
            value.onTextChange = [this, i, isUpper]
            {
                auto& slider = isUpper ? *upperBoundarySliders[i] : *lowerBoundaryControls()[i];
                auto& readout = isUpper ? upperBoundaryValues[i] : lowerBoundaryValues[i];
                beginBoundaryGesture (static_cast<int> (i), isUpper);
                auto* parameter = processor.getAPVTS().getParameter (slider.getProperties()["qqscParameterID"].toString());
                if (parameter != nullptr) parameter->beginChangeGesture();
                slider.setValue (slider.getValueFromText (readout.getText()), juce::sendNotificationSync);
                handleBoundaryChange (static_cast<int> (i), isUpper);
                if (parameter != nullptr) parameter->endChangeGesture();
                endLinkedGesture();
                refreshBoundaryReadouts();
            };
        }
        lower[i]->onGestureStart = [this, i] { beginBoundaryGesture (static_cast<int> (i), false); };
        upper.onGestureStart = [this, i] { beginBoundaryGesture (static_cast<int> (i), true); };
        lower[i]->onGestureEnd = upper.onGestureEnd = [this] { endLinkedGesture(); };
        lower[i]->onValueChange = [this, i] { handleBoundaryChange (static_cast<int> (i), false); };
        upper.onValueChange = [this, i] { handleBoundaryChange (static_cast<int> (i), true); };
        if (i == 0)
        {
            downRatioSliders[i]->onGestureStart = [this] { beginUndoTransaction ("Down Ratio ST"); };
            mainRatios[i]->valueFromTextFunction = ratioFromText;
        }
        else
        {
            const auto targetIndex = i % 2 == 1 ? i + 1 : i - 1;
            const auto pair = i < 3 ? LinkedPair::downRatioLR : LinkedPair::downRatioMS;
            downRatioSliders[i]->onGestureStart = [this, i, targetIndex, pair]
            { beginLinkedGesture (pair, *downRatioSliders[i], *downRatioSliders[targetIndex], "Down Ratio Link"); };
            downRatioSliders[i]->onGestureEnd = [this] { endLinkedGesture(); };
            downRatioSliders[i]->onValueChange = [this, i, targetIndex, pair]
            { handleLinkedValueChange (pair, *downRatioSliders[i], *downRatioSliders[targetIndex]); };
            downRatioSliders[i]->valueFromTextFunction = [this, i, targetIndex, pair] (const juce::String& text)
            { return handleLinkedTextEntry (pair, *downRatioSliders[i], *downRatioSliders[targetIndex], text, "Down Ratio Link"); };
        }
        // Boundary readouts start the same gesture snapshot as their faders, so
        // numeric entry, dragging, fine dragging and Alt reset use one law.
        for (auto* slider : { lower[i], &upper })
            slider->valueFromTextFunction = [] (const juce::String& text)
            { return text.containsIgnoreCase ("off") || text.containsIgnoreCase ("-inf") ? -120.0 : text.getDoubleValue(); };
    }
    for (size_t i = 0; i < mainRatios.size(); ++i)
    {
        const auto singleStart = mainRatios[i]->onGestureStart;
        const auto singleChange = mainRatios[i]->onValueChange;
        mainRatios[i]->onGestureStart = [this, i, singleStart]
        {
            if (attachedCompressionMode == 1) beginDualRatioGesture (static_cast<int> (i), true);
            else if (singleStart) singleStart();
        };
        mainRatios[i]->onValueChange = [this, i, singleChange]
        {
            if (attachedCompressionMode == 1) handleDualRatioChange (static_cast<int> (i), true);
            else if (singleChange) singleChange();
        };
        mainRatios[i]->onGestureEnd = downRatioSliders[i]->onGestureEnd = [this] { endLinkedGesture(); };
        downRatioSliders[i]->onGestureStart = [this, i] { beginDualRatioGesture (static_cast<int> (i), false); };
        downRatioSliders[i]->onValueChange = [this, i] { handleDualRatioChange (static_cast<int> (i), false); };
    }
    updateCompressionUi();
}

void QQSuperCompressionAudioProcessorEditor::finishCompressionControlGestures()
{
    // End old attachment gestures before any mode-driven detach, including
    // hidden domains. Cancellation also ignores remaining drag events until
    // physical mouseUp, preventing an old drag anchor from changing the new bank.
    for (auto* slider : lowerBoundaryControls()) slider->cancelNativeDragForParameterRebind();
    for (auto* slider : mainRatioControls()) slider->cancelNativeDragForParameterRebind();
    for (auto& slider : upperBoundarySliders)
        if (slider != nullptr) slider->cancelNativeDragForParameterRebind();
    for (auto& slider : downRatioSliders)
        if (slider != nullptr) slider->cancelNativeDragForParameterRebind();
    endLinkedGesture();
}

void QQSuperCompressionAudioProcessorEditor::reattachCompressionControls (bool dual)
{
    finishCompressionControlGestures();
    auto& state = processor.getAPVTS();
    const std::array<const char*, 5> singleLower { "thresholdDb", "thresholdLDb", "thresholdRDb", "thresholdMDb", "thresholdSDb" };
    const std::array<const char*, 5> singleUpper { "rangeDb", "rangeLDb", "rangeRDb", "rangeMDb", "rangeSDb" };
    const std::array<const char*, 5> dualLower { "upThresholdDb", "upThresholdLDb", "upThresholdRDb", "upThresholdMDb", "upThresholdSDb" };
    const std::array<const char*, 5> dualUpper { "downThresholdDb", "downThresholdLDb", "downThresholdRDb", "downThresholdMDb", "downThresholdSDb" };
    const std::array<const char*, 5> singleRatio { "ratio", "ratioL", "ratioR", "ratioM", "ratioS" };
    const std::array<const char*, 5> upRatio { "upRatio", "upRatioL", "upRatioR", "upRatioM", "upRatioS" };
    const std::array<const char*, 5> downRatio { "downRatio", "downRatioL", "downRatioR", "downRatioM", "downRatioS" };
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    const std::array<std::unique_ptr<Attachment>*, 5> lowerAttachments { &thresholdAttachment, &thresholdLAttachment, &thresholdRAttachment, &thresholdMAttachment, &thresholdSAttachment };
    const std::array<std::unique_ptr<Attachment>*, 5> ratioAttachments { &ratioAttachment, &ratioLAttachment, &ratioRAttachment, &ratioMAttachment, &ratioSAttachment };
    const auto lower = lowerBoundaryControls();
    const auto ratios = mainRatioControls();
    const juce::ScopedValueSetter<bool> guard (boundaryValueUpdateInProgress, true);
    for (size_t i = 0; i < lower.size(); ++i)
    {
        lowerAttachments[i]->reset();
        ratioAttachments[i]->reset();
        upperBoundaryAttachments[i].reset();
        downRatioAttachments[i].reset();
        const auto lowerID = dual ? dualLower[i] : singleLower[i];
        const auto upperID = dual ? dualUpper[i] : singleUpper[i];
        lower[i]->getProperties().set ("qqscParameterID", lowerID);
        upperBoundarySliders[i]->getProperties().set ("qqscParameterID", upperID);
        ratios[i]->getProperties().set ("qqscParameterID", dual ? upRatio[i] : singleRatio[i]);
        downRatioSliders[i]->getProperties().set ("qqscParameterID", downRatio[i]);
        ratios[i]->getProperties().set ("qqscLightOrigin", dual ? 1.0 : 0.5);
        downRatioSliders[i]->getProperties().set ("qqscLightOrigin", 0.0);
        *lowerAttachments[i] = std::make_unique<Attachment> (state, lowerID, *lower[i]);
        upperBoundaryAttachments[i] = std::make_unique<Attachment> (state, upperID, *upperBoundarySliders[i]);
        *ratioAttachments[i] = std::make_unique<Attachment> (state, dual ? upRatio[i] : singleRatio[i], *ratios[i]);
        downRatioAttachments[i] = std::make_unique<Attachment> (state, downRatio[i], *downRatioSliders[i]);
        // Attachments install parameter text converters. Restore the editor's
        // reciprocal syntax and relative-LINK commit handlers after each bank
        // switch, including host automation, Undo and A/B recalls.
        ratios[i]->textFromValueFunction = ratioText;
        downRatioSliders[i]->textFromValueFunction = ratioText;
        lower[i]->valueFromTextFunction = [] (const juce::String& text)
            { return text.containsIgnoreCase ("off") || text.containsIgnoreCase ("-inf") ? -120.0 : text.getDoubleValue(); };
        upperBoundarySliders[i]->valueFromTextFunction = [dual] (const juce::String& text)
        {
            return ! dual ? static_cast<double> (qqsc::params::rangeFromText (text))
                : (text.containsIgnoreCase ("off") || text.containsIgnoreCase ("-inf") ? -120.0 : text.getDoubleValue());
        };
        if (i == 0)
        {
            ratios[i]->valueFromTextFunction = ratioFromText;
            downRatioSliders[i]->valueFromTextFunction = ratioFromText;
        }
        else
        {
            const auto targetIndex = i % 2 == 1 ? i + 1 : i - 1;
            const auto upPair = i < 3 ? LinkedPair::ratioLR : LinkedPair::ratioMS;
            const auto downPair = i < 3 ? LinkedPair::downRatioLR : LinkedPair::downRatioMS;
            ratios[i]->valueFromTextFunction = [this, i, targetIndex, upPair] (const juce::String& text)
            {
                const auto controls = mainRatioControls();
                return handleLinkedTextEntry (upPair, *controls[i], *controls[targetIndex], text, "Ratio Link");
            };
            downRatioSliders[i]->valueFromTextFunction = [this, i, targetIndex, downPair] (const juce::String& text)
            { return handleLinkedTextEntry (downPair, *downRatioSliders[i], *downRatioSliders[targetIndex], text, "Down Ratio Link"); };
        }
        if (dual)
        {
            ratios[i]->valueFromTextFunction = [this, i] (const juce::String& text)
            { return dualRatioFromText (static_cast<int> (i), true, text); };
            downRatioSliders[i]->valueFromTextFunction = [this, i] (const juce::String& text)
            { return dualRatioFromText (static_cast<int> (i), false, text); };
        }
        ratios[i]->updateText();
        downRatioSliders[i]->updateText();
        lower[i]->setResetValue (-120.0);
        upperBoundarySliders[i]->setResetValue (dual ? 0.0 : qqsc::params::rangeOffDb);
        ratios[i]->setResetValue (1.0);
        downRatioSliders[i]->setResetValue (1.0);
        ratios[i]->setTooltip (dual ? "Upward Ratio: 1:32 to 1:1" : "Ratio: 1:32 to 32:1. Below 1:1 boosts; above 1:1 reduces.");
        downRatioSliders[i]->setTooltip ("Downward Ratio: 1:1 to 32:1");
        lower[i]->setTooltip (dual ? "UP gate: lift only above UP and below DOWN. At equality all dynamic gain stops." : "Threshold; lower boundary. Colliding with Range pushes it upward.");
        upperBoundarySliders[i]->setTooltip (dual ? "Downward Threshold; upper boundary. At equality all dynamic gain stops." : "Range: finite values set an upper cutoff. The extra OFF endpoint removes the upper limit; finite 0 dB remains a cutoff.");
    }
}

void QQSuperCompressionAudioProcessorEditor::beginBoundaryGesture (int domain, bool upper)
{
    endLinkedGesture();
    boundaryGestureActive = true;
    const auto controls = lowerBoundaryControls();
    auto& source = upper ? *upperBoundarySliders[static_cast<size_t> (domain)] : *controls[static_cast<size_t> (domain)];
    const auto targetIndex = domain == 0 ? 0 : (domain % 2 == 1 ? domain + 1 : domain - 1);
    if (domain == 0)
        beginUndoTransaction (upper ? "Upper Boundary" : "Lower Boundary");
    else
    {
        auto& target = upper ? *upperBoundarySliders[static_cast<size_t> (targetIndex)] : *controls[static_cast<size_t> (targetIndex)];
        const auto pair = upper ? (domain < 3 ? LinkedPair::upperBoundaryLR : LinkedPair::upperBoundaryMS)
                                : (domain < 3 ? LinkedPair::thresholdLR : LinkedPair::thresholdMS);
        beginLinkedGesture (pair, source, target, upper ? "Upper Boundary Link" : "Lower Boundary Link");
    }
    // A pushed boundary is a real parameter edit and must be part of the same
    // host automation gesture and undo transaction as the boundary being held.
    for (const auto index : { domain, targetIndex })
    {
        if (index != domain && ! linkButton.getToggleState()) continue;
        for (auto* companion : { controls[static_cast<size_t> (index)], upperBoundarySliders[static_cast<size_t> (index)].get() })
        {
            if (companion == &source) continue;
            auto* parameter = processor.getAPVTS().getParameter (companion->getProperties()["qqscParameterID"].toString());
            if (parameter != nullptr && std::find (companionGestureParameters.begin(), companionGestureParameters.end(), parameter) == companionGestureParameters.end())
            {
                parameter->beginChangeGesture();
                companionGestureParameters.push_back (parameter);
            }
        }
    }
}

void QQSuperCompressionAudioProcessorEditor::handleBoundaryChange (int domain, bool upper)
{
    if (boundaryValueUpdateInProgress) return;
    const auto lower = lowerBoundaryControls();
    const auto index = static_cast<size_t> (domain);
    if (domain != 0)
    {
        const auto targetIndex = static_cast<size_t> (domain % 2 == 1 ? domain + 1 : domain - 1);
        const auto pair = upper ? (domain < 3 ? LinkedPair::upperBoundaryLR : LinkedPair::upperBoundaryMS)
                                : (domain < 3 ? LinkedPair::thresholdLR : LinkedPair::thresholdMS);
        handleLinkedValueChange (pair, upper ? *upperBoundarySliders[index] : *lower[index],
                                 upper ? *upperBoundarySliders[targetIndex] : *lower[targetIndex]);
    }
    const juce::ScopedValueSetter<bool> guard (boundaryValueUpdateInProgress, true);
    if (boundaryGestureActive)
    {
        auto lowerValue = static_cast<float> (lower[index]->getValue());
        auto upperValue = static_cast<float> (upperBoundarySliders[index]->getValue());
        if (lowerValue > upperValue)
        {
            if (upper) lowerValue = upperValue;
            else upperValue = lowerValue;
        }
        // Publish the whole intended UI pair after LINK has clamped its shared
        // delta. This also corrects a temporary raw-parameter crossing before
        // the attachment delivered this callback, and handles same-value edits.
        processor.setBoundaryForDomainDb (attachedCompressionMode == 1, upper, domain, upper ? upperValue : lowerValue);
        processor.setBoundaryForDomainDb (attachedCompressionMode == 1, ! upper, domain, upper ? lowerValue : upperValue);
    }
    lower[index]->setValue (processor.getBoundaryForDomainDb (attachedCompressionMode == 1, false, domain), juce::dontSendNotification);
    upperBoundarySliders[index]->setValue (processor.getBoundaryForDomainDb (attachedCompressionMode == 1, true, domain), juce::dontSendNotification);
    refreshBoundaryReadouts();
}

void QQSuperCompressionAudioProcessorEditor::refreshBoundaryReadouts()
{
    if (upperBoundarySliders[0] == nullptr) return;
    const auto lower = lowerBoundaryControls();
    for (size_t i = 0; i < lower.size(); ++i)
    {
        const auto formatBoundary = [this, i] (double db, bool upper)
        {
            if (upper && attachedCompressionMode == 0 && ! qqsc::params::isRangeEnabled (static_cast<float> (db)))
                return juce::String ("OFF");
            if (! qqsc::params::isThresholdEnabled (static_cast<float> (db)))
                return juce::String (attachedCompressionMode == 0 && ! upper ? "OFF" : "-inf");
            return juce::String (db, i == 0 ? 2 : 1) + (i == 0 ? " dB" : "");
        };
        if (! lowerBoundaryValues[i].isBeingEdited())
            lowerBoundaryValues[i].setText (formatBoundary (lower[i]->getValue(), false), juce::dontSendNotification);
        if (! upperBoundaryValues[i].isBeingEdited())
            upperBoundaryValues[i].setText (formatBoundary (upperBoundarySliders[i]->getValue(), true), juce::dontSendNotification);
    }
}

void QQSuperCompressionAudioProcessorEditor::updateCompressionUi()
{
    if (upperBoundarySliders[0] == nullptr) return;
    const bool dual = processor.getAPVTS().getRawParameterValue ("compressionMode")->load() >= 0.5f;
    const int channelMode = juce::roundToInt (processor.getAPVTS().getRawParameterValue (qqsc::params::processingMode)->load());
    const bool changed = attachedCompressionMode != static_cast<int> (dual) || channelMode != laidOutProcessingMode;
    const auto referenceGain = processor.getAPVTS().getRawParameterValue (qqsc::params::inputGainDb)->load();
    const int referenceKeySource = juce::roundToInt (processor.getAPVTS().getRawParameterValue (qqsc::params::keySource)->load());
    const bool referenceChanged = referenceGain != boundaryReferenceInputDb || referenceKeySource != boundaryReferenceKeySource;
    boundaryReferenceInputDb = referenceGain;
    boundaryReferenceKeySource = referenceKeySource;
    if (attachedCompressionMode != static_cast<int> (dual))
    {
        attachedCompressionMode = static_cast<int> (dual);
        reattachCompressionControls (dual);
    }
    laidOutProcessingMode = channelMode;
    compressionModeButton.setButtonText (dual ? "DUAL" : "SINGLE");
    compressionModeButton.setToggleState (dual, juce::dontSendNotification);
    dualRatioLinkButton.setVisible (dual);
    dualRatioLinkButton.setToggleState (processor.getAPVTS().getRawParameterValue (qqsc::params::dualRatioLink)->load() >= 0.5f,
                                       juce::dontSendNotification);
    const auto lower = lowerBoundaryControls();
    const juce::ScopedValueSetter<bool> guard (boundaryValueUpdateInProgress, true);
    for (size_t i = 0; i < lower.size(); ++i)
    {
        const bool visible = channelMode == qqsc::params::stereoLinked ? i == 0
            : (channelMode == qqsc::params::leftRight ? i == 1 || i == 2 : i == 3 || i == 4);
        const juce::String prefix = i == 0 ? "" : juce::String (i == 1 ? "L " : i == 2 ? "R " : i == 3 ? "M " : "S ");
        lower[i]->getProperties().set ("qqscDualBoundary", dual);
        upperBoundarySliders[i]->getProperties().set ("qqscDualBoundary", dual);
        lowerBoundaryNames[i].setColour (juce::Label::textColourId, boundaryColour (dual, false));
        upperBoundaryNames[i].setColour (juce::Label::textColourId, boundaryColour (dual, true));
        lowerBoundaryValues[i].setColour (juce::Label::textColourId, dual ? boundaryColour (dual, false) : qqsc::ui::text());
        upperBoundaryValues[i].setColour (juce::Label::textColourId, boundaryColour (dual, true));
        if (changed || referenceChanged) { lower[i]->repaint(); upperBoundarySliders[i]->repaint(); }
        lowerBoundaryNames[i].setText (prefix + (i == 0 ? (dual ? "UP THR" : "THRESHOLD") : (dual ? "UP" : "THR")), juce::dontSendNotification);
        upperBoundaryNames[i].setText (prefix + (i == 0 ? (dual ? "DOWN THR" : "RANGE") : (dual ? "DOWN" : "RNG")), juce::dontSendNotification);
        upRatioNames[i].setText (prefix + "UP RATIO", juce::dontSendNotification);
        downRatioNames[i].setText (prefix + "DOWN RATIO", juce::dontSendNotification);
        upperBoundarySliders[i]->setVisible (visible);
        downRatioSliders[i]->setVisible (visible && dual);
        for (auto* label : { &lowerBoundaryNames[i], &upperBoundaryNames[i], &lowerBoundaryValues[i], &upperBoundaryValues[i] })
            label->setVisible (visible);
        upRatioNames[i].setVisible (visible && dual);
        downRatioNames[i].setVisible (visible && dual);
        for (bool upward : { true, false })
        {
            auto& button = upward ? upEnabledButtons[i] : downEnabledButtons[i];
            const auto* id = (upward ? qqsc::params::upEnabledIds : qqsc::params::downEnabledIds)[i];
            const bool enabled = processor.getAPVTS().getRawParameterValue (id)->load() >= 0.5f;
            button.setVisible (visible && dual);
            button.setToggleState (enabled, juce::dontSendNotification);
            button.setButtonText (enabled ? "ON" : "OFF");
            auto* knob = upward ? mainRatioControls()[i] : downRatioSliders[i].get();
            knob->setAlpha (! dual || enabled ? 1.0f : 0.45f);
        }
        if (activeLinkSource != lower[i] && activeLinkSource != upperBoundarySliders[i].get())
        {
            lower[i]->setValue (processor.getBoundaryForDomainDb (dual, false, static_cast<int> (i)), juce::dontSendNotification);
            upperBoundarySliders[i]->setValue (processor.getBoundaryForDomainDb (dual, true, static_cast<int> (i)), juce::dontSendNotification);
        }
    }
    refreshBoundaryReadouts();
    if (changed) resized();
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

    if (code == juce::KeyPress::escapeKey && sidechainPanelOpen)
    {
        sidechainPanelOpen = false;
        processor.setSidechainListenEnabled (false);
        updateSidechainUi();
        return true;
    }

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
    updateSidechainUi();

    // Direct UI control changes notify immediately. This additional path covers
    // Undo/Redo, A/B recall and host-side parameter changes while the editor is
    // open, including when transport is stopped and no audio callback is running.
    if (latencyChoiceChangedOutsideUiGesture)
        processor.notifyHostProcessingLatency();
}

void QQSuperCompressionAudioProcessorEditor::updateModeUi()
{
    updateCompressionUi();
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
    ratioChannel0Label.setVisible (! st && attachedCompressionMode != 1);
    ratioChannel1Label.setVisible (! st && attachedCompressionMode != 1);
    ratioLabel.setVisible (attachedCompressionMode != 1);

    thresholdSlider.setVisible (st);
    thresholdLSlider.setVisible (lr);
    thresholdRSlider.setVisible (lr);
    thresholdMSlider.setVisible (ms);
    thresholdSSlider.setVisible (ms);
    thresholdLabel.setVisible (false);
    thresholdChannel0Label.setVisible (false);
    thresholdChannel1Label.setVisible (false);

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
    if (qqsc::ui::isDarkTheme())
    {
        g.fillAll (qqsc::ui::canvas());
        const auto scale = static_cast<float> (getWidth()) / defaultEditorWidth;
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (scale));
        g.setColour (juce::Colour (0xff414345));
        g.drawRoundedRectangle ({ 1.0f, 1.0f, 1198.0f, 798.0f }, 17.0f, 0.8f);
        g.setColour (juce::Colour (0xff121416));
        g.drawRoundedRectangle ({ 2.5f, 2.5f, 1195.0f, 795.0f }, 16.0f, 0.8f);
        qqsc::dark::panel (g, { 16.0f, 74.0f, 1168.0f, 542.0f }, 15.0f);
        darkBottomPanel.draw (g, { 16.0f, 620.0f, 1168.0f, 162.0f }, 15.0f);
        return;
    }
    if (! qqsc::ui::isClassicTheme())
    {
        // Landscape chassis shares the same design space as the widgets.
        // Display remains opaque and owns its 60 Hz paints independently.
        g.fillAll (qqsc::ui::canvas());
        const float scale = static_cast<float> (getWidth()) / defaultEditorWidth;
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (scale));
        juce::ColourGradient chassis (juce::Colour (0xfff4f1ed), 0.0f, 0.0f,
                                     juce::Colour (0xffe2dcd4), 1200.0f, 800.0f, false);
        chassis.addColour (0.45, qqsc::ui::canvas());
        g.setGradientFill (chassis);
        g.fillRect (0.0f, 0.0f, 1200.0f, 800.0f);
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.drawRoundedRectangle ({ 3.0f, 3.0f, 1194.0f, 794.0f }, 15.0f, 1.0f);
        g.setColour (qqsc::ui::border().withAlpha (0.55f));
        g.drawRoundedRectangle ({ 1.0f, 1.0f, 1198.0f, 798.0f }, 17.0f, 0.8f);
        qqsc::warm::backplate (g, { 16.0f, 74.0f, 1168.0f, 542.0f }, 15.0f);
        qqsc::warm::backplate (g, { 16.0f, 620.0f, 1168.0f, 162.0f }, 15.0f);
        return;
    }

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
    const auto visualPanel = scaled ({ 16.0f, 74.0f, 1168.0f, 542.0f });
    const auto controlPanel = scaled ({ 16.0f, 620.0f, 1168.0f, 162.0f });

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
    // Layout is calculated in the 1200x800 landscape design space. The
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
    themeButton.setBounds (right - 76, headerButtonY, 76, headerButtonH);
    right -= 76 + smallGap;
    sidechainButton.setBounds (right - 86, headerButtonY, 86, headerButtonH);
    right -= 86 + 12;

    // The existing sidechain popup grows
    // leftward from the Side Chain button and adds HPF beside Key Gain.
    sidechainPanelBounds = { sidechainButton.getRight() - 330, 58, 330, 146 };
    sidechainPanelBackground.setBounds (sidechainPanelBounds);
    const int panelX = sidechainPanelBounds.getX();
    const int panelY = sidechainPanelBounds.getY();
    keySourceLabel.setBounds (panelX + 12, panelY + 10, 94, 14);
    keyInternalButton.setBounds (panelX + 12, panelY + 27, 45, 23);
    keyExternalButton.setBounds (panelX + 61, panelY + 27, 45, 23);
    keyMeterLabel.setBounds (panelX + 12, panelY + 57, 94, 14);
    keyMeter.setBounds (panelX + 12, panelY + 74, 94, 20);
    sidechainListenButton.setBounds (panelX + 12, panelY + 106, 94, 25);
    keyGainLabel.setBounds (panelX + 119, panelY + 10, 96, 14);
    keyGainSlider.setBounds (panelX + 119, panelY + 26, 96, 108);
    keyHpfLabel.setBounds (panelX + 222, panelY + 10, 96, 14);
    keyHpfSlider.setBounds (panelX + 222, panelY + 26, 96, 108);


    title.setBounds (margin, 16, juce::jmax (260, right - margin), 34);
    versionLabel.setBounds (margin + 2, 50, 72, 14);

    auto area = juce::Rectangle<int> (0, 0, defaultEditorWidth, defaultEditorHeight).reduced (margin);
    area.removeFromTop (58);

    // Wider history with only 20 design pixels less height, retaining readable
    // stacked LR/MS domains and the established controls' physical proportions.
    auto visualRow = area.removeFromTop (530);

    const int meterWidth = juce::jlimit (180, 200, visualRow.getWidth() / 5);
    meters.setBounds (visualRow.removeFromRight (meterWidth));
    visualRow.removeFromRight (8);

    // The original 76 px strip follows the Display's exact plot geometry.
    // Slider/APVTS normalization never determines a thumb's visual ordinate.
    auto thresholdArea = visualRow.removeFromRight (76);
    visualRow.removeFromRight (6);
    display.setBounds (visualRow);
    if (upperBoundarySliders[0] != nullptr)
    {
        const auto lower = lowerBoundaryControls();
        const auto lane = thresholdArea.reduced (3, 0);
        for (size_t index = 0; index < lower.size(); ++index)
        {
            const auto plot = display.getBoundaryPlotForDomain (static_cast<int> (index)).translated (
                static_cast<float> (display.getX()), static_cast<float> (display.getY()));
            const auto railTop = static_cast<int> (std::floor (plot.getY())) - 8;
            const auto railBottom = static_cast<int> (std::ceil (plot.getBottom())) + 8;
            const auto halfWidth = lane.getWidth() / 2;
            lower[index]->setBounds (lane.getX(), railTop, halfWidth, railBottom - railTop);
            upperBoundarySliders[index]->setBounds (lane.getX() + halfWidth, railTop, lane.getWidth() - halfWidth, railBottom - railTop);
            if (index == 0)
            {
                upperBoundaryNames[index].setBounds (lane.getX(), juce::roundToInt (plot.getY()) - 45, lane.getWidth(), 14);
                upperBoundaryValues[index].setBounds (lane.getX(), juce::roundToInt (plot.getY()) - 31, lane.getWidth(), 21);
                lowerBoundaryValues[index].setBounds (lane.getX(), juce::roundToInt (plot.getBottom()) + 8, lane.getWidth(), 20);
                lowerBoundaryNames[index].setBounds (lane.getX(), juce::roundToInt (plot.getBottom()) + 28, lane.getWidth(), 12);
            }
            else
            {
                const int headerY = juce::roundToInt (plot.getY()) - 19;
                const int valueY = juce::roundToInt (plot.getBottom()) + 8;
                lowerBoundaryNames[index].setBounds (lane.getX(), headerY, halfWidth, 13);
                upperBoundaryNames[index].setBounds (lane.getX() + halfWidth, headerY, lane.getWidth() - halfWidth, 13);
                lowerBoundaryValues[index].setBounds (lane.getX(), valueY, halfWidth - 1, 20);
                upperBoundaryValues[index].setBounds (lane.getX() + halfWidth + 1, valueY, lane.getWidth() - halfWidth - 1, 20);
            }
        }
    }

    area.removeFromTop (10);

    // Controls remain compact and functionally unchanged; the extra editor height
    // is intentionally spent on Display/Meters rather than larger knobs.
    auto controls = area.removeFromTop (158);
    const auto controlsY = controls.getY();
    constexpr int controlGap = 6;
    constexpr int smallTrimW = 116;
    constexpr int modeW = 170;
    const int mainW = (controls.getWidth() - smallTrimW * 2 - modeW - controlGap * 5) / 3;

    auto inputArea = controls.removeFromLeft (smallTrimW);
    inputGainLabel.setBounds (inputArea.removeFromTop (18));
    inputGainSlider.setBounds (inputArea.withSizeKeepingCentre (100, 116));
    controls.removeFromLeft (controlGap);

    auto ratioArea = controls.removeFromLeft (mainW).withWidth (216);
    ratioArea.setCentre (296, ratioArea.getCentreY());
    const auto fullRatioArea = ratioArea;
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
    if (downRatioSliders[0] != nullptr)
    {
        const auto ratios = mainRatioControls();
        for (auto* ratio : ratios)
            ratio->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 24);
        if (attachedCompressionMode == 1)
        {
            auto upColumn = fullRatioArea.withWidth (fullRatioArea.getWidth() / 2);
            auto downColumn = fullRatioArea.withTrimmedLeft (upColumn.getWidth());
            const auto placeRatio = [this, &ratios] (size_t index, juce::Rectangle<int> up, juce::Rectangle<int> down, bool compact)
            {
                upRatioNames[index].setBounds (up.removeFromTop (14));
                downRatioNames[index].setBounds (down.removeFromTop (14));
                const int knobWidth = compact ? 70 : 68;
                const int knobHeight = compact ? 62 : 104;
                ratios[index]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, compact ? 18 : 23);
                downRatioSliders[index]->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, compact ? 18 : 23);
                ratios[index]->setBounds (up.withSizeKeepingCentre (knobWidth, knobHeight));
                downRatioSliders[index]->setBounds (down.withSizeKeepingCentre (knobWidth, knobHeight));
                const int width = compact ? 22 : 25, height = compact ? 12 : 15;
                for (bool upward : { true, false })
                {
                    auto& button = upward ? upEnabledButtons[index] : downEnabledButtons[index];
                    const auto bounds = upward ? ratios[index]->getBounds() : downRatioSliders[index]->getBounds();
                    button.setBounds (bounds.getRight() + 1, bounds.getY() + (compact ? 23 : 29), width, height);
                }
            };
            placeRatio (0, upColumn, downColumn, false);
            auto upFirst = upColumn.removeFromTop (upColumn.getHeight() / 2);
            auto downFirst = downColumn.removeFromTop (downColumn.getHeight() / 2);
            placeRatio (1, upFirst, downFirst, true);
            placeRatio (3, upFirst, downFirst, true);
            placeRatio (2, upColumn, downColumn, true);
            placeRatio (4, upColumn, downColumn, true);
        }
    }
    controls.removeFromLeft (controlGap);

    auto makeupArea = controls.removeFromLeft (mainW).withWidth (216);
    makeupArea.setCentre (512, makeupArea.getCentreY());
    auto makeupHeader = makeupArea.removeFromTop (20);
    compressionModeButton.setBounds (380, controlsY, 60, 21);
    const bool compactRatioLink = laidOutProcessingMode != qqsc::params::stereoLinked;
    dualRatioLinkButton.setBounds (compactRatioLink ? 281 : 278, controlsY + (compactRatioLink ? 16 : 24),
                                  compactRatioLink ? 30 : 36, compactRatioLink ? 14 : 17);
    auto matchArea = makeupHeader.removeFromRight (58).reduced (2, 0);
    makeupLabel.setBounds (458, controlsY, 108, 20);
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

    auto mixArea = controls.removeFromLeft (mainW).withWidth (216);
    mixArea.setCentre (728, mixArea.getCentreY());
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

    // Five equal visual dials, positioned by their centres rather than by
    // differently sized label/text-box containers. The same geometry feeds all
    // themes; paired LR/MS and compact Dual ratios retain their smaller dials.
    const std::array<FineKnob*, 5> primaryDials {
        &inputGainSlider, &ratioSlider, &makeupSTSlider, &mixSlider, &outputGainSlider
    };
    for (size_t i = 0; i < primaryDials.size(); ++i)
    {
        auto& dial = *primaryDials[i];
        if (i != 1 || attachedCompressionMode == 0)
        {
            dial.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 86, 24);
            dial.setBounds (30 + static_cast<int> (i) * 216, controlsY + 28, 100, 118);
        }
    }
    outputGainLabel.setCentrePosition (944, outputGainLabel.getBounds().getCentreY());

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
