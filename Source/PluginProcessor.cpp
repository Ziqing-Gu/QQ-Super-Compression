#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

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
}

QQSuperCompressionAudioProcessor::QQSuperCompressionAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                               .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                               .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "QQSuperCompressionState", createParameterLayout())
{
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

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { qqsc::params::processingMode, 1 }, "Processing Mode",
        qqsc::params::modeChoices(), qqsc::params::stereoLinked));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { qqsc::params::bypass, 1 }, "Bypass", false));

    return layout;
}

void QQSuperCompressionAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = juce::jmax (1.0, sampleRate);
    maxLookaheadSamples = juce::jmax (0, static_cast<int> (std::ceil (currentSampleRate * 0.100)));
    delayCapacity = juce::jmax (2, maxLookaheadSamples + 2);
    lookaheadDelayBuffer.setSize (2, delayCapacity, false, true, true);
    lookaheadDelayBuffer.clear();

    leftEngine.prepare (maxLookaheadSamples);
    rightEngine.prepare (maxLookaheadSamples);
    midEngine.prepare (maxLookaheadSamples);
    sideEngine.prepare (maxLookaheadSamples);

    ratioSmoother.reset (sampleRate, 0.010);
    makeupSTSmoother.reset (sampleRate, 0.010);
    makeupLSmoother.reset (sampleRate, 0.010);
    makeupRSmoother.reset (sampleRate, 0.010);
    makeupMSmoother.reset (sampleRate, 0.010);
    makeupSSmoother.reset (sampleRate, 0.010);
    mixSmoother.reset (sampleRate, 0.010);

    ratioSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::ratio)->load());
    makeupSTSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load());
    makeupLSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load());
    makeupRSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load());
    makeupMSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load());
    makeupSSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load());
    mixSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::mix)->load() * 0.01f);

    gainReductionHoldDurationSamples = juce::jmax<int64_t> (1, static_cast<int64_t> (std::llround (currentSampleRate * 2.0)));
    resetGainReductionHold();

    currentLookaheadSamples = -1;
    updateLookaheadConfiguration (true);

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

void QQSuperCompressionAudioProcessor::notifyHostLookaheadLatency (float lookaheadMs)
{
    const auto presetMs = qqsc::params::snapLookaheadMs (lookaheadMs);
    const auto samples = juce::jmax (0, static_cast<int> (std::round (currentSampleRate * static_cast<double> (presetMs) * 0.001)));
    setLatencySamples (samples);
}

void QQSuperCompressionAudioProcessor::resetLookaheadState() noexcept
{
    lookaheadDelayBuffer.clear();
    delayWriteIndex = 0;
    detectorSampleCounter = 0;
    leftEngine.reset();
    rightEngine.reset();
    midEngine.reset();
    sideEngine.reset();
}

