from pathlib import Path

root = Path(__file__).parent
def edit(file, pairs):
    p = root / file
    s = p.read_text(encoding='utf-8-sig')
    for a, b in pairs:
        assert a in s, (file, a[:100])
        s = s.replace(a, b)
    p.write_text(s, encoding='utf-8', newline='\n')

edit('CMakeLists.txt', [('VERSION 1.2.3', 'VERSION 1.2.4'),
    ('with fixed dB Ratio and independent', 'with Classic / Super algorithms and independent')])
edit('Source/Parameters.h', [('    inline constexpr auto inputGainDb',
    '    inline constexpr auto algorithmMode = "algorithmMode"; // Appended host parameter in v1.2.4\n'
    '    enum AlgorithmMode { classicAlgorithm = 0, superAlgorithm = 1 };\n\n    inline constexpr auto inputGainDb')])
edit('Source/StaticCompressionEngine.h', [
    ('namespace qqsc\n{', 'namespace qqsc\n{\nenum class CompressionAlgorithm { classic, super };'),
    ('// This independent dB experiment preserves the legacy equation only at -inf.',
     '// Classic preserves the legacy equation at -inf. Super uses the original\n// rational law at both finite and -inf thresholds.'),
    ('static float gainForLevel (float level, float ratio, float thresholdLinear) noexcept',
     'static float gainForLevel (float level, float ratio, float thresholdLinear,\n                              CompressionAlgorithm algorithm = CompressionAlgorithm::classic) noexcept'),
    ('        // Fixed dB ratio above a finite threshold:',
     '        if (algorithm == CompressionAlgorithm::super)\n'
     '            return (1.0f + (ratio - 1.0f) * thresholdLinear)\n'
     '                 / juce::jmax (1.0e-9f, 1.0f + (ratio - 1.0f) * level);\n\n'
     '        // Fixed dB ratio above a finite threshold:'),
    ('static float singleGainForLevel (float level, float ratio, float lower, float upper) noexcept',
     'static float singleGainForLevel (float level, float ratio, float lower, float upper,\n                                    CompressionAlgorithm algorithm = CompressionAlgorithm::classic) noexcept'),
    ('const auto gain = gainForLevel (level, ratio, lower);',
     'const auto gain = gainForLevel (level, ratio, lower, algorithm);'),
    ('gatedUpwardGain (level, ratio, lower, juce::jmin (1.0f, upper));',
     'gatedUpwardGain (level, ratio, lower, juce::jmin (1.0f, upper), algorithm);'),
    ('static float gatedUpwardGain (float level, float ratio, float lower, float anchor) noexcept',
     'static float gatedUpwardGain (float level, float ratio, float lower, float anchor,\n                                 CompressionAlgorithm algorithm = CompressionAlgorithm::classic) noexcept'),
    ('        const auto gain = level >= anchor || ratio == 1.0f ? 1.0f\n            : juce::jmin (maximumUpwardGain, std::pow (anchor / level, 1.0f - ratio));',
     '        const auto gain = algorithm == CompressionAlgorithm::super\n'
     '            ? upwardGainForLevel (level, ratio, anchor)\n'
     '            : (level >= anchor || ratio == 1.0f ? 1.0f\n'
     '                : juce::jmin (maximumUpwardGain, std::pow (anchor / level, 1.0f - ratio)));'),
    ('float upThreshold, float downThreshold) noexcept',
     'float upThreshold, float downThreshold,\n                                   CompressionAlgorithm algorithm = CompressionAlgorithm::classic) noexcept'),
    ('gatedUpwardGain (level, upRatio, upThreshold, downThreshold);',
     'gatedUpwardGain (level, upRatio, upThreshold, downThreshold, algorithm);'),
    ('gainForLevel (level, downRatio, downThreshold);',
     'gainForLevel (level, downRatio, downThreshold, algorithm);')])
edit('Source/PluginProcessor.h', [
    ('        int compressionMode =', '        int algorithmMode = qqsc::params::classicAlgorithm;\n        int compressionMode ='),
    ('    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 5> upEnableFades, downEnableFades;',
     '    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 5> upEnableFades, downEnableFades;\n'
     '    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> algorithmFade; // 0 Classic, 1 Super')])
