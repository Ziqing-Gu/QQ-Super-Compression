from pathlib import Path
root=Path(__file__).parent
def edit(file,pairs):
    p=root/file;s=p.read_text(encoding='utf-8-sig')
    for a,b in pairs:
        assert a in s,(file,a[:100]);s=s.replace(a,b)
    p.write_text(s,encoding='utf-8',newline='\n')
edit('CMakeLists.txt',[('VERSION 1.2.4','VERSION 1.2.5')])
edit('Source/DynamicsLimits.h',[('inline constexpr float minimumUpRatio',
    'inline constexpr float classicThresholdMinimumGain = 1.0e-6f; // -120 dB, finite in Classic\ninline constexpr float minimumUpRatio')])
edit('Source/StaticCompressionEngine.h',[
    ('// Classic preserves the legacy equation at -inf. Super uses the original\n// rational law at both finite and -inf thresholds.',
     '// Classic uses a finite -120 dB minimum throughout. Super uses the original\n// rational law at both finite and -inf thresholds.'),
    ('        // Threshold OFF: exact pre-Threshold QQ law.\n',
     '        // The shared host endpoint is finite -120 dB in Classic; only\n'
     '        // Super interprets it as -inf and uses the old rational law.\n'
     '        if (algorithm == CompressionAlgorithm::classic)\n'
     '            thresholdLinear = juce::jmax (classicThresholdMinimumGain, thresholdLinear);\n\n'),
    ('        ratio = juce::jlimit (minimumUpRatio, maximumDownRatio, ratio);\n        if (lower >= upper',
     '        ratio = juce::jlimit (minimumUpRatio, maximumDownRatio, ratio);\n'
     '        if (algorithm == CompressionAlgorithm::classic)\n'
     '        {\n'
     '            lower = juce::jmax (classicThresholdMinimumGain, lower);\n'
     '            upper = juce::jmax (classicThresholdMinimumGain, upper);\n'
     '        }\n'
     '        if (lower >= upper'),
    ('        if (level <= lower || lower >= anchor) return 1.0f;\n        // -inf explicitly preserves the original QQ upward family.',
     '        if (algorithm == CompressionAlgorithm::classic)\n'
     '        {\n'
     '            lower = juce::jmax (classicThresholdMinimumGain, lower);\n'
     '            anchor = juce::jmax (classicThresholdMinimumGain, anchor);\n'
     '        }\n'
     '        if (level <= lower || lower >= anchor) return 1.0f;\n'
     '        // Only Super has a true -inf gate.'),
    ('        if (upThreshold >= downThreshold || level <= upThreshold)',
     '        if (algorithm == CompressionAlgorithm::classic)\n'
     '        {\n'
     '            upThreshold = juce::jmax (classicThresholdMinimumGain, upThreshold);\n'
     '            downThreshold = juce::jmax (classicThresholdMinimumGain, downThreshold);\n'
     '        }\n'
     '        if (upThreshold >= downThreshold || level <= upThreshold)')])
edit('Source/Parameters.h',[
    ('    // Threshold OFF is represented by the bottom endpoint. DSP maps this sentinel\n    // to a true zero-linear threshold, i.e. the exact pre-Threshold (-inf) law.',
     '    // Preserve the host numeric endpoint. Super interprets it as -inf;\n'
     '    // Classic interprets it as finite -120 dB inside its transfer functions.\n'
     '    // Keeping the per-algorithm mapping in the engine also preserves both\n'
     '    // sides of the Classic/Super crossfade at this shared stored value.'),
    ('    inline juce::String rangeText (float db)',
     '    inline juce::String boundaryText (float db, bool classic, int decimals = 2)\n'
     '    {\n'
     '        return classic || isThresholdEnabled (db)\n'
     '            ? juce::String (db, decimals) + " dB" : juce::String ("-inf dB");\n'
     '    }\n\n'
     '    inline juce::String rangeText (float db, bool classic = false)'),
    ('        if (! isThresholdEnabled (db)) return "-inf dB";',
     '        if (! classic && ! isThresholdEnabled (db)) return "-inf dB";')])
