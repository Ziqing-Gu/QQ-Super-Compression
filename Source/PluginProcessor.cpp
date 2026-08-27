#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>

namespace
{
float peakToDb (float peak) noexcept
{
    return juce::Decibels::gainToDecibels (juce::jmax (peak, 0.0f), -120.0f);
}

float maxAbs (float a, float b) noexcept
{
    return juce::jmax (std::abs (a), std::abs (b));
}

constexpr auto abPrefix = "qqscAB_";
constexpr auto stateSchemaProperty = "qqscStateSchemaVersion";
constexpr int oversamplingSchemaVersion = 2; // v0.1.10: 0 ms-only 1x/8x/16x Oversampling schema
constexpr int currentStateSchemaVersion = 3; // v0.9.2: adds Input/Output Gain; OS schema itself is unchanged

juce::Identifier abProperty (const juce::String& suffix)
{
    return juce::Identifier (juce::String (abPrefix) + suffix);
}

juce::PropertiesFile::Options userPreferencesOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "QQSuperCompression";
    options.filenameSuffix = ".settings";
    options.folderName = "Qing Audio";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
   #if JUCE_MAC
    options.osxLibrarySubFolder = "Application Support";
   #endif
    return options;
}

float loadLastUserLookaheadMs()
{
    juce::PropertiesFile properties (userPreferencesOptions());

    // 26 ms is the first-run fallback for the fixed-preset design. User
    // PluginDoctor tests found it to be the shortest tested window where the
    // sharp distortion boundary had moved to about 20 Hz. Once the user makes
    // a selection, that selection becomes the default for future new instances.
    const auto stored = static_cast<float> (properties.getDoubleValue ("lastLookaheadMs", 26.0));
    return qqsc::params::snapLookaheadMs (stored);
}

bool stateContainsParameter (const juce::ValueTree& state, const char* parameterID)
{
    // APVTS serialises parameter children with an "id" property.
    for (const auto& child : state)
        if (child.getProperty ("id").toString() == parameterID)
            return true;

    return false;
}

std::optional<float> stateParameterNormalisedValue (const juce::ValueTree& state, const char* parameterID)
{
    for (const auto& child : state)
    {
        if (child.getProperty ("id").toString() == parameterID)
            return child.getProperty ("value").toString().getFloatValue();
    }

    return std::nullopt;
}

int migrateLegacy019OversamplingChoice (float oldNormalised) noexcept
{
    // v0.1.9 choices were 1x/2x/4x/8x. User PluginDoctor testing later
    // rejected 2x and 4x because aliasing remained severe. Preserve explicit
    // 1x; map every old oversampled choice to the new practical default 8x.
    const auto oldChoice = juce::jlimit (0, 3, juce::roundToInt (oldNormalised * 3.0f));
    return oldChoice == 0 ? 0 : 1;
}
}

QQSuperCompressionAudioProcessor::QQSuperCompressionAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                               .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                               .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "QQSuperCompressionState", createParameterLayout())
{
    // v0.1.10 intentionally keeps only 1x/8x/16x. User PluginDoctor tests
    // found 2x and 4x still left severe aliasing in the 0 ms flavour mode,
    // while the extra FIR latency of 8x/16x was small enough not to justify
    // keeping those intermediate choices. Index 0 is a true 1x dummy stage.
    oversamplers[0] = std::make_unique<juce::dsp::Oversampling<float>> (6u);
    for (size_t i = 1; i < oversamplers.size(); ++i)
    {
        const auto stageCount = static_cast<size_t> (qqsc::params::oversamplingStageCountForChoiceIndex (static_cast<int> (i)));
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            6u, stageCount, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    }

    snapshotA = captureCurrentSnapshot();
    snapshotB = snapshotA;
}

juce::AudioProcessorValueTreeState::ParameterLayout QQSuperCompressionAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qqsc::params::ratio, 1 }, "Ratio",
        juce::NormalisableRange<float> { 1.0f, 32.0f, 0.01f, 0.55f }, 8.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            return juce::String (v, v < 10.0f ? 2 : 1) + ":1";
        })));

    auto addMakeup = [&] (const char* id, const juce::String& name)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { -36.0f, 36.0f, 0.01f }, 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));
    };

    // Keep the original 0.1.2 parameter ID for ST/common Makeup so old candidate
    // projects retain their value. LR/MS get independent new parameters.
    addMakeup (qqsc::params::makeupGainDb,  "Makeup Gain ST");
    addMakeup (qqsc::params::makeupGainLDb, "Makeup Gain L");
    addMakeup (qqsc::params::makeupGainRDb, "Makeup Gain R");
    addMakeup (qqsc::params::makeupGainMDb, "Makeup Gain M");
    addMakeup (qqsc::params::makeupGainSDb, "Makeup Gain S");

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qqsc::params::mix, 1 }, "Mix",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // Keep the existing float parameter ID for 0.1.4 project/A-B compatibility,
    // but 0.1.5 UI/DSP snap it to the six approved presets. A future new
    // instance starts from the user's last manually selected preset.
    const auto defaultLookaheadMs = loadLastUserLookaheadMs();
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qqsc::params::lookaheadMs, 1 }, "Lookahead",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, defaultLookaheadMs,
        juce::AudioParameterFloatAttributes()
            .withLabel ("ms")
            .withStringFromValueFunction ([] (float v, int)
            {
                return juce::String (qqsc::params::snapLookaheadMs (v), 0) + " ms";
            })
            .withValueFromStringFunction ([] (const juce::String& s)
            {
                return qqsc::params::snapLookaheadMs (static_cast<float> (s.getDoubleValue()));
            })));

    // Keep Oversampling appended after the 0.1.8 parameter sequence. v0.1.10
    // narrows the choices to 1x/8x/16x and defaults to 8x. The parameter stores
    // the user's preferred 0 ms flavour; it is ignored (but preserved) whenever
    // Lookahead is 10 ms or longer.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { qqsc::params::processingMode, 1 }, "Processing Mode",
        qqsc::params::modeChoices(), qqsc::params::stereoLinked));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { qqsc::params::bypass, 1 }, "Bypass", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { qqsc::params::oversampling, 1 }, "Oversampling",
        qqsc::params::oversamplingChoices(), 1));

    // v0.9.2 appends new trim parameters after the complete legacy parameter
    // sequence so existing candidate-project parameter order/IDs are not disturbed.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qqsc::params::inputGainDb, 1 }, "Input Gain",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qqsc::params::outputGainDb, 1 }, "Output Gain",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return layout;
}

void QQSuperCompressionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = juce::jmax (1.0, sampleRate);

    // AudioProcessor::getBlockSize() is only the typical size, so reserve a
    // generous no-allocation ceiling for the oversampling scratch buffers
    // without making every plug-in instance reserve tens of megabytes. Normal
    // DAW block sizes are far below 16384 samples.
    configuredMaximumBlockSize = juce::jmax (16384, juce::jmax (1, samplesPerBlock));

    for (auto& oversampler : oversamplers)
    {
        oversampler->initProcessing (static_cast<size_t> (configuredMaximumBlockSize));
        oversampler->reset();
    }

    wetBaseBuffer.setSize (6, configuredMaximumBlockSize, false, true, true);
    wetBaseBuffer.clear();
    originalInputBuffer.setSize (2, configuredMaximumBlockSize, false, true, true);
    originalInputBuffer.clear();

    maxLookaheadSamplesBase = juce::jmax (0, static_cast<int> (std::ceil (currentSampleRate * 0.100)));
    maxLookaheadSamplesInternal = maxLookaheadSamplesBase; // 8x/16x are only legal at 0 ms, so no oversampled lookahead queue is needed.
    oversampledDelayCapacity = juce::jmax (2, maxLookaheadSamplesInternal + 2);
    oversampledLookaheadDelayBuffer.setSize (2, oversampledDelayCapacity, false, true, true);
    oversampledLookaheadDelayBuffer.clear();

    leftEngine.prepare (maxLookaheadSamplesInternal);
    rightEngine.prepare (maxLookaheadSamplesInternal);
    midEngine.prepare (maxLookaheadSamplesInternal);
    sideEngine.prepare (maxLookaheadSamplesInternal);

    int maxOversamplingLatency = 0;
    for (int index = 0; index < static_cast<int> (oversamplers.size()); ++index)
        maxOversamplingLatency = juce::jmax (maxOversamplingLatency, getOversamplingLatencySamples (index));

    dryDelayCapacity = juce::jmax (2, maxLookaheadSamplesBase + maxOversamplingLatency + 2);
    dryDelayBuffer.setSize (2, dryDelayCapacity, false, true, true);
    dryDelayBuffer.clear();
    originalDryDelayBuffer.setSize (2, dryDelayCapacity, false, true, true);
    originalDryDelayBuffer.clear();

    // Input/Makeup/Mix/Output trims remain host-rate operations. Ratio smoothing
    // runs in the selected internal domain and is re-timed whenever Oversampling changes.
    inputGainSmoother.reset (currentSampleRate, 0.010);
    makeupSTSmoother.reset (currentSampleRate, 0.010);
    makeupLSmoother.reset (currentSampleRate, 0.010);
    makeupRSmoother.reset (currentSampleRate, 0.010);
    makeupMSmoother.reset (currentSampleRate, 0.010);
    makeupSSmoother.reset (currentSampleRate, 0.010);
    mixSmoother.reset (currentSampleRate, 0.010);
    outputGainSmoother.reset (currentSampleRate, 0.010);

    inputGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::inputGainDb)->load()));
    ratioSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::ratio)->load());
    makeupSTSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load());
    makeupLSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load());
    makeupRSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load());
    makeupMSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load());
    makeupSSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load());
    mixSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::mix)->load() * 0.01f);
    outputGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::outputGainDb)->load()));

    gainReductionHoldDurationSamples = juce::jmax<int64_t> (1, static_cast<int64_t> (std::llround (currentSampleRate * 2.0)));
    resetGainReductionHold();

    currentLookaheadSamplesBase = -1;
    currentLookaheadSamplesInternal = -1;
    currentOversamplingIndex = -1;
    currentOversamplingFactor = 1;
    currentTotalLatencySamples = 0;
    updateProcessingConfiguration (true);

    loudnessMatch.prepare (currentSampleRate);
    resetMatchAccumulator();
    matchReady.store (false, std::memory_order_relaxed);
    resetMatchOnNextPlaybackBlock.store (true, std::memory_order_relaxed);
    lastTransportPlaying = false;
    lastTransportSample = -1;
    lastTransportBlockSize = 0;
}

void QQSuperCompressionAudioProcessor::releaseResources()
{
}

int QQSuperCompressionAudioProcessor::getOversamplingLatencySamples (int oversamplingIndex) const noexcept
{
    oversamplingIndex = juce::jlimit (0, static_cast<int> (oversamplers.size()) - 1, oversamplingIndex);
    if (oversamplers[static_cast<size_t> (oversamplingIndex)] == nullptr)
        return 0;

    return juce::jmax (0, juce::roundToInt (
        oversamplers[static_cast<size_t> (oversamplingIndex)]->getLatencyInSamples()));
}

int QQSuperCompressionAudioProcessor::getCombinedLatencySamples (float requestedLookaheadMs,
                                                                  int oversamplingIndex) const noexcept
{
    const auto presetMs = qqsc::params::snapLookaheadMs (requestedLookaheadMs);
    const auto lookaheadSamples = juce::jmax (0, static_cast<int> (
        std::round (currentSampleRate * static_cast<double> (presetMs) * 0.001)));
    const auto effectiveOversamplingIndex = qqsc::params::effectiveOversamplingChoiceIndex (presetMs, oversamplingIndex);
    return lookaheadSamples + getOversamplingLatencySamples (effectiveOversamplingIndex);
}

void QQSuperCompressionAudioProcessor::notifyHostProcessingLatency()
{
    const auto currentLookaheadMs = qqsc::params::snapLookaheadMs (
        apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load());
    const auto storedOversamplingChoice = juce::jlimit (0, 2,
        juce::roundToInt (apvts.getRawParameterValue (qqsc::params::oversampling)->load()));
    setLatencySamples (getCombinedLatencySamples (currentLookaheadMs, storedOversamplingChoice));
}

juce::dsp::Oversampling<float>& QQSuperCompressionAudioProcessor::getCurrentOversampler() noexcept
{
    const auto index = juce::jlimit (0, static_cast<int> (oversamplers.size()) - 1, currentOversamplingIndex);
    jassert (oversamplers[static_cast<size_t> (index)] != nullptr);
    return *oversamplers[static_cast<size_t> (index)];
}

void QQSuperCompressionAudioProcessor::resetOversampledCoreState() noexcept
{
    oversampledLookaheadDelayBuffer.clear();
    oversampledDelayWriteIndex = 0;
    detectorSampleCounter = 0;
    leftEngine.reset();
    rightEngine.reset();
    midEngine.reset();
    sideEngine.reset();
}

void QQSuperCompressionAudioProcessor::resetDryDelayState() noexcept
{
    dryDelayBuffer.clear();
    originalDryDelayBuffer.clear();
    dryDelayWriteIndex = 0;
}