edit('Source/PluginProcessor.cpp', [
    ('constexpr int currentStateSchemaVersion = 17; // v1.2.3 Stable: accepted fixed-dB finite-threshold curve',
     'constexpr int currentStateSchemaVersion = 18; // v1.2.4: Classic / Super choice, including A/B'),
    ('    if (dual)\n        return qqsc::StaticCompressionEngine::dualGainForLevel (level,\n            effectiveDualRatio (d, true), effectiveDualRatio (d, false), lower, upper);\n    return qqsc::StaticCompressionEngine::singleGainForLevel (level,\n        apvts.getRawParameterValue (qqsc::params::ratioIds[d])->load(), lower, upper);',
     '    const auto algorithm = apvts.getRawParameterValue (qqsc::params::algorithmMode)->load() >= 0.5f\n'
     '        ? qqsc::CompressionAlgorithm::super : qqsc::CompressionAlgorithm::classic;\n'
     '    if (dual)\n        return qqsc::StaticCompressionEngine::dualGainForLevel (level,\n'
     '            effectiveDualRatio (d, true), effectiveDualRatio (d, false), lower, upper, algorithm);\n'
     '    return qqsc::StaticCompressionEngine::singleGainForLevel (level,\n'
     '        apvts.getRawParameterValue (qqsc::params::ratioIds[d])->load(), lower, upper, algorithm);'),
    ('    return layout;\n}',
     '    // Preserve every existing host ID/index. This is a sound parameter,\n'
     '    // saved in both project state and A/B banks, not a global preference.\n'
     '    layout.add (std::make_unique<juce::AudioParameterChoice> (\n'
     '        juce::ParameterID { qqsc::params::algorithmMode, 1 }, "Compression Algorithm",\n'
     '        juce::StringArray { "Classic", "Super" }, qqsc::params::classicAlgorithm));\n'
     '    return layout;\n}'),
    ('    resetRatioSmoother (ratioSmoother,  ratioCurrent,  ratioTarget);',
     '    resetRatioSmoother (algorithmFade, algorithmFade.getCurrentValue(),\n'
     '                       apvts.getRawParameterValue (qqsc::params::algorithmMode)->load());\n'
     '    resetRatioSmoother (ratioSmoother,  ratioCurrent,  ratioTarget);'),
    ('    const bool dualCompression =',
     '    algorithmFade.setTargetValue (apvts.getRawParameterValue (qqsc::params::algorithmMode)->load());\n    const bool dualCompression ='),
    ('        std::array<float, 5> gains;\n',
     '        // One shared weight per INTERNAL sample keeps all domains aligned.\n'
     '        // Both laws see the same detector and delayed carrier. A linear\n'
     '        // gain blend is exactly a crossfade between their audio outputs;\n'
     '        // unity stays unity. Reversing mid-fade starts at the current weight.\n'
     '        const auto superAmount = algorithmFade.getNextValue();\n'
     '        std::array<float, 5> gains;\n'),
    ('            gains[d] = dualCompression\n                ? qqsc::StaticCompressionEngine::dualGainForLevel (levels[d], upRatio, downRatio, lowerBoundaries[d], upperBoundaries[d])\n                : qqsc::StaticCompressionEngine::singleGainForLevel (levels[d], singleRatios[d], lowerBoundaries[d], upperBoundaries[d]);',
     '            const auto evaluate = [&] (qqsc::CompressionAlgorithm algorithm)\n'
     '            {\n'
     '                return dualCompression\n'
     '                    ? qqsc::StaticCompressionEngine::dualGainForLevel (levels[d], upRatio, downRatio, lowerBoundaries[d], upperBoundaries[d], algorithm)\n'
     '                    : qqsc::StaticCompressionEngine::singleGainForLevel (levels[d], singleRatios[d], lowerBoundaries[d], upperBoundaries[d], algorithm);\n'
     '            };\n'
     '            if (superAmount <= 0.0f) gains[d] = evaluate (qqsc::CompressionAlgorithm::classic);\n'
     '            else if (superAmount >= 1.0f) gains[d] = evaluate (qqsc::CompressionAlgorithm::super);\n'
     '            else gains[d] = (1.0f - superAmount) * evaluate (qqsc::CompressionAlgorithm::classic)\n'
     '                          + superAmount * evaluate (qqsc::CompressionAlgorithm::super);'),
    ('    snapshot.compressionMode =',
     '    snapshot.algorithmMode = juce::jlimit (0, 1, juce::roundToInt (apvts.getRawParameterValue (qqsc::params::algorithmMode)->load()));\n    snapshot.compressionMode ='),
    ('    setActualParameterValue (qqsc::params::compressionMode, static_cast<float> (snapshot.compressionMode));',
     '    setActualParameterValue (qqsc::params::algorithmMode, static_cast<float> (snapshot.algorithmMode));\n'
     '    setActualParameterValue (qqsc::params::compressionMode, static_cast<float> (snapshot.compressionMode));'),
    ('        state.setProperty (abProperty (prefix + "compressionMode"), s.compressionMode, nullptr);',
     '        state.setProperty (abProperty (prefix + "algorithmMode"), s.algorithmMode, nullptr);\n'
     '        state.setProperty (abProperty (prefix + "compressionMode"), s.compressionMode, nullptr);'),
    ('            s.compressionMode = juce::jlimit',
     '            s.algorithmMode = juce::jlimit (0, 1, static_cast<int> (state.getProperty (abProperty (prefix + "algorithmMode"), s.algorithmMode)));\n'
     '            s.compressionMode = juce::jlimit'),
    ('state.setProperty ("qqscCurveVariant", "fixed-db-finite-threshold", nullptr);',
     'state.setProperty ("qqscCurveVariant", "classic-super-selectable", nullptr);'),
    ('            apvts.replaceState (state);',
     '            apvts.replaceState (state);\n'
     '            if (! stateContainsParameter (state, qqsc::params::algorithmMode))\n'
     '            {\n'
     '                // 1.2.3 / dB comparison states retain their fixed-dB law.\n'
     '                // Earlier saved projects retain the original rational law.\n'
     '                const bool fixedDb = schemaVersion >= 17\n'
     '                    || state.getProperty ("qqscCurveVariant").toString() == "fixed-db-finite-threshold";\n'
     '                setActualParameterValue (qqsc::params::algorithmMode, fixedDb ? 0.0f : 1.0f);\n'
     '            }')])