edit('Source/PluginProcessor.h',[
    ('    void initialiseDualRatioLinkPreference (bool enabled);',
     '    void initialiseDualRatioLinkPreference (bool enabled);\n'
     '    void initialiseInputOutputLinkPreference (bool enabled);\n'
     '    bool isClassicAlgorithm() const noexcept { return classicAlgorithmForText.load (std::memory_order_relaxed); }'),
    ('createParameterLayout();','createParameterLayout (const std::atomic<bool>* classicForText = nullptr);'),
    ('    std::atomic<bool> dualRatioLinkPreferenceInitialised { false };',
     '    std::atomic<bool> dualRatioLinkPreferenceInitialised { false };\n'
     '    std::atomic<bool> inputOutputLinkPreferenceInitialised { false };'),
    ('    juce::AudioProcessorValueTreeState apvts;',
     '    // Constructed before APVTS; host text callbacks never access a partly\n'
     '    // constructed parameter tree, and never take locks on the audio thread.\n'
     '    std::atomic<bool> classicAlgorithmForText { true };\n'
     '    juce::AudioProcessorValueTreeState apvts;')])
edit('Source/PluginProcessor.cpp',[
    ('currentStateSchemaVersion = 18; // v1.2.4: Classic / Super choice, including A/B',
     'currentStateSchemaVersion = 19; // v1.2.5: finite Classic floor and remembered I/O Link'),
    ('"QQSuperCompressionState", createParameterLayout())',
     '"QQSuperCompressionState", createParameterLayout (&classicAlgorithmForText))'),
    ('    startTimerHz (30);',
     '    apvts.addParameterListener (qqsc::params::algorithmMode, this);\n'
     '    apvts.addParameterListener (qqsc::params::inputOutputLink, this);\n'
     '    startTimerHz (30);'),
    ('    stopTimer();\n',
     '    stopTimer();\n'
     '    apvts.removeParameterListener (qqsc::params::algorithmMode, this);\n'
     '    apvts.removeParameterListener (qqsc::params::inputOutputLink, this);\n'),
    ('void QQSuperCompressionAudioProcessor::parameterChanged (const juce::String& id, float value)\n{',
     'void QQSuperCompressionAudioProcessor::parameterChanged (const juce::String& id, float value)\n{\n'
     '    if (id == qqsc::params::algorithmMode)\n'
     '    {\n'
     '        classicAlgorithmForText.store (value < 0.5f, std::memory_order_relaxed);\n'
     '        return;\n'
     '    }\n'
     '    if (id == qqsc::params::inputOutputLink)\n'
     '    {\n'
     '        inputOutputLinkPreferenceInitialised.store (true);\n'
     '        return;\n'
     '    }'),
    ('QQSuperCompressionAudioProcessor::createParameterLayout()',
     'QQSuperCompressionAudioProcessor::createParameterLayout (const std::atomic<bool>* classicForText)'),
    ('.withStringFromValueFunction ([] (float v, int)\n                {\n                    return qqsc::params::isThresholdEnabled (v) ? juce::String (v, 2) + " dB" : juce::String ("OFF");\n                })',
     '.withStringFromValueFunction ([classicForText] (float v, int)\n'
     '                {\n'
     '                    return qqsc::params::boundaryText (v, classicForText == nullptr || classicForText->load (std::memory_order_relaxed));\n'
     '                })'),
    ('.withStringFromValueFunction ([] (float db, int)\n                {\n                    return qqsc::params::isThresholdEnabled (db) ? juce::String (db, 2) + " dB" : juce::String ("-inf dB");\n                })',
     '.withStringFromValueFunction ([classicForText] (float db, int)\n'
     '                {\n'
     '                    return qqsc::params::boundaryText (db, classicForText == nullptr || classicForText->load (std::memory_order_relaxed));\n'
     '                })'),
    ('.withStringFromValueFunction ([] (float db, int) { return qqsc::params::rangeText (db); })',
     '.withStringFromValueFunction ([classicForText] (float db, int) { return qqsc::params::rangeText (db, classicForText == nullptr || classicForText->load (std::memory_order_relaxed)); })'),
    ('    // by A/B sound banks and defaults to the original independent trim behavior.',
     '    // by A/B sound banks. First use defaults ON; the editor restores the\n'
     '    // last explicit user choice once unless a project/host edit came first.'),
    ('"Input Output Gain Link", false));','"Input Output Gain Link", true));'),
    ('void QQSuperCompressionAudioProcessor::initialiseDualRatioLinkPreference (bool enabled)',
     'void QQSuperCompressionAudioProcessor::initialiseInputOutputLinkPreference (bool enabled)\n'
     '{\n'
     '    if (! inputOutputLinkPreferenceInitialised.exchange (true))\n'
     '        setActualParameterValue (qqsc::params::inputOutputLink, enabled ? 1.0f : 0.0f);\n'
     '}\n\n'
     'void QQSuperCompressionAudioProcessor::initialiseDualRatioLinkPreference (bool enabled)'),
    ('            dualRatioLinkPreferenceInitialised.store (true);',
     '            dualRatioLinkPreferenceInitialised.store (true);\n'
     '            inputOutputLinkPreferenceInitialised.store (true);')])
