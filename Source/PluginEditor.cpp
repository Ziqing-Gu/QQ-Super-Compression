#include "PluginEditor.h"
#include "Parameters.h"
#include <cmath>

namespace
{
constexpr int defaultEditorWidth = 1020;
constexpr int defaultEditorHeight = 670;
constexpr double editorAspectRatio = static_cast<double> (defaultEditorWidth) / defaultEditorHeight;
constexpr int minEditorHeight = 610;
constexpr int minEditorWidth = static_cast<int> (minEditorHeight * editorAspectRatio + 0.5);
constexpr int maxEditorHeight = 950;
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
    int savedHeight = defaultEditorHeight;
    if (uiProperties != nullptr)
    {
        savedWidth = uiProperties->getIntValue ("editorWidth", defaultEditorWidth);
        savedHeight = uiProperties->getIntValue ("editorHeight", defaultEditorHeight);
    }

    // Older builds allowed width and height to drift independently. Migrate
    // any saved non-proportional size to the largest uniform scale that fits
    // inside the old saved rectangle, then keep the fixed design aspect ratio.
    const auto minScale = static_cast<double> (minEditorHeight) / defaultEditorHeight;
    const auto maxScale = static_cast<double> (maxEditorHeight) / defaultEditorHeight;
    const auto savedScale = juce::jlimit (minScale, maxScale,
                                         juce::jmin (static_cast<double> (savedWidth) / defaultEditorWidth,
                                                     static_cast<double> (savedHeight) / defaultEditorHeight));
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
    configureLabel (makeupLabel, "MAKEUP");
    configureLabel (makeupChannel0Label, "L");
    configureLabel (makeupChannel1Label, "R");
    configureLabel (mixLabel, "MIX");
    configureLabel (outputGainLabel, "OUTPUT GAIN");
    configureLabel (modeLabel, "MODE");
    configureLabel (lookaheadLabel, "LOOKAHEAD (ms)");
    configureLabel (oversamplingLabel, "OVERSAMPLING");