edit('Source/PluginEditor.h', [
    ('    juce::TextButton sidechainButton',
     '    juce::TextButton algorithmButton { "ALGO: CLASSIC" };\n    void updateAlgorithmUi();\n    juce::TextButton sidechainButton')])
edit('Source/PluginEditor.cpp', [
    ('    configureActionButton (themeButton);',
     '    configureActionButton (algorithmButton);\n'
     '    algorithmButton.getProperties().set ("qqscAlwaysLit", true);\n'
     '    algorithmButton.setComponentID ("algorithmMode");\n'
     '    algorithmButton.setTooltip ("Compression algorithm: Classic = fixed dB ratio; Super = original QQ curve. Click to switch with a 10 ms crossfade.");\n'
     '    contentRoot.addAndMakeVisible (algorithmButton);\n'
     '    registerKeyboardListener (algorithmButton);\n'
     '    algorithmButton.onClick = [this]\n'
     '    {\n'
     '        endLinkedGesture();\n'
     '        beginUndoTransaction ("Classic / Super Algorithm");\n'
     '        const auto current = processor.getAPVTS().getRawParameterValue (qqsc::params::algorithmMode)->load();\n'
     '        setChoiceParameter (qqsc::params::algorithmMode, current >= 0.5f ? 0 : 1);\n'
     '        updateAlgorithmUi();\n'
     '    };\n'
     '    updateAlgorithmUi();\n'
     '    configureActionButton (themeButton);'),
    ('&themeButton, &oversamplingButton, &sidechainButton', '&algorithmButton, &themeButton, &oversamplingButton, &sidechainButton'),
    ('    themeButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());',
     '    algorithmButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::warmAccent());\n'
     '    themeButton.setColour (juce::TextButton::buttonOnColourId, qqsc::ui::cyanAccent());'),
    ('void QQSuperCompressionAudioProcessorEditor::timerCallback()\n{',
     'void QQSuperCompressionAudioProcessorEditor::updateAlgorithmUi()\n'
     '{\n'
     '    const bool super = processor.getAPVTS().getRawParameterValue (qqsc::params::algorithmMode)->load() >= 0.5f;\n'
     '    algorithmButton.setButtonText (super ? "ALGO: SUPER" : "ALGO: CLASSIC");\n'
     '    algorithmButton.setToggleState (super, juce::dontSendNotification);\n'
     '}\n\n'
     'void QQSuperCompressionAudioProcessorEditor::timerCallback()\n{\n    updateAlgorithmUi();'),
    ('    right -= 86 + 12;',
     '    right -= 86 + smallGap;\n'
     '    algorithmButton.setBounds (right - 118, headerButtonY, 118, headerButtonH);')])
print('Implemented Classic / Super DSP, 10ms crossfade, state/AB migration, toolbar control.')