edit('Source/PluginEditor.cpp',[
    ('    processor.initialiseDualRatioLinkPreference (uiProperties == nullptr || uiProperties->getBoolValue ("dualRatioLink", true));',
     '    processor.initialiseDualRatioLinkPreference (uiProperties == nullptr || uiProperties->getBoolValue ("dualRatioLink", true));\n'
     '    processor.initialiseInputOutputLinkPreference (uiProperties == nullptr || uiProperties->getBoolValue ("inputOutputLink", true));'),
    ('    slider.textFromValueFunction = [] (double v)\n    {\n        return qqsc::params::isThresholdEnabled (static_cast<float> (v))\n            ? juce::String (v, 2) + " dB" : juce::String ("OFF");\n    };',
     '    slider.textFromValueFunction = [this] (double v)\n'
     '    { return qqsc::params::boundaryText (static_cast<float> (v), processor.isClassicAlgorithm()); };'),
    ('    if (thresholdPair)\n    {','    if (thresholdPair && ! processor.isClassicAlgorithm())\n    {'),
    ('            if (! qqsc::params::isThresholdEnabled (static_cast<float> (db)))',
     '            if (! processor.isClassicAlgorithm() && ! qqsc::params::isThresholdEnabled (static_cast<float> (db)))'),
    ('        setChoiceParameter (qqsc::params::inputOutputLink, enabled ? 1 : 0);\n        updateCompressionUi();',
     '        setChoiceParameter (qqsc::params::inputOutputLink, enabled ? 1 : 0);\n'
     '        if (uiProperties != nullptr)\n'
     '        {\n'
     '            uiProperties->reload();\n'
     '            uiProperties->setValue ("inputOutputLink", enabled);\n'
     '            uiProperties->saveIfNeeded();\n'
     '        }\n'
     '        updateCompressionUi();'),
    ('Keeps the current offset; no change when enabled.',
     'Keeps the current offset; no gain change when enabled. Your last Link choice is remembered for new instances.'),
    ('    algorithmButton.setToggleState (super, juce::dontSendNotification);',
     '    algorithmButton.setToggleState (super, juce::dontSendNotification);\n'
     '    refreshBoundaryReadouts();')])
edit('Source/DynamicDisplay.cpp',[
    ('(qqsc::params::isThresholdEnabled (db) ? juce::String (db, 1) : juce::String ("-inf"))',
     '(processor.isClassicAlgorithm() || qqsc::params::isThresholdEnabled (db) ? juce::String (db, 1) : juce::String ("-inf"))')])
for name in ['build-candidate.cmd','build-tests.cmd','package-candidate.ps1','install-candidate.ps1']:
    edit(name,[('1.2.4','1.2.5'),('QQSC-1.2.5-Algorithms','QQSC-1.2.5-ThresholdFloor')])
edit('install-candidate.ps1',[
    ('3EB16E8F6803EDFA606BD4C3388FD3661FB20036460DCD3AF88228A163E80F05',
     'E56C48964156FCC69308F4E9B11930A73FF1020DCCF05550AA856E148A143534')])
print('Implemented Classic finite floor and remembered default-ON I/O Link.')