    inputGainLabel.setColour (juce::Label::textColourId, qqsc::ui::textMuted());
    ratioLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.36f));
    makeupLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.36f));
    mixLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.36f));
    outputGainLabel.setColour (juce::Label::textColourId, qqsc::ui::textMuted());
    modeLabel.setColour (juce::Label::textColourId, qqsc::ui::warmAccent().darker (0.42f));
    lookaheadLabel.setColour (juce::Label::textColourId, qqsc::ui::cyanAccent().darker (0.42f));
    oversamplingLabel.setColour (juce::Label::textColourId, qqsc::ui::cyanAccent().darker (0.42f));

    for (auto* label : { &inputGainLabel, &ratioLabel, &makeupLabel, &makeupChannel0Label, &makeupChannel1Label,
                          &mixLabel, &outputGainLabel, &modeLabel, &lookaheadLabel, &oversamplingLabel })
        contentRoot.addAndMakeVisible (*label);

    configureKnob (inputGainSlider, " dB");
    configureKnob (ratioSlider);
    ratioSlider.textFromValueFunction = [] (double v)
    {
        return juce::String (v, v < 10.0 ? 2 : 1) + ":1";
    };

    for (auto* makeup : { &makeupSTSlider, &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider })
        configureKnob (*makeup, " dB");
    configureKnob (mixSlider, " %");
    configureKnob (outputGainSlider, " dB");

    // Input/Output are secondary trims: smaller controls so Ratio / Makeup / Mix
    // remain the visual focus. Their text boxes stay directly editable.
    inputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 22);
    outputGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 22);

    // Warm/transparent UI: musical gain controls use the warm lamp colour.
    inputGainSlider.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
    ratioSlider.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
    mixSlider.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
    outputGainSlider.setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());
    for (auto* makeup : { &makeupSTSlider, &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider })
        makeup->setColour (juce::Slider::rotarySliderFillColourId, qqsc::ui::warmAccent());

    for (auto* slider : { &inputGainSlider, &ratioSlider, &makeupSTSlider, &makeupLSlider, &makeupRSlider,
                          &makeupMSlider, &makeupSSlider, &mixSlider, &outputGainSlider })
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
    aButton.setClickingTogglesState (false);
    bButton.setClickingTogglesState (false);

    for (auto* button : { &modeButton, &matchButton, &bypassButton, &aButton, &bButton, &aToBButton, &bToAButton, &oversamplingButton })
    {
        contentRoot.addAndMakeVisible (*button);
        registerKeyboardListener (*button);
    }

    auto& state = processor.getAPVTS();
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::inputGainDb, inputGainSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::ratio, ratioSlider);
    makeupSTAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainDb, makeupSTSlider);
    makeupLAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainLDb, makeupLSlider);
    makeupRAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainRDb, makeupRSlider);
    makeupMAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainMDb, makeupMSlider);
    makeupSAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainSDb, makeupSSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::mix, mixSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::outputGainDb, outputGainSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, qqsc::params::bypass, bypassButton);

    inputGainSlider.onGestureStart = [this] { beginUndoTransaction ("Input Gain"); };
    ratioSlider.onGestureStart = [this] { beginUndoTransaction ("Ratio"); };
    makeupSTSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup ST"); };
    makeupLSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup L"); };
    makeupRSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup R"); };
    makeupMSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup M"); };
    makeupSSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup S"); };
    mixSlider.onGestureStart = [this] { beginUndoTransaction ("Mix"); };
    outputGainSlider.onGestureStart = [this] { beginUndoTransaction ("Output Gain"); };

    modeButton.onClick = [this] { cycleMode(); };
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

    // Non-zero Lookahead uses 1x internally and does not expose a meaningless
    // Oversampling control. The stored 0 ms choice is preserved while hidden.
    oversamplingLabel.setVisible (zeroMs);
    oversamplingButton.setVisible (zeroMs);
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

    bool latencyChoiceChangedOutsideDirectUiGesture = false;

    const auto lookaheadIndex = qqsc::params::lookaheadChoiceIndexForMs (
        processor.getAPVTS().getRawParameterValue (qqsc::params::lookaheadMs)->load());
    if (lookaheadCombo.getSelectedItemIndex() != lookaheadIndex)
    {
        lookaheadCombo.setSelectedItemIndex (lookaheadIndex, juce::dontSendNotification);
        latencyChoiceChangedOutsideDirectUiGesture = true;
    }

    const auto oversamplingIndex = juce::jlimit (0, 2, juce::roundToInt (
        processor.getAPVTS().getRawParameterValue (qqsc::params::oversampling)->load()));
    const auto currentButtonText = qqsc::params::oversamplingNameForChoiceIndex (oversamplingIndex);
    if (oversamplingButton.getButtonText() != currentButtonText)
        latencyChoiceChangedOutsideDirectUiGesture = true;

    updateOversamplingUi();

    // Direct ComboBox changes notify immediately. This additional path covers
    // Undo/Redo, A/B recall and host-side parameter changes while the editor is
    // open, including when transport is stopped and no audio callback is running.
    if (latencyChoiceChangedOutsideDirectUiGesture)
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

    makeupSTSlider.setVisible (st);
    makeupLSlider.setVisible (lr);
    makeupRSlider.setVisible (lr);
    makeupMSlider.setVisible (ms);
    makeupSSlider.setVisible (ms);
    makeupChannel0Label.setVisible (! st);
    makeupChannel1Label.setVisible (! st);

    if (lr)
    {
        makeupChannel0Label.setText ("L", juce::dontSendNotification);
        makeupChannel1Label.setText ("R", juce::dontSendNotification);
    }
    else if (ms)
    {
        makeupChannel0Label.setText ("M", juce::dontSendNotification);
        makeupChannel1Label.setText ("S", juce::dontSendNotification);
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
    const auto visualPanel = scaled ({ 16.0f, 74.0f, 988.0f, 364.0f });
    const auto controlPanel = scaled ({ 16.0f, 452.0f, 988.0f, 190.0f });

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
    // Layout is always calculated in the original 1020x670 design space.
    // The single parent transform then scales every child, font, stroke and
    // meter together by the same X/Y factor.
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

    auto visualRow = area.removeFromTop (350);

    // 0.1.3 deliberately slims the meter panel while keeping its full height,
    // giving the Dynamic Display substantially more horizontal room.
    const int meterWidth = juce::jlimit (190, 220, visualRow.getWidth() / 4);
    meters.setBounds (visualRow.removeFromRight (meterWidth));
    visualRow.removeFromRight (10);
    display.setBounds (visualRow);

    area.removeFromTop (16);

    auto controls = area.removeFromTop (166);
    constexpr int controlGap = 6;
    constexpr int smallTrimW = 122;
    constexpr int modeW = 176;
    const int mainW = (controls.getWidth() - smallTrimW * 2 - modeW - controlGap * 5) / 3;

    auto inputArea = controls.removeFromLeft (smallTrimW);
    inputGainLabel.setBounds (inputArea.removeFromTop (22));
    inputGainSlider.setBounds (inputArea.withSizeKeepingCentre (112, 138));
    controls.removeFromLeft (controlGap);

    auto ratioArea = controls.removeFromLeft (mainW);
    ratioLabel.setBounds (ratioArea.removeFromTop (22));
    ratioSlider.setBounds (ratioArea.withSizeKeepingCentre (150, 140));
    controls.removeFromLeft (controlGap);

    auto makeupArea = controls.removeFromLeft (mainW);
    auto makeupHeader = makeupArea.removeFromTop (24);
    auto matchArea = makeupHeader.removeFromRight (62).reduced (2, 1);
    makeupLabel.setBounds (makeupHeader);
    matchButton.setBounds (matchArea);

    // ST shows one common Makeup knob. LR and MS retain the established two
    // independent knobs, using the restored vector-drawn 0.9.1 control style.
    makeupSTSlider.setBounds (makeupArea.withSizeKeepingCentre (150, 138));

    auto dualMakeup = makeupArea;
    const int dualW = dualMakeup.getWidth() / 2;
    auto first = dualMakeup.removeFromLeft (dualW);
    auto second = dualMakeup;

    makeupChannel0Label.setBounds (first.removeFromTop (16));
    makeupChannel1Label.setBounds (second.removeFromTop (16));

    const auto firstKnob = first.withSizeKeepingCentre (82, 118);
    const auto secondKnob = second.withSizeKeepingCentre (82, 118);
    makeupLSlider.setBounds (firstKnob);
    makeupMSlider.setBounds (firstKnob);
    makeupRSlider.setBounds (secondKnob);
    makeupSSlider.setBounds (secondKnob);
    controls.removeFromLeft (controlGap);

    auto mixArea = controls.removeFromLeft (mainW);
    mixLabel.setBounds (mixArea.removeFromTop (22));
    mixSlider.setBounds (mixArea.withSizeKeepingCentre (150, 140));
    controls.removeFromLeft (controlGap);

    auto outputArea = controls.removeFromLeft (smallTrimW);
    outputGainLabel.setBounds (outputArea.removeFromTop (22));
    outputGainSlider.setBounds (outputArea.withSizeKeepingCentre (112, 138));
    controls.removeFromLeft (controlGap);

    auto modeArea = controls;
    modeLabel.setBounds (modeArea.removeFromTop (20));
    auto modeButtonArea = modeArea.removeFromTop (40);
    modeButton.setBounds (modeButtonArea.withSizeKeepingCentre (juce::jmin (120, modeButtonArea.getWidth() - 20), 36));
    modeArea.removeFromTop (2);
    lookaheadLabel.setBounds (modeArea.removeFromTop (16));
    auto lookaheadComboArea = modeArea.removeFromTop (28);
    lookaheadCombo.setBounds (lookaheadComboArea.withSizeKeepingCentre (112, 26));
    modeArea.removeFromTop (2);
    oversamplingLabel.setBounds (modeArea.removeFromTop (16));
    auto oversamplingButtonArea = modeArea.removeFromTop (28);
    oversamplingButton.setBounds (oversamplingButtonArea.withSizeKeepingCentre (112, 26));
}