void QQSuperCompressionAudioProcessor::resetAllProcessingState() noexcept
{
    resetOversampledCoreState();
    resetDryDelayState();
    for (auto& oversampler : oversamplers)
        oversampler->reset();
}

void QQSuperCompressionAudioProcessor::updateProcessingConfiguration (bool force)
{
    const auto requestedMs = qqsc::params::snapLookaheadMs (
        apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load());
    const auto requestedLookaheadBase = juce::jlimit (0, maxLookaheadSamplesBase,
        static_cast<int> (std::round (currentSampleRate * static_cast<double> (requestedMs) * 0.001)));
    const auto storedOversamplingChoice = juce::jlimit (0, 2,
        juce::roundToInt (apvts.getRawParameterValue (qqsc::params::oversampling)->load()));
    const auto requestedOversamplingIndex = qqsc::params::effectiveOversamplingChoiceIndex (requestedMs, storedOversamplingChoice);
    const auto requestedFactor = qqsc::params::oversamplingFactorForChoiceIndex (requestedOversamplingIndex);
    const auto requestedLookaheadInternal = requestedLookaheadBase * requestedFactor;
    const auto requestedOversamplingLatency = getOversamplingLatencySamples (requestedOversamplingIndex);
    const auto requestedTotalLatency = requestedLookaheadBase + requestedOversamplingLatency;

    const bool oversamplingChanged = requestedOversamplingIndex != currentOversamplingIndex;
    const bool lookaheadChanged = requestedLookaheadBase != currentLookaheadSamplesBase;

    if (! force && ! oversamplingChanged && ! lookaheadChanged)
        return;

    const auto ratioCurrent = ratioSmoother.getCurrentValue();
    const auto ratioTarget = juce::jmax (1.0f, apvts.getRawParameterValue (qqsc::params::ratio)->load());

    currentOversamplingIndex = requestedOversamplingIndex;
    currentOversamplingFactor = requestedFactor;
    currentLookaheadSamplesBase = requestedLookaheadBase;
    currentLookaheadSamplesInternal = requestedLookaheadInternal;
    currentTotalLatencySamples = requestedTotalLatency;

    leftEngine.setLookaheadSamples (requestedLookaheadInternal);
    rightEngine.setLookaheadSamples (requestedLookaheadInternal);
    midEngine.setLookaheadSamples (requestedLookaheadInternal);
    sideEngine.setLookaheadSamples (requestedLookaheadInternal);

    ratioSmoother.reset (currentSampleRate * currentOversamplingFactor, 0.010);
    if (force)
        ratioSmoother.setCurrentAndTargetValue (ratioTarget);
    else
    {
        ratioSmoother.setCurrentAndTargetValue (ratioCurrent);
        ratioSmoother.setTargetValue (ratioTarget);
    }

    if (force || oversamplingChanged)
    {
        // A sample-rate-domain change invalidates both FIR history and the
        // internal lookahead queue. Clear Dry as well so the first block after
        // a user factor change cannot mix a warm Dry path with a cold Wet path.
        // The host may perform one PDC realignment because total latency changed.
        resetAllProcessingState();
    }
    else
    {
        // Lookahead changed while staying in the same oversampled domain. Keep
        // buffered audio and rebuild the monotonic peak queues from the newest
        // internal samples, mirroring the 0.1.8 warm-change behaviour.
        detectorSampleCounter = 0;
        const auto ratioForWarmup = ratioTarget;
        const bool stereoBus = getTotalNumInputChannels() >= 2;

        for (int age = requestedLookaheadInternal; age >= 1; --age)
        {
            int index = oversampledDelayWriteIndex - age;
            while (index < 0)
                index += oversampledDelayCapacity;

            const auto l = oversampledLookaheadDelayBuffer.getSample (0, index);
            const auto r = oversampledLookaheadDelayBuffer.getSample (1, index);
            const auto m = stereoBus ? 0.5f * (l + r) : l;
            const auto side = stereoBus ? 0.5f * (l - r) : 0.0f;

            leftEngine.processSample (l, ratioForWarmup, detectorSampleCounter);
            rightEngine.processSample (r, ratioForWarmup, detectorSampleCounter);
            midEngine.processSample (m, ratioForWarmup, detectorSampleCounter);
            sideEngine.processSample (side, ratioForWarmup, detectorSampleCounter);
            ++detectorSampleCounter;
        }
    }

    // Detector window/factor changes make an old visual peak meaningless.
    resetGainReductionHold();

    // Bypass and Dry use the identical combined delay. JUCE FIR oversampling is
    // configured with integer latency so the host, Dry path and Wet path all use
    // the same exact sample count.
    setLatencySamples (currentTotalLatencySamples);
}

bool QQSuperCompressionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void QQSuperCompressionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto bypassed = apvts.getRawParameterValue (qqsc::params::bypass)->load() >= 0.5f;
    processBlockInternal (buffer, bypassed);
}

void QQSuperCompressionAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    processBlockInternal (buffer, true);
}

