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
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (editorAspectRatio);

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
    title.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::bold)));
    contentRoot.addAndMakeVisible (title);

    // Version is intentionally small and subdued: useful for screenshots/build
    // identification without becoming another explanatory subtitle. The string
    // comes from the CMake/JUCE plug-in version so UI and binary metadata match.
    versionLabel.setText (juce::String ("v") + JucePlugin_VersionString, juce::dontSendNotification);
    versionLabel.setJustificationType (juce::Justification::centredLeft);
    versionLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.34f));
    versionLabel.setFont (juce::Font (juce::FontOptions (9.0f)));
    contentRoot.addAndMakeVisible (versionLabel);

    // The old explanatory subtitle was deliberately removed in 0.1.3 at the
    // user's request. Only the small build/version identifier remains.
    contentRoot.addAndMakeVisible (display);
    contentRoot.addAndMakeVisible (meters);

    configureLabel (ratioLabel, "RATIO");
    configureLabel (makeupLabel, "MAKEUP");
    configureLabel (makeupChannel0Label, "L");
    configureLabel (makeupChannel1Label, "R");
    configureLabel (mixLabel, "MIX");
    configureLabel (modeLabel, "MODE");
    configureLabel (lookaheadLabel, "LOOKAHEAD (ms)");

    for (auto* label : { &ratioLabel, &makeupLabel, &makeupChannel0Label, &makeupChannel1Label, &mixLabel, &modeLabel, &lookaheadLabel })
        contentRoot.addAndMakeVisible (*label);

    configureKnob (ratioSlider);
    ratioSlider.textFromValueFunction = [] (double v)
    {
        return juce::String (v, v < 10.0 ? 2 : 1) + ":1";
    };

    for (auto* makeup : { &makeupSTSlider, &makeupLSlider, &makeupRSlider, &makeupMSlider, &makeupSSlider })
        configureKnob (*makeup, " dB");
    configureKnob (mixSlider, " %");

    for (auto* slider : { &ratioSlider, &makeupSTSlider, &makeupLSlider, &makeupRSlider,
                          &makeupMSlider, &makeupSSlider, &mixSlider })
    {
        contentRoot.addAndMakeVisible (*slider);
        registerKeyboardListener (*slider);
    }

    lookaheadCombo.addItemList (qqsc::params::lookaheadChoices(), 1);
    lookaheadCombo.setJustificationType (juce::Justification::centred);
    lookaheadCombo.setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (31, 35, 42));
    lookaheadCombo.setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.34f));
    lookaheadCombo.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    lookaheadCombo.setColour (juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha (0.82f));
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

    aToBButton.setButtonText (arrowText ("A", "B"));
    bToAButton.setButtonText (arrowText ("B", "A"));

    bypassButton.setClickingTogglesState (true);
    aButton.setClickingTogglesState (false);
    bButton.setClickingTogglesState (false);

    for (auto* button : { &modeButton, &matchButton, &bypassButton, &aButton, &bButton, &aToBButton, &bToAButton })
    {
        contentRoot.addAndMakeVisible (*button);
        registerKeyboardListener (*button);
    }

    auto& state = processor.getAPVTS();
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::ratio, ratioSlider);
    makeupSTAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainDb, makeupSTSlider);
    makeupLAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainLDb, makeupLSlider);
    makeupRAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainRDb, makeupRSlider);
    makeupMAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainMDb, makeupMSlider);
    makeupSAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::makeupGainSDb, makeupSSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, qqsc::params::mix, mixSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, qqsc::params::bypass, bypassButton);

    ratioSlider.onGestureStart = [this] { beginUndoTransaction ("Ratio"); };
    makeupSTSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup ST"); };
    makeupLSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup L"); };
    makeupRSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup R"); };
    makeupMSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup M"); };
    makeupSSlider.onGestureStart = [this] { beginUndoTransaction ("Makeup S"); };
    mixSlider.onGestureStart = [this] { beginUndoTransaction ("Mix"); };

    modeButton.onClick = [this] { cycleMode(); };
    matchButton.onClick = [this] { processor.applyMatchForCurrentMode(); };
    aButton.onClick = [this] { processor.selectABSlot (0); };
    bButton.onClick = [this] { processor.selectABSlot (1); };
    aToBButton.onClick = [this] { processor.copyAToB(); };
    bToAButton.onClick = [this] { processor.copyBToA(); };

    registerKeyboardListener (*this);
    setWantsKeyboardFocus (true);

    updateModeUi();
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
}