void QQSuperCompressionAudioProcessor::updateLookaheadConfiguration (bool force)
{
    const auto requestedMs = qqsc::params::snapLookaheadMs (
        apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load());
    const auto requestedSamples = juce::jlimit (0, maxLookaheadSamples,
        static_cast<int> (std::round (currentSampleRate * static_cast<double> (requestedMs) * 0.001)));

    if (! force && requestedSamples == currentLookaheadSamples)
        return;

    currentLookaheadSamples = requestedSamples;
    leftEngine.setLookaheadSamples (requestedSamples);
    rightEngine.setLookaheadSamples (requestedSamples);
    midEngine.setLookaheadSamples (requestedSamples);
    sideEngine.setLookaheadSamples (requestedSamples);

    if (force)
    {
        resetLookaheadState();
    }
    else
    {
        // Keep the already-buffered audio so changing the test value does not
        // deliberately insert a new silence gap. Rebuild each peak queue from
        // the most recent N samples, so the first sample after the change has a
        // complete future-window history rather than a cold detector. A host may
        // still perform its own one-time PDC realignment because latency changed.
        detectorSampleCounter = 0;
        const auto ratioForWarmup = juce::jmax (1.0f, apvts.getRawParameterValue (qqsc::params::ratio)->load());
        const bool stereoBus = getTotalNumInputChannels() >= 2;
        for (int age = requestedSamples; age >= 1; --age)
        {
            int index = delayWriteIndex - age;
            while (index < 0)
                index += delayCapacity;

            const auto l = lookaheadDelayBuffer.getSample (0, index);
            const auto r = lookaheadDelayBuffer.getSample (1, index);
            const auto m = stereoBus ? 0.5f * (l + r) : l;
            const auto side = stereoBus ? 0.5f * (l - r) : 0.0f;

            leftEngine.processSample (l, ratioForWarmup, detectorSampleCounter);
            rightEngine.processSample (r, ratioForWarmup, detectorSampleCounter);
            midEngine.processSample (m, ratioForWarmup, detectorSampleCounter);
            sideEngine.processSample (side, ratioForWarmup, detectorSampleCounter);
            ++detectorSampleCounter;
        }
    }

    // A Lookahead change changes the detector observation window, so discard an
    // old visual GR Hold marker rather than showing a peak from the previous
    // detector setting for another two seconds. This is meter-only.
    resetGainReductionHold();

    // This is real audio-path latency, not a cosmetic setting. Bypass uses the
    // same delay path, so host PDC must always see the same value in both states.
    setLatencySamples (currentLookaheadSamples);
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

    // The selected Lookahead preset controls both analysis length and actual
    // plugin delay. Changing it while audio is running may cause one host/PDC
    // realignment event; the steady-state processing after that uses the new
    // value exactly.
    updateLookaheadConfiguration();

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

    ratioSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::ratio)->load());
    makeupSTSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load());
    makeupLSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load());
    makeupRSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load());
    makeupMSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load());
    makeupSSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load());
    mixSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::mix)->load() * 0.01f);

    float graphInputPeak = 0.0f;
    float graphWetPeak = 0.0f;
    float graphOutputPeak = 0.0f;

    float meterInputPeak[2]  { 0.0f, 0.0f };
    float meterOutputPeak[2] { 0.0f, 0.0f };
    float meterMaxGrDb[2]    { 0.0f, 0.0f };

    for (int i = 0; i < numSamples; ++i)
    {
        // Current (future relative to the delayed output) input sample.
        const float inputL = buffer.getSample (0, i);
        const float inputR = stereoBus ? buffer.getSample (1, i) : 0.0f;
        const float inputM = stereoBus ? 0.5f * (inputL + inputR) : inputL;
        const float inputS = stereoBus ? 0.5f * (inputL - inputR) : 0.0f;

        const auto ratioNow = ratioSmoother.getNextValue();

        // Each analyser sees the complete future window for the sample that is
        // about to leave the delay line. No Attack/Release envelope is applied.
        const auto gainL = leftEngine.processSample  (inputL, ratioNow, detectorSampleCounter);
        const auto gainR = rightEngine.processSample (inputR, ratioNow, detectorSampleCounter);
        const auto gainM = midEngine.processSample   (inputM, ratioNow, detectorSampleCounter);
        const auto gainS = sideEngine.processSample  (inputS, ratioNow, detectorSampleCounter);
        const auto linkedGain = juce::jmin (gainL, gainR);

        // Store current input, then read the sample exactly Lookahead samples old.
        lookaheadDelayBuffer.setSample (0, delayWriteIndex, inputL);
        lookaheadDelayBuffer.setSample (1, delayWriteIndex, inputR);
        int readIndex = delayWriteIndex - currentLookaheadSamples;
        while (readIndex < 0)
            readIndex += delayCapacity;

        const float dryL = lookaheadDelayBuffer.getSample (0, readIndex);
        const float dryR = stereoBus ? lookaheadDelayBuffer.getSample (1, readIndex) : 0.0f;
        const float dryM = stereoBus ? 0.5f * (dryL + dryR) : dryL;
        const float dryS = stereoBus ? 0.5f * (dryL - dryR) : 0.0f;

        if (++delayWriteIndex >= delayCapacity)
            delayWriteIndex = 0;
        ++detectorSampleCounter;

        const float wetIndependentL = dryL * gainL;
        const float wetIndependentR = dryR * gainR;
        const float wetM = dryM * gainM;
        const float wetS = dryS * gainS;
        const float wetLinkedL = dryL * linkedGain;
        const float wetLinkedR = dryR * linkedGain;

        // Strict Integrated LUFS Match source: delayed Dry versus the compressed
        // Wet before Makeup/Mix. The loudness engine applies BS.1770 K-weighting,
        // 400 ms blocks at 75% overlap, the -70 LUFS absolute gate and the
        // relative -10 LU gate. ST is measured as stereo; LR and MS components
        // are measured independently with the same mono BS.1770 maths.
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

        float wetL = dryL;
        float wetR = dryR;
        float processedL = dryL;
        float processedR = dryR;
        float gr0 = 0.0f;
        float gr1 = 0.0f;

        if (mode == qqsc::params::midSide)
        {
            wetL = stereoBus ? wetM + wetS : wetM;
            wetR = stereoBus ? wetM - wetS : 0.0f;

            const float processedM = wetM * makeupM;
            const float processedS = wetS * makeupS;
            processedL = stereoBus ? processedM + processedS : processedM;
            processedR = stereoBus ? processedM - processedS : 0.0f;

            gr0 = midEngine.getCurrentGainReductionDb();
            gr1 = sideEngine.getCurrentGainReductionDb();

            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryM));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryS));
        }
        else if (mode == qqsc::params::leftRight)
        {
            wetL = wetIndependentL;
            wetR = wetIndependentR;
            processedL = wetL * makeupL;
            processedR = wetR * makeupR;

            gr0 = leftEngine.getCurrentGainReductionDb();
            gr1 = rightEngine.getCurrentGainReductionDb();

            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryL));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryR));
        }
        else
        {
            wetL = wetLinkedL;
            wetR = wetLinkedR;
            processedL = wetL * makeupST;
            processedR = wetR * makeupST;

            const float linkedGr = juce::jmax (leftEngine.getCurrentGainReductionDb(),
                                                rightEngine.getCurrentGainReductionDb());
            gr0 = linkedGr;
            gr1 = linkedGr;

            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryL));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryR));
        }

        meterMaxGrDb[0] = juce::jmax (meterMaxGrDb[0], gr0);
        meterMaxGrDb[1] = juce::jmax (meterMaxGrDb[1], stereoBus || mode == qqsc::params::midSide ? gr1 : 0.0f);

        const float mixedL = dryL + (processedL - dryL) * mix;
        const float mixedR = dryR + (processedR - dryR) * mix;

        // Bypass must preserve the same Lookahead delay/PDC as active processing.
        const float outL = forceBypass ? dryL : mixedL;
        const float outR = forceBypass ? dryR : mixedR;

        buffer.setSample (0, i, outL);
        if (stereoBus)
            buffer.setSample (1, i, outR);

        graphInputPeak = juce::jmax (graphInputPeak, stereoBus ? maxAbs (dryL, dryR) : std::abs (dryL));
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
    snapshot.ratio = apvts.getRawParameterValue (qqsc::params::ratio)->load();
    snapshot.makeupST = apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load();
    snapshot.makeupL = apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load();
    snapshot.makeupR = apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load();
    snapshot.makeupM = apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load();
    snapshot.makeupS = apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load();
    snapshot.mix = apvts.getRawParameterValue (qqsc::params::mix)->load();
    snapshot.lookaheadMs = qqsc::params::snapLookaheadMs (apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load());
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
    setActualParameterValue (qqsc::params::ratio, snapshot.ratio);
    setActualParameterValue (qqsc::params::makeupGainDb, snapshot.makeupST);
    setActualParameterValue (qqsc::params::makeupGainLDb, snapshot.makeupL);
    setActualParameterValue (qqsc::params::makeupGainRDb, snapshot.makeupR);
    setActualParameterValue (qqsc::params::makeupGainMDb, snapshot.makeupM);
    setActualParameterValue (qqsc::params::makeupGainSDb, snapshot.makeupS);
    setActualParameterValue (qqsc::params::mix, snapshot.mix);
    const auto lookaheadMs = qqsc::params::snapLookaheadMs (snapshot.lookaheadMs);
    setActualParameterValue (qqsc::params::lookaheadMs, lookaheadMs);
    notifyHostLookaheadLatency (lookaheadMs);
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
        state.setProperty (abProperty (prefix + "ratio"), s.ratio, nullptr);
        state.setProperty (abProperty (prefix + "makeupST"), s.makeupST, nullptr);
        state.setProperty (abProperty (prefix + "makeupL"), s.makeupL, nullptr);
        state.setProperty (abProperty (prefix + "makeupR"), s.makeupR, nullptr);
        state.setProperty (abProperty (prefix + "makeupM"), s.makeupM, nullptr);
        state.setProperty (abProperty (prefix + "makeupS"), s.makeupS, nullptr);
        state.setProperty (abProperty (prefix + "mix"), s.mix, nullptr);
        state.setProperty (abProperty (prefix + "lookaheadMs"), s.lookaheadMs, nullptr);
        state.setProperty (abProperty (prefix + "mode"), s.mode, nullptr);
    };

    write ("A_", snapshotA);
    write ("B_", snapshotB);
}

void QQSuperCompressionAudioProcessor::readABStateFrom (const juce::ValueTree& state)
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
            s.ratio = static_cast<float> (state.getProperty (abProperty (prefix + "ratio"), s.ratio));
            s.makeupST = static_cast<float> (state.getProperty (abProperty (prefix + "makeupST"), s.makeupST));
            s.makeupL = static_cast<float> (state.getProperty (abProperty (prefix + "makeupL"), s.makeupL));
            s.makeupR = static_cast<float> (state.getProperty (abProperty (prefix + "makeupR"), s.makeupR));
            s.makeupM = static_cast<float> (state.getProperty (abProperty (prefix + "makeupM"), s.makeupM));
            s.makeupS = static_cast<float> (state.getProperty (abProperty (prefix + "makeupS"), s.makeupS));
            s.mix = static_cast<float> (state.getProperty (abProperty (prefix + "mix"), s.mix));
            s.lookaheadMs = qqsc::params::snapLookaheadMs (
                static_cast<float> (state.getProperty (abProperty (prefix + "lookaheadMs"), s.lookaheadMs)));
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
            apvts.replaceState (state);

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

            readABStateFrom (state);
            notifyHostLookaheadLatency (snappedLookahead);
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