void QQSuperCompressionAudioProcessor::processBlockInternal (juce::AudioBuffer<float>& buffer, bool forceBypass)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numInputChannels; ch < numOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numInputChannels <= 0 || numSamples <= 0)
        return;

    // The prepared reserve is intentionally very large, but JUCE Oversampling
    // still requires the actual block not to exceed initProcessing().
    jassert (numSamples <= configuredMaximumBlockSize);
    if (numSamples > configuredMaximumBlockSize)
    {
        // This should not occur in normal plug-in hosts. Fail safely rather than
        // letting the oversampler write beyond its preallocated internal blocks.
        buffer.clear();
        return;
    }

    updateProcessingConfiguration();

    bool transportPlaying = false;
    int64_t transportSample = -1;
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            transportPlaying = position->getIsPlaying();
            if (auto samplePosition = position->getTimeInSamples())
                transportSample = *samplePosition;
        }
    }

    bool discontinuity = false;
    if (transportPlaying && lastTransportPlaying && transportSample >= 0 && lastTransportSample >= 0)
    {
        const auto expected = lastTransportSample + static_cast<int64_t> (lastTransportBlockSize);
        discontinuity = std::llabs (transportSample - expected) > static_cast<int64_t> (juce::jmax (8, numSamples * 2));
    }

    if (transportPlaying
        && (! lastTransportPlaying
            || discontinuity
            || resetMatchOnNextPlaybackBlock.exchange (false, std::memory_order_relaxed)))
    {
        resetMatchAccumulator();
        matchReady.store (false, std::memory_order_relaxed);
    }

    const bool stereoBus = numInputChannels >= 2;
    const int mode = juce::jlimit (static_cast<int> (qqsc::params::stereoLinked),
                                   static_cast<int> (qqsc::params::leftRight),
                                   static_cast<int> (apvts.getRawParameterValue (qqsc::params::processingMode)->load()));

    meterState.processingMode.store (mode, std::memory_order_relaxed);

    if (mode != gainReductionHoldMode)
        resetGainReductionHold (mode);

    inputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::inputGainDb)->load()));
    ratioSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::ratio)->load());
    makeupSTSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load());
    makeupLSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load());
    makeupRSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load());
    makeupMSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load());
    makeupSSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load());
    mixSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::mix)->load() * 0.01f);
    outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::outputGainDb)->load()));

    float meterMaxGrDb[2] { 0.0f, 0.0f };

    // v0.9.2 Input Gain is part of the audio signal path before detector/compression,
    // but the Dynamic Display Dry/Input reference must stay pre-Input-Gain. Keep an
    // untouched host-rate copy, then apply the smoothed Input Gain to the processing
    // buffer before Oversampling. This also means Mix=0 is the input-trimmed Dry path,
    // while true Bypass remains the untouched delayed input.
    for (int i = 0; i < numSamples; ++i)
    {
        const float originalL = buffer.getSample (0, i);
        const float originalR = stereoBus ? buffer.getSample (1, i) : 0.0f;
        originalInputBuffer.setSample (0, i, originalL);
        originalInputBuffer.setSample (1, i, originalR);

        const auto inputGain = inputGainSmoother.getNextValue();
        buffer.setSample (0, i, originalL * inputGain);
        if (stereoBus)
            buffer.setSample (1, i, originalR * inputGain);
    }

    // JUCE Oversampling owns the upsampled scratch block. Its configured channel
    // count is six so a single up/down pipeline can return all three exact
    // pre-Makeup mode variants simultaneously:
    //   0/1 = ST linked L/R, 2/3 = LR independent L/R, 4/5 = MS M/S.
    // This preserves the existing 0.1.6 behaviour where Match accumulates ST,
    // LR and MS in parallel, instead of silently making Match mode-dependent.
    const auto& hostInputBuffer = static_cast<const juce::AudioBuffer<float>&> (buffer);
    const juce::dsp::AudioBlock<const float> hostInputBlock (hostInputBuffer);
    auto& oversampler = getCurrentOversampler();
    auto oversampledBlock = oversampler.processSamplesUp (hostInputBlock);

    const auto internalNumSamples = static_cast<int> (oversampledBlock.getNumSamples());
    const auto expectedInternalSamples = numSamples * currentOversamplingFactor;
    jassert (internalNumSamples == expectedInternalSamples);
    jassert (oversampledBlock.getNumChannels() >= 6u);
    if (internalNumSamples != expectedInternalSamples || oversampledBlock.getNumChannels() < 6u)
    {
        // A failed/unprepared Oversampling block would otherwise become an
        // out-of-range access in Release. Fail silent and let the host call the
        // normal prepare/configuration path rather than touching invalid memory.
        buffer.clear();
        return;
    }

    for (int i = 0; i < internalNumSamples; ++i)
    {
        const float inputL = oversampledBlock.getSample (0, i);
        const float inputR = stereoBus ? oversampledBlock.getSample (1, i) : 0.0f;
        const float inputM = stereoBus ? 0.5f * (inputL + inputR) : inputL;
        const float inputS = stereoBus ? 0.5f * (inputL - inputR) : 0.0f;

        const auto ratioNow = ratioSmoother.getNextValue();

        const auto gainL = leftEngine.processSample  (inputL, ratioNow, detectorSampleCounter);
        const auto gainR = rightEngine.processSample (inputR, ratioNow, detectorSampleCounter);
        const auto gainM = midEngine.processSample   (inputM, ratioNow, detectorSampleCounter);
        const auto gainS = sideEngine.processSample  (inputS, ratioNow, detectorSampleCounter);
        const auto linkedGain = juce::jmin (gainL, gainR);

        oversampledLookaheadDelayBuffer.setSample (0, oversampledDelayWriteIndex, inputL);
        oversampledLookaheadDelayBuffer.setSample (1, oversampledDelayWriteIndex, inputR);

        int readIndex = oversampledDelayWriteIndex - currentLookaheadSamplesInternal;
        while (readIndex < 0)
            readIndex += oversampledDelayCapacity;

        const float dryLInternal = oversampledLookaheadDelayBuffer.getSample (0, readIndex);
        const float dryRInternal = stereoBus ? oversampledLookaheadDelayBuffer.getSample (1, readIndex) : 0.0f;
        const float dryMInternal = stereoBus ? 0.5f * (dryLInternal + dryRInternal) : dryLInternal;
        const float drySInternal = stereoBus ? 0.5f * (dryLInternal - dryRInternal) : 0.0f;

        if (++oversampledDelayWriteIndex >= oversampledDelayCapacity)
            oversampledDelayWriteIndex = 0;
        ++detectorSampleCounter;

        const float wetLinkedL = dryLInternal * linkedGain;
        const float wetLinkedR = dryRInternal * linkedGain;
        const float wetIndependentL = dryLInternal * gainL;
        const float wetIndependentR = dryRInternal * gainR;
        const float wetM = dryMInternal * gainM;
        const float wetS = drySInternal * gainS;

        oversampledBlock.setSample (0, i, wetLinkedL);
        oversampledBlock.setSample (1, i, wetLinkedR);
        oversampledBlock.setSample (2, i, wetIndependentL);
        oversampledBlock.setSample (3, i, wetIndependentR);
        oversampledBlock.setSample (4, i, wetM);
        oversampledBlock.setSample (5, i, wetS);

        float gr0 = 0.0f;
        float gr1 = 0.0f;
        if (mode == qqsc::params::midSide)
        {
            gr0 = midEngine.getCurrentGainReductionDb();
            gr1 = sideEngine.getCurrentGainReductionDb();
        }
        else if (mode == qqsc::params::leftRight)
        {
            gr0 = leftEngine.getCurrentGainReductionDb();
            gr1 = rightEngine.getCurrentGainReductionDb();
        }
        else
        {
            const auto linkedGr = juce::jmax (leftEngine.getCurrentGainReductionDb(),
                                               rightEngine.getCurrentGainReductionDb());
            gr0 = linkedGr;
            gr1 = linkedGr;
        }

        meterMaxGrDb[0] = juce::jmax (meterMaxGrDb[0], gr0);
        meterMaxGrDb[1] = juce::jmax (meterMaxGrDb[1], stereoBus || mode == qqsc::params::midSide ? gr1 : 0.0f);
    }

    auto wetBlock = juce::dsp::AudioBlock<float> (wetBaseBuffer)
                        .getSubsetChannelBlock (0, 6)
                        .getSubBlock (0, static_cast<size_t> (numSamples));
    oversampler.processSamplesDown (wetBlock);

    float graphInputPeak = 0.0f;
    float graphWetPeak = 0.0f;
    float graphOutputPeak = 0.0f;
    float meterInputPeak[2]  { 0.0f, 0.0f };
    float meterOutputPeak[2] { 0.0f, 0.0f };

    for (int i = 0; i < numSamples; ++i)
    {
        // buffer now contains the Input-Gain-adjusted signal used by the compressor.
        // originalInputBuffer remains the pre-Input-Gain reference for Display/Bypass.
        const float inputL = buffer.getSample (0, i);
        const float inputR = stereoBus ? buffer.getSample (1, i) : 0.0f;
        const float originalL = originalInputBuffer.getSample (0, i);
        const float originalR = stereoBus ? originalInputBuffer.getSample (1, i) : 0.0f;

        dryDelayBuffer.setSample (0, dryDelayWriteIndex, inputL);
        dryDelayBuffer.setSample (1, dryDelayWriteIndex, inputR);
        originalDryDelayBuffer.setSample (0, dryDelayWriteIndex, originalL);
        originalDryDelayBuffer.setSample (1, dryDelayWriteIndex, originalR);

        int dryReadIndex = dryDelayWriteIndex - currentTotalLatencySamples;
        while (dryReadIndex < 0)
            dryReadIndex += dryDelayCapacity;

        const float dryL = dryDelayBuffer.getSample (0, dryReadIndex);
        const float dryR = stereoBus ? dryDelayBuffer.getSample (1, dryReadIndex) : 0.0f;
        const float displayDryL = originalDryDelayBuffer.getSample (0, dryReadIndex);
        const float displayDryR = stereoBus ? originalDryDelayBuffer.getSample (1, dryReadIndex) : 0.0f;
        const float dryM = stereoBus ? 0.5f * (dryL + dryR) : dryL;
        const float dryS = stereoBus ? 0.5f * (dryL - dryR) : 0.0f;

        if (++dryDelayWriteIndex >= dryDelayCapacity)
            dryDelayWriteIndex = 0;

        const float wetLinkedL = wetBaseBuffer.getSample (0, i);
        const float wetLinkedR = stereoBus ? wetBaseBuffer.getSample (1, i) : 0.0f;
        const float wetIndependentL = wetBaseBuffer.getSample (2, i);
        const float wetIndependentR = stereoBus ? wetBaseBuffer.getSample (3, i) : 0.0f;
        const float wetM = wetBaseBuffer.getSample (4, i);
        const float wetS = stereoBus ? wetBaseBuffer.getSample (5, i) : 0.0f;

        if (transportPlaying)
        {
            loudnessMatch.processSample (dryL, stereoBus ? dryR : 0.0f,
                                         wetLinkedL, stereoBus ? wetLinkedR : 0.0f,
                                         wetIndependentL, stereoBus ? wetIndependentR : 0.0f,
                                         dryM, stereoBus ? dryS : 0.0f,
                                         wetM, stereoBus ? wetS : 0.0f);
        }

        const auto makeupST = juce::Decibels::decibelsToGain (makeupSTSmoother.getNextValue());
        const auto makeupL  = juce::Decibels::decibelsToGain (makeupLSmoother.getNextValue());
        const auto makeupR  = juce::Decibels::decibelsToGain (makeupRSmoother.getNextValue());
        const auto makeupM  = juce::Decibels::decibelsToGain (makeupMSmoother.getNextValue());
        const auto makeupS  = juce::Decibels::decibelsToGain (makeupSSmoother.getNextValue());
        const auto mix = mixSmoother.getNextValue();

        float wetL = wetLinkedL;
        float wetR = wetLinkedR;
        float processedL = wetLinkedL * makeupST;
        float processedR = wetLinkedR * makeupST;

        if (mode == qqsc::params::midSide)
        {
            wetL = stereoBus ? wetM + wetS : wetM;
            wetR = stereoBus ? wetM - wetS : 0.0f;

            const float processedM = wetM * makeupM;
            const float processedS = wetS * makeupS;
            processedL = stereoBus ? processedM + processedS : processedM;
            processedR = stereoBus ? processedM - processedS : 0.0f;

            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryM));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryS));
        }
        else if (mode == qqsc::params::leftRight)
        {
            wetL = wetIndependentL;
            wetR = wetIndependentR;
            processedL = wetL * makeupL;
            processedR = wetR * makeupR;

            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryL));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryR));
        }
        else
        {
            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryL));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryR));
        }

        const float mixedL = dryL + (processedL - dryL) * mix;
        const float mixedR = dryR + (processedR - dryR) * mix;
        const auto outputGain = outputGainSmoother.getNextValue();
        const float activeOutL = mixedL * outputGain;
        const float activeOutR = mixedR * outputGain;

        // True Bypass retains the same combined latency but bypasses Input Gain,
        // compression, Makeup, Mix and Output Gain. This preserves the established
        // latency-safe bypass contract while keeping Gain trims part of the effect.
        const float outL = forceBypass ? displayDryL : activeOutL;
        const float outR = forceBypass ? displayDryR : activeOutR;

        buffer.setSample (0, i, outL);
        if (stereoBus)
            buffer.setSample (1, i, outR);

        graphInputPeak = juce::jmax (graphInputPeak, stereoBus ? maxAbs (displayDryL, displayDryR) : std::abs (displayDryL));
        graphWetPeak = juce::jmax (graphWetPeak, stereoBus ? maxAbs (wetL, wetR) : std::abs (wetL));
        graphOutputPeak = juce::jmax (graphOutputPeak, stereoBus ? maxAbs (outL, outR) : std::abs (outL));

        if (mode == qqsc::params::midSide)
        {
            const float outM = stereoBus ? 0.5f * (outL + outR) : outL;
            const float outS = stereoBus ? 0.5f * (outL - outR) : 0.0f;
            meterOutputPeak[0] = juce::jmax (meterOutputPeak[0], std::abs (outM));
            meterOutputPeak[1] = juce::jmax (meterOutputPeak[1], std::abs (outS));
        }
        else
        {
            meterOutputPeak[0] = juce::jmax (meterOutputPeak[0], std::abs (outL));
            meterOutputPeak[1] = juce::jmax (meterOutputPeak[1], std::abs (outR));
        }
    }

    if (transportPlaying)
        updateMatchResults();

    lastTransportPlaying = transportPlaying;
    lastTransportSample = transportSample;
    lastTransportBlockSize = numSamples;

    meterState.inputDb.store  (peakToDb (graphInputPeak), std::memory_order_relaxed);
    meterState.wetDb.store    (peakToDb (graphWetPeak), std::memory_order_relaxed);
    meterState.outputDb.store (peakToDb (graphOutputPeak), std::memory_order_relaxed);

    meterState.inputDb0.store  (peakToDb (meterInputPeak[0]), std::memory_order_relaxed);
    meterState.inputDb1.store  (peakToDb (meterInputPeak[1]), std::memory_order_relaxed);
    meterState.outputDb0.store (peakToDb (meterOutputPeak[0]), std::memory_order_relaxed);
    meterState.outputDb1.store (peakToDb (meterOutputPeak[1]), std::memory_order_relaxed);
    meterState.gainReductionDb0.store (meterMaxGrDb[0], std::memory_order_relaxed);
    meterState.gainReductionDb1.store (meterMaxGrDb[1], std::memory_order_relaxed);

    updateGainReductionHoldChannel (0, meterMaxGrDb[0], numSamples);
    updateGainReductionHoldChannel (1, stereoBus ? meterMaxGrDb[1] : 0.0f, numSamples);
    meterState.gainReductionHoldDb0.store (gainReductionHoldDb[0], std::memory_order_relaxed);
    meterState.gainReductionHoldDb1.store (gainReductionHoldDb[1], std::memory_order_relaxed);
}