void QQSuperCompressionAudioProcessorEditor::configureLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.64f));
    label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
}

void QQSuperCompressionAudioProcessorEditor::configureActionButton (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (31, 35, 42));
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromRGB (57, 161, 201));
    button.setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha (0.80f));
    button.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
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
            processor.notifyHostLookaheadLatency (value);
        }
    }

    // User preference, not project state: new instances use the last manually
    // selected preset, while an existing project still restores its own APVTS
    // Lookahead value when the host reloads the instance.
    if (uiProperties != nullptr)
    {
        uiProperties->setValue ("lastLookaheadMs", value);
        uiProperties->saveIfNeeded();
    }
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

    const auto lookaheadIndex = qqsc::params::lookaheadChoiceIndexForMs (
        processor.getAPVTS().getRawParameterValue (qqsc::params::lookaheadMs)->load());
    if (lookaheadCombo.getSelectedItemIndex() != lookaheadIndex)
        lookaheadCombo.setSelectedItemIndex (lookaheadIndex, juce::dontSendNotification);
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
    g.fillAll (juce::Colour::fromRGB (9, 10, 13));

    const auto uiScale = static_cast<float> (getWidth()) / defaultEditorWidth;
    const auto headerHeight = 70.0f * uiScale;

    g.setColour (juce::Colour::fromRGB (17, 19, 24));
    g.fillRect (0.0f, 0.0f, static_cast<float> (getWidth()), headerHeight);

    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine (static_cast<int> (std::lround (headerHeight)),
                          0.0f, static_cast<float> (getWidth()));
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
    const int columnW = controls.getWidth() / 4;

    auto ratioArea = controls.removeFromLeft (columnW);
    ratioLabel.setBounds (ratioArea.removeFromTop (22));
    ratioSlider.setBounds (ratioArea.reduced (16, 0));

    auto makeupArea = controls.removeFromLeft (columnW);
    auto makeupHeader = makeupArea.removeFromTop (24);
    auto matchArea = makeupHeader.removeFromRight (68).reduced (2, 1);
    makeupLabel.setBounds (makeupHeader);
    matchButton.setBounds (matchArea);

    // ST shows one common Makeup knob. LR and MS show two independent knobs.
    makeupSTSlider.setBounds (makeupArea.reduced (24, 0));

    auto dualMakeup = makeupArea;
    const int dualW = dualMakeup.getWidth() / 2;
    auto first = dualMakeup.removeFromLeft (dualW);
    auto second = dualMakeup;

    makeupChannel0Label.setBounds (first.removeFromTop (16));
    makeupChannel1Label.setBounds (second.removeFromTop (16));

    const auto firstKnob = first.reduced (4, 0);
    const auto secondKnob = second.reduced (4, 0);
    makeupLSlider.setBounds (firstKnob);
    makeupMSlider.setBounds (firstKnob);
    makeupRSlider.setBounds (secondKnob);
    makeupSSlider.setBounds (secondKnob);

    auto mixArea = controls.removeFromLeft (columnW);
    mixLabel.setBounds (mixArea.removeFromTop (22));
    mixSlider.setBounds (mixArea.reduced (16, 0));

    auto modeArea = controls;
    modeLabel.setBounds (modeArea.removeFromTop (22));
    auto modeButtonArea = modeArea.removeFromTop (46);
    modeButton.setBounds (modeButtonArea.withSizeKeepingCentre (juce::jmin (120, modeButtonArea.getWidth() - 28), 38));
    modeArea.removeFromTop (4);
    lookaheadLabel.setBounds (modeArea.removeFromTop (20));
    auto lookaheadComboArea = modeArea.removeFromTop (34);
    lookaheadCombo.setBounds (lookaheadComboArea.withSizeKeepingCentre (112, 30));
}