void QQSuperCompressionAudioProcessor::resetGainReductionHold (int mode) noexcept
{
    gainReductionHoldDb[0] = 0.0f;
    gainReductionHoldDb[1] = 0.0f;
    gainReductionHoldSamplesRemaining[0] = 0;
    gainReductionHoldSamplesRemaining[1] = 0;
    gainReductionHoldMode = mode;
    meterState.gainReductionHoldDb0.store (0.0f, std::memory_order_relaxed);
    meterState.gainReductionHoldDb1.store (0.0f, std::memory_order_relaxed);
}

void QQSuperCompressionAudioProcessor::updateGainReductionHoldChannel (int channel,
                                                                        float blockPeakGrDb,
                                                                        int blockSamples) noexcept
{
    if (channel < 0 || channel > 1)
        return;

    const auto currentPeak = juce::jmax (0.0f, blockPeakGrDb);

    // A genuinely deeper reduction becomes the new Hold immediately and starts
    // a fresh two-second timer. Otherwise the existing marker remains visible.
    if (currentPeak > gainReductionHoldDb[channel] + 0.0001f)
    {
        gainReductionHoldDb[channel] = currentPeak;
        gainReductionHoldSamplesRemaining[channel] = gainReductionHoldDurationSamples;
        return;
    }

    gainReductionHoldSamplesRemaining[channel] -= juce::jmax (0, blockSamples);
    if (gainReductionHoldSamplesRemaining[channel] <= 0)
    {
        // Automatic refresh: after two seconds without a deeper peak, jump the
        // marker to the current block value. If GR then rises again it follows
        // immediately and restarts the hold timer at the new maximum.
        gainReductionHoldDb[channel] = currentPeak;
        gainReductionHoldSamplesRemaining[channel] = gainReductionHoldDurationSamples;
    }
}

QQSuperCompressionAudioProcessor::ParameterSnapshot QQSuperCompressionAudioProcessor::captureCurrentSnapshot() const noexcept
{
    ParameterSnapshot snapshot;
    snapshot.inputGainDb = apvts.getRawParameterValue (qqsc::params::inputGainDb)->load();
    snapshot.ratio = apvts.getRawParameterValue (qqsc::params::ratio)->load();
    snapshot.makeupST = apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load();
    snapshot.makeupL = apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load();
    snapshot.makeupR = apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load();
    snapshot.makeupM = apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load();
    snapshot.makeupS = apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load();
    snapshot.mix = apvts.getRawParameterValue (qqsc::params::mix)->load();
    snapshot.outputGainDb = apvts.getRawParameterValue (qqsc::params::outputGainDb)->load();
    snapshot.lookaheadMs = qqsc::params::snapLookaheadMs (apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load());
    snapshot.oversampling = juce::jlimit (0, 2, juce::roundToInt (apvts.getRawParameterValue (qqsc::params::oversampling)->load()));
    snapshot.mode = juce::roundToInt (apvts.getRawParameterValue (qqsc::params::processingMode)->load());
    return snapshot;
}

void QQSuperCompressionAudioProcessor::setActualParameterValue (const char* parameterID, float actualValue)
{
    if (auto* parameter = apvts.getParameter (parameterID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (actualValue));
        parameter->endChangeGesture();
    }
}

void QQSuperCompressionAudioProcessor::applySnapshot (const ParameterSnapshot& snapshot)
{
    setActualParameterValue (qqsc::params::inputGainDb, snapshot.inputGainDb);
    setActualParameterValue (qqsc::params::ratio, snapshot.ratio);
    setActualParameterValue (qqsc::params::makeupGainDb, snapshot.makeupST);
    setActualParameterValue (qqsc::params::makeupGainLDb, snapshot.makeupL);
    setActualParameterValue (qqsc::params::makeupGainRDb, snapshot.makeupR);
    setActualParameterValue (qqsc::params::makeupGainMDb, snapshot.makeupM);
    setActualParameterValue (qqsc::params::makeupGainSDb, snapshot.makeupS);
    setActualParameterValue (qqsc::params::mix, snapshot.mix);
    setActualParameterValue (qqsc::params::outputGainDb, snapshot.outputGainDb);
    const auto snapshotLookaheadMs = qqsc::params::snapLookaheadMs (snapshot.lookaheadMs);
    setActualParameterValue (qqsc::params::lookaheadMs, snapshotLookaheadMs);
    setActualParameterValue (qqsc::params::oversampling, static_cast<float> (juce::jlimit (0, 2, snapshot.oversampling)));
    notifyHostProcessingLatency();
    setActualParameterValue (qqsc::params::processingMode, static_cast<float> (snapshot.mode));
}

void QQSuperCompressionAudioProcessor::refreshActiveSnapshot()
{
    const juce::ScopedLock lock (abLock);
    if (activeABSlot.load (std::memory_order_relaxed) == 0)
        snapshotA = captureCurrentSnapshot();
    else
        snapshotB = captureCurrentSnapshot();
}

void QQSuperCompressionAudioProcessor::selectABSlot (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    const auto current = activeABSlot.load (std::memory_order_relaxed);
    if (slot == current)
        return;

    ParameterSnapshot target;
    {
        const juce::ScopedLock lock (abLock);
        if (current == 0)
            snapshotA = captureCurrentSnapshot();
        else
            snapshotB = captureCurrentSnapshot();

        target = slot == 0 ? snapshotA : snapshotB;
        activeABSlot.store (slot, std::memory_order_relaxed);
    }

    undoManager.beginNewTransaction (slot == 0 ? "Select A" : "Select B");
    applySnapshot (target);
}

void QQSuperCompressionAudioProcessor::copyAToB()
{
    ParameterSnapshot copied;
    bool applyToCurrent = false;
    {
        const juce::ScopedLock lock (abLock);
        if (activeABSlot.load (std::memory_order_relaxed) == 0)
            snapshotA = captureCurrentSnapshot();
        else
            snapshotB = captureCurrentSnapshot();

        snapshotB = snapshotA;
        copied = snapshotB;
        applyToCurrent = activeABSlot.load (std::memory_order_relaxed) == 1;
    }

    if (applyToCurrent)
    {
        undoManager.beginNewTransaction ("Copy A to B");
        applySnapshot (copied);
    }
}

void QQSuperCompressionAudioProcessor::copyBToA()
{
    ParameterSnapshot copied;
    bool applyToCurrent = false;
    {
        const juce::ScopedLock lock (abLock);
        if (activeABSlot.load (std::memory_order_relaxed) == 0)
            snapshotA = captureCurrentSnapshot();
        else
            snapshotB = captureCurrentSnapshot();

        snapshotA = snapshotB;
        copied = snapshotA;
        applyToCurrent = activeABSlot.load (std::memory_order_relaxed) == 0;
    }

    if (applyToCurrent)
    {
        undoManager.beginNewTransaction ("Copy B to A");
        applySnapshot (copied);
    }
}

void QQSuperCompressionAudioProcessor::writeABStateTo (juce::ValueTree& state)
{
    refreshActiveSnapshot();
    const juce::ScopedLock lock (abLock);

    state.setProperty (abProperty ("active"), activeABSlot.load (std::memory_order_relaxed), nullptr);

    auto write = [&] (const juce::String& prefix, const ParameterSnapshot& s)
    {
        state.setProperty (abProperty (prefix + "inputGainDb"), s.inputGainDb, nullptr);
        state.setProperty (abProperty (prefix + "ratio"), s.ratio, nullptr);
        state.setProperty (abProperty (prefix + "makeupST"), s.makeupST, nullptr);
        state.setProperty (abProperty (prefix + "makeupL"), s.makeupL, nullptr);
        state.setProperty (abProperty (prefix + "makeupR"), s.makeupR, nullptr);
        state.setProperty (abProperty (prefix + "makeupM"), s.makeupM, nullptr);
        state.setProperty (abProperty (prefix + "makeupS"), s.makeupS, nullptr);
        state.setProperty (abProperty (prefix + "mix"), s.mix, nullptr);
        state.setProperty (abProperty (prefix + "outputGainDb"), s.outputGainDb, nullptr);
        state.setProperty (abProperty (prefix + "lookaheadMs"), s.lookaheadMs, nullptr);
        state.setProperty (abProperty (prefix + "oversampling"), s.oversampling, nullptr);
        state.setProperty (abProperty (prefix + "mode"), s.mode, nullptr);
    };

    write ("A_", snapshotA);
    write ("B_", snapshotB);
}

void QQSuperCompressionAudioProcessor::readABStateFrom (const juce::ValueTree& state, bool legacyOversamplingSchema)
{
    const auto fallback = captureCurrentSnapshot();
    const bool hasAB = state.hasProperty (abProperty ("A_ratio"));

    ParameterSnapshot newA = fallback;
    ParameterSnapshot newB = fallback;
    int newActive = 0;

    if (hasAB)
    {
        auto read = [&] (const juce::String& prefix, ParameterSnapshot& s)
        {
            s.inputGainDb = static_cast<float> (state.getProperty (abProperty (prefix + "inputGainDb"), s.inputGainDb));
            s.ratio = static_cast<float> (state.getProperty (abProperty (prefix + "ratio"), s.ratio));
            s.makeupST = static_cast<float> (state.getProperty (abProperty (prefix + "makeupST"), s.makeupST));
            s.makeupL = static_cast<float> (state.getProperty (abProperty (prefix + "makeupL"), s.makeupL));
            s.makeupR = static_cast<float> (state.getProperty (abProperty (prefix + "makeupR"), s.makeupR));
            s.makeupM = static_cast<float> (state.getProperty (abProperty (prefix + "makeupM"), s.makeupM));
            s.makeupS = static_cast<float> (state.getProperty (abProperty (prefix + "makeupS"), s.makeupS));
            s.mix = static_cast<float> (state.getProperty (abProperty (prefix + "mix"), s.mix));
            s.outputGainDb = static_cast<float> (state.getProperty (abProperty (prefix + "outputGainDb"), s.outputGainDb));
            s.lookaheadMs = qqsc::params::snapLookaheadMs (
                static_cast<float> (state.getProperty (abProperty (prefix + "lookaheadMs"), s.lookaheadMs)));
            const auto storedOversampling = static_cast<int> (state.getProperty (abProperty (prefix + "oversampling"), s.oversampling));
            s.oversampling = legacyOversamplingSchema
                ? (storedOversampling <= 0 ? 0 : 1)
                : juce::jlimit (0, 2, storedOversampling);
            s.mode = static_cast<int> (state.getProperty (abProperty (prefix + "mode"), s.mode));
        };

        read ("A_", newA);
        read ("B_", newB);
        newActive = juce::jlimit (0, 1, static_cast<int> (state.getProperty (abProperty ("active"), 0)));
    }

    const juce::ScopedLock lock (abLock);
    snapshotA = newA;
    snapshotB = newB;
    activeABSlot.store (newActive, std::memory_order_relaxed);
}

void QQSuperCompressionAudioProcessor::resetMatchAccumulator() noexcept
{
    loudnessMatch.reset();
    matchSTValid.store (false, std::memory_order_relaxed);
    matchLValid.store  (false, std::memory_order_relaxed);
    matchRValid.store  (false, std::memory_order_relaxed);
    matchMValid.store  (false, std::memory_order_relaxed);
    matchSValid.store  (false, std::memory_order_relaxed);
}

void QQSuperCompressionAudioProcessor::updateMatchResults() noexcept
{
    const auto& result = loudnessMatch.getLatestMatch();

    matchSTDb.store (result.st, std::memory_order_relaxed);
    matchLDb.store  (result.l,  std::memory_order_relaxed);
    matchRDb.store  (result.r,  std::memory_order_relaxed);
    matchMDb.store  (result.m,  std::memory_order_relaxed);
    matchSDb.store  (result.s,  std::memory_order_relaxed);
    matchSTValid.store (result.validST, std::memory_order_relaxed);
    matchLValid.store  (result.validL,  std::memory_order_relaxed);
    matchRValid.store  (result.validR,  std::memory_order_relaxed);
    matchMValid.store  (result.validM,  std::memory_order_relaxed);
    matchSValid.store  (result.validS,  std::memory_order_relaxed);

    const int mode = juce::jlimit (static_cast<int> (qqsc::params::stereoLinked),
                                   static_cast<int> (qqsc::params::leftRight),
                                   juce::roundToInt (apvts.getRawParameterValue (qqsc::params::processingMode)->load()));

    bool ready = result.validST;
    if (mode == qqsc::params::leftRight)
        ready = result.validL || result.validR;
    else if (mode == qqsc::params::midSide)
        ready = result.validM || result.validS;

    matchReady.store (ready, std::memory_order_relaxed);
}

bool QQSuperCompressionAudioProcessor::hasMatchData() const noexcept
{
    if (! matchReady.load (std::memory_order_relaxed))
        return false;

    const int mode = juce::jlimit (static_cast<int> (qqsc::params::stereoLinked),
                                   static_cast<int> (qqsc::params::leftRight),
                                   juce::roundToInt (apvts.getRawParameterValue (qqsc::params::processingMode)->load()));

    if (mode == qqsc::params::leftRight)
        return matchLValid.load (std::memory_order_relaxed) || matchRValid.load (std::memory_order_relaxed);
    if (mode == qqsc::params::midSide)
        return matchMValid.load (std::memory_order_relaxed) || matchSValid.load (std::memory_order_relaxed);
    return matchSTValid.load (std::memory_order_relaxed);
}

bool QQSuperCompressionAudioProcessor::applyMatchForCurrentMode()
{
    if (! hasMatchData())
        return false;

    const int mode = juce::jlimit (static_cast<int> (qqsc::params::stereoLinked),
                                   static_cast<int> (qqsc::params::leftRight),
                                   juce::roundToInt (apvts.getRawParameterValue (qqsc::params::processingMode)->load()));

    undoManager.beginNewTransaction ("Match Makeup Gain");

    if (mode == qqsc::params::leftRight)
    {
        if (matchLValid.load (std::memory_order_relaxed))
            setActualParameterValue (qqsc::params::makeupGainLDb, matchLDb.load (std::memory_order_relaxed));
        if (matchRValid.load (std::memory_order_relaxed))
            setActualParameterValue (qqsc::params::makeupGainRDb, matchRDb.load (std::memory_order_relaxed));
    }
    else if (mode == qqsc::params::midSide)
    {
        if (matchMValid.load (std::memory_order_relaxed))
            setActualParameterValue (qqsc::params::makeupGainMDb, matchMDb.load (std::memory_order_relaxed));
        if (matchSValid.load (std::memory_order_relaxed))
            setActualParameterValue (qqsc::params::makeupGainSDb, matchSDb.load (std::memory_order_relaxed));
    }
    else if (matchSTValid.load (std::memory_order_relaxed))
    {
        setActualParameterValue (qqsc::params::makeupGainDb, matchSTDb.load (std::memory_order_relaxed));
    }

    matchReady.store (false, std::memory_order_relaxed);
    resetMatchOnNextPlaybackBlock.store (true, std::memory_order_relaxed);
    return true;
}

juce::AudioProcessorParameter* QQSuperCompressionAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter (qqsc::params::bypass);
}

void QQSuperCompressionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (stateSchemaProperty, currentStateSchemaVersion, nullptr);
    writeABStateTo (state);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void QQSuperCompressionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            const auto schemaVersion = static_cast<int> (state.getProperty (stateSchemaProperty, 0));
            const bool legacyOversamplingSchema = schemaVersion < oversamplingSchemaVersion;
            const bool stateHasOversampling = stateContainsParameter (state, qqsc::params::oversampling);
            const bool stateHasInputGain = stateContainsParameter (state, qqsc::params::inputGainDb);
            const bool stateHasOutputGain = stateContainsParameter (state, qqsc::params::outputGainDb);
            const auto legacyOversamplingNormalised = stateParameterNormalisedValue (state, qqsc::params::oversampling);
            apvts.replaceState (state);

            // Pre-0.9.2 projects have no trim parameters. They migrate explicitly
            // to unity gain so loading an older project cannot acquire a hidden
            // level change from whatever value a newly-created instance had.
            if (! stateHasInputGain)
                setActualParameterValue (qqsc::params::inputGainDb, 0.0f);
            if (! stateHasOutputGain)
                setActualParameterValue (qqsc::params::outputGainDb, 0.0f);

            // 0.1.8-or-earlier state: there was no Oversampling parameter. New
            // behaviour defaults the remembered 0 ms flavour choice to 8x.
            if (! stateHasOversampling)
            {
                if (auto* parameter = apvts.getParameter (qqsc::params::oversampling))
                    parameter->setValueNotifyingHost (parameter->convertTo0to1 (1.0f));
            }
            // 0.1.9 state: old choices were 1x/2x/4x/8x. Preserve explicit 1x;
            // map every oversampled old choice to 8x because 2x/4x were rejected
            // by user PluginDoctor testing and 8x is the new default.
            else if (legacyOversamplingSchema && legacyOversamplingNormalised.has_value())
            {
                const auto migratedChoice = migrateLegacy019OversamplingChoice (*legacyOversamplingNormalised);
                if (auto* parameter = apvts.getParameter (qqsc::params::oversampling))
                    parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (migratedChoice)));
            }

            // 0.1.4 allowed arbitrary values. 0.1.5 migrates any such project
            // value to the nearest approved preset while preserving the same
            // parameter ID so old candidate projects still load.
            const auto restoredLookahead = apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load();
            const auto snappedLookahead = qqsc::params::snapLookaheadMs (restoredLookahead);
            if (std::abs (restoredLookahead - snappedLookahead) > 0.0001f)
            {
                if (auto* parameter = apvts.getParameter (qqsc::params::lookaheadMs))
                    parameter->setValueNotifyingHost (parameter->convertTo0to1 (snappedLookahead));
            }

            readABStateFrom (state, legacyOversamplingSchema);
            notifyHostProcessingLatency();
        }
    }
}

juce::AudioProcessorEditor* QQSuperCompressionAudioProcessor::createEditor()
{
    return new QQSuperCompressionAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QQSuperCompressionAudioProcessor();
}
