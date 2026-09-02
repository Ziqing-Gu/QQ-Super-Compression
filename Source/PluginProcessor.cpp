#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
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
constexpr auto monitorLRProperty = "qqscMonitorLRSelection";
constexpr auto monitorMSProperty = "qqscMonitorMSSelection";
constexpr int oversamplingSchemaVersion = 2; // v0.1.10: 0 ms-only 1x/8x/16x Oversampling schema
constexpr int currentStateSchemaVersion = 10; // v1.1.1: appended detector-only Side Chain HPF
constexpr float centeredChannelMonitorGain = 0.70710678118654752440f; // 1/sqrt(2), -3.0103 dB

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

uint64_t packDisplayStereoSample (float left, float right) noexcept
{
    uint32_t leftBits = 0;
    uint32_t rightBits = 0;
    std::memcpy (&leftBits, &left, sizeof (left));
    std::memcpy (&rightBits, &right, sizeof (right));
    return static_cast<uint64_t> (leftBits) | (static_cast<uint64_t> (rightBits) << 32u);
}

void unpackDisplayStereoSample (uint64_t packed, float& left, float& right) noexcept
{
    const auto leftBits = static_cast<uint32_t> (packed & 0xffffffffu);
    const auto rightBits = static_cast<uint32_t> (packed >> 32u);
    std::memcpy (&left, &leftBits, sizeof (left));
    std::memcpy (&right, &rightBits, sizeof (right));
}
}

struct QQSuperCompressionAudioProcessor::DisplayKeyHistoryStorage
{
    DisplayKeyHistoryStorage (double hostRateIn, uint64_t generationIn)
        : generation (generationIn),
          hostSampleRate (juce::jmax (1.0, hostRateIn)),
          analysisSampleRate (juce::jmin (48000.0, hostSampleRate)),
          capacity (static_cast<uint64_t> (std::ceil (analysisSampleRate * 10.0))),
          samples (std::make_unique<std::atomic<uint64_t>[]> (static_cast<size_t> (capacity)))
    {
        for (uint64_t i = 0; i < capacity; ++i)
            samples[static_cast<size_t> (i)].store (0, std::memory_order_relaxed);
    }

    void push (float left, float right, int source, bool stereo) noexcept
    {
        if (source != audioThreadKeySource || stereo != audioThreadStereoKey)
        {
            audioThreadKeySource = source;
            audioThreadStereoKey = stereo;
            sourceStartCounter.store (writeCounterLocal, std::memory_order_release);
            keySource.store (source, std::memory_order_release);
            stereoKey.store (stereo, std::memory_order_release);
            phase = 0.0;
            sumLeft = 0.0;
            sumRight = 0.0;
            sumCount = 0;
        }

        sumLeft += static_cast<double> (left);
        sumRight += static_cast<double> (right);
        ++sumCount;
        phase += analysisSampleRate;

        if (phase + 1.0e-9 < hostSampleRate)
            return;

        phase -= hostSampleRate;
        const auto scale = 1.0 / static_cast<double> (juce::jmax (1, sumCount));
        const auto displayLeft = static_cast<float> (sumLeft * scale);
        const auto displayRight = static_cast<float> (sumRight * scale);
        const auto slot = static_cast<size_t> (writeCounterLocal % capacity);
        samples[slot].store (packDisplayStereoSample (displayLeft, displayRight), std::memory_order_relaxed);
        ++writeCounterLocal;
        writeCounter.store (writeCounterLocal, std::memory_order_release);
        sumLeft = 0.0;
        sumRight = 0.0;
        sumCount = 0;
    }

    const uint64_t generation;
    const double hostSampleRate;
    const double analysisSampleRate;
    const uint64_t capacity;
    std::unique_ptr<std::atomic<uint64_t>[]> samples;
    std::atomic<uint64_t> writeCounter { 0 };
    std::atomic<uint64_t> sourceStartCounter { 0 };
    std::atomic<int> keySource { qqsc::params::keyInternal };
    std::atomic<bool> stereoKey { false };
    uint64_t writeCounterLocal = 0;
    int audioThreadKeySource = -1;
    bool audioThreadStereoKey = false;
    double phase = 0.0;
    double sumLeft = 0.0;
    double sumRight = 0.0;
    int sumCount = 0;
};

QQSuperCompressionAudioProcessor::QQSuperCompressionAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                               .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                               .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
                               .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)),
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

std::shared_ptr<QQSuperCompressionAudioProcessor::DisplayKeyHistoryStorage>
QQSuperCompressionAudioProcessor::createDisplayKeyHistoryStorage()
{
    const auto generation = displayKeyHistoryGenerationCounter.fetch_add (1, std::memory_order_relaxed) + 1;
    const auto hostRate = displayKeyHistoryHostSampleRate.load (std::memory_order_relaxed);
    return std::make_shared<DisplayKeyHistoryStorage> (hostRate, generation);
}

void QQSuperCompressionAudioProcessor::setDisplayKeyHistoryCaptureEnabled (bool enabled)
{
    const auto wasEnabled = displayKeyHistoryCaptureEnabled.exchange (enabled, std::memory_order_acq_rel);

    if (enabled)
    {
        if (! wasEnabled || std::atomic_load_explicit (&displayKeyHistoryStorage, std::memory_order_acquire) == nullptr)
            std::atomic_store_explicit (&displayKeyHistoryStorage, createDisplayKeyHistoryStorage(),
                                        std::memory_order_release);
    }
    else
    {
        std::shared_ptr<DisplayKeyHistoryStorage> empty;
        std::atomic_store_explicit (&displayKeyHistoryStorage, std::move (empty),
                                    std::memory_order_release);
    }
}

QQSuperCompressionAudioProcessor::DisplayKeyHistoryPosition
QQSuperCompressionAudioProcessor::getDisplayKeyHistoryPosition() const noexcept
{
    const auto storage = std::atomic_load_explicit (&displayKeyHistoryStorage, std::memory_order_acquire);
    if (storage == nullptr)
        return {};

    return { storage->generation, storage->writeCounter.load (std::memory_order_acquire) };
}

bool QQSuperCompressionAudioProcessor::copyDisplayKeyHistory (
    uint64_t generation, uint64_t requestedStartCounter, uint64_t requestedEndCounter,
    DisplayKeyHistorySnapshot& destination) const
{
    const auto storage = std::atomic_load_explicit (&displayKeyHistoryStorage, std::memory_order_acquire);
    if (storage == nullptr || storage->generation != generation)
        return false;

    const auto publishedEnd = storage->writeCounter.load (std::memory_order_acquire);
    const auto endCounter = juce::jmin (requestedEndCounter, publishedEnd);
    const auto sourceStart = storage->sourceStartCounter.load (std::memory_order_acquire);
    const auto ringStart = endCounter > storage->capacity ? endCounter - storage->capacity : 0;
    const auto requestedStart = juce::jmin (requestedStartCounter, endCounter);
    const auto firstCounter = juce::jmax (requestedStart, juce::jmax (sourceStart, ringStart));
    if (endCounter <= firstCounter)
        return false;

    const auto count = endCounter - firstCounter;
    destination = {};
    destination.generation = generation;
    destination.firstCounter = firstCounter;
    destination.sampleRate = storage->analysisSampleRate;
    destination.keySource = storage->keySource.load (std::memory_order_acquire);
    destination.stereoKey = storage->stereoKey.load (std::memory_order_acquire);
    destination.left.resize (static_cast<size_t> (count));
    destination.right.resize (static_cast<size_t> (count));

    for (uint64_t i = 0; i < count; ++i)
    {
        const auto counter = firstCounter + i;
        const auto packed = storage->samples[static_cast<size_t> (counter % storage->capacity)]
                                .load (std::memory_order_relaxed);
        unpackDisplayStereoSample (packed,
                                   destination.left[static_cast<size_t> (i)],
                                   destination.right[static_cast<size_t> (i)]);
    }

    // Atomic slots avoid data races. This final bound check additionally
    // rejects a snapshot if an extremely slow worker was overtaken by a full
    // ten seconds of new audio while it was copying the ring.
    const auto endAfterCopy = storage->writeCounter.load (std::memory_order_acquire);
    const auto startAfterCopy = storage->sourceStartCounter.load (std::memory_order_acquire);
    return endAfterCopy - firstCounter <= storage->capacity && startAfterCopy == sourceStart;
}

juce::AudioProcessorValueTreeState::ParameterLayout QQSuperCompressionAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addRatio = [&] (const char* id, const juce::String& name)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { 1.0f, 32.0f, 0.01f, 0.55f }, 8.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
            {
                return juce::String (v, v < 10.0f ? 2 : 1) + ":1";
            })));
    };

    // Keep the legacy Ratio ID as ST. v1.0.0 appends independent LR/MS Ratio
    // parameters later so older projects keep their established parameter ID.
    addRatio (qqsc::params::ratio, "Ratio ST");

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

    // v0.9.7 Threshold Rebuild: append only one new sound parameter after the
    // established v0.9.4 order. OFF is the -inf sentinel and must be exactly
    // sonically identical to v0.9.4. 0.01 dB resolution allows Shift fine drag.
    auto addThreshold = [&] (const char* id, const juce::String& name)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { qqsc::params::thresholdOffDb, 0.0f, 0.01f }, qqsc::params::thresholdOffDb,
            juce::AudioParameterFloatAttributes()
                .withLabel ("dB")
                .withStringFromValueFunction ([] (float v, int)
                {
                    return qqsc::params::isThresholdEnabled (v) ? juce::String (v, 2) + " dB" : juce::String ("OFF");
                })
                .withValueFromStringFunction ([] (const juce::String& text)
                {
                    return (text.containsIgnoreCase ("off") || text.containsIgnoreCase ("-inf")) ? qqsc::params::thresholdOffDb
                                                            : juce::jlimit (qqsc::params::thresholdOffDb, 0.0f,
                                                                           static_cast<float> (text.getDoubleValue()));
                })));
    };

    // Legacy Threshold ID is ST. The four independent domain parameters are
    // appended in v1.0.0 and migrate from the legacy value when absent.
    addThreshold (qqsc::params::thresholdDb, "Threshold ST");
    addRatio (qqsc::params::ratioL, "Ratio L");
    addRatio (qqsc::params::ratioR, "Ratio R");
    addRatio (qqsc::params::ratioM, "Ratio M");
    addRatio (qqsc::params::ratioS, "Ratio S");
    addThreshold (qqsc::params::thresholdLDb, "Threshold L");
    addThreshold (qqsc::params::thresholdRDb, "Threshold R");
    addThreshold (qqsc::params::thresholdMDb, "Threshold M");
    addThreshold (qqsc::params::thresholdSDb, "Threshold S");

    // Link is workflow state, not a sound parameter. It is saved in the project
    // but deliberately excluded from A/B snapshots. Default ON preserves the
    // expected paired editing behaviour without forcing the values equal.
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { qqsc::params::domainLink, 1 }, "Domain Link", true));

    // v1.0.1 revision: keep legacy Mix as ST, then append independent LR/MS
    // Mix parameters. Older states migrate all four from the legacy shared Mix.
    auto addMix = [&] (const char* id, const juce::String& name)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f }, 100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));
    };
    addMix (qqsc::params::mixL, "Mix L");
    addMix (qqsc::params::mixR, "Mix R");
    addMix (qqsc::params::mixM, "Mix M");
    addMix (qqsc::params::mixS, "Mix S");

    // v1.1.0 Candidate External Key parameters are appended after the complete
    // v1.0.4 sequence. INT + 0 dB are explicit legacy-safe defaults.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { qqsc::params::keySource, 1 }, "Key Source",
        qqsc::params::keySourceChoices(), qqsc::params::keyInternal));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qqsc::params::keyGainDb, 1 }, "Key Gain",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.01f }, 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // v1.1.1 appends a detector-only HPF after the complete v1.1.0 parameter
    // sequence. OFF is exact legacy behaviour; active values use a log 20-500 Hz law.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { qqsc::params::keyHpfHz, 1 }, "Side Chain HPF",
        qqsc::params::keyHpfRange(), qqsc::params::keyHpfOffHz,
        juce::AudioParameterFloatAttributes()
            .withLabel ("Hz")
            .withStringFromValueFunction ([] (float value, int)
            {
                return qqsc::params::isKeyHpfEnabled (value)
                    ? juce::String (std::round (value), 0) + " Hz"
                    : juce::String ("OFF");
            })
            .withValueFromStringFunction ([] (const juce::String& text)
            {
                if (text.containsIgnoreCase ("off"))
                    return qqsc::params::keyHpfOffHz;

                const auto value = static_cast<float> (text.getDoubleValue());
                return qqsc::params::isKeyHpfEnabled (value)
                    ? qqsc::params::clampKeyHpfHz (value)
                    : qqsc::params::keyHpfOffHz;
            })));

    return layout;
}

void QQSuperCompressionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = juce::jmax (1.0, sampleRate);

    displayKeyHistoryHostSampleRate.store (currentSampleRate, std::memory_order_relaxed);
    if (displayKeyHistoryCaptureEnabled.load (std::memory_order_acquire))
        std::atomic_store_explicit (&displayKeyHistoryStorage, createDisplayKeyHistoryStorage(),
                                    std::memory_order_release);

    configuredMaximumBlockSize = juce::jmax (16384, juce::jmax (1, samplesPerBlock));

    for (auto& oversampler : oversamplers)
    {
        oversampler->initProcessing (static_cast<size_t> (configuredMaximumBlockSize));
        oversampler->reset();
    }

    wetBaseBuffer.setSize (6, configuredMaximumBlockSize, false, true, true);
    wetBaseBuffer.clear();
    oversamplingInputBuffer.setSize (6, configuredMaximumBlockSize, false, true, true);
    oversamplingInputBuffer.clear();
    keyInputBuffer.setSize (2, configuredMaximumBlockSize, false, true, true);
    keyInputBuffer.clear();
    originalInputBuffer.setSize (2, configuredMaximumBlockSize, false, true, true);
    originalInputBuffer.clear();

    // v1.0.1 restores the v0.9.4/v0.9.7 future-window peak core. Non-zero
    // Lookahead always runs at 1x; only 0 ms may use 8x/16x, so the detector
    // never needs an oversampled non-zero lookahead queue.
    maxLookaheadSamplesBase = juce::jmax (0, static_cast<int> (std::ceil (currentSampleRate * 0.100)));
    maxLookaheadSamplesInternal = maxLookaheadSamplesBase;
    oversampledDelayCapacity = juce::jmax (2, maxLookaheadSamplesInternal + 2);
    oversampledLookaheadDelayBuffer.setSize (2, oversampledDelayCapacity, false, true, true);
    oversampledLookaheadDelayBuffer.clear();
    oversampledKeyHistoryBuffer.setSize (4, oversampledDelayCapacity, false, true, true);
    oversampledKeyHistoryBuffer.clear();

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
    keyListenDelayBuffer.setSize (2, dryDelayCapacity, false, true, true);
    keyListenDelayBuffer.clear();

    // Input/Makeup/Mix/Output remain host-rate smoothers. All five Ratio
    // smoothers are re-timed to the effective internal rate in
    // updateProcessingConfiguration() whenever 0 ms Oversampling changes.
    inputGainSmoother.reset (currentSampleRate, 0.010);
    keyGainSmoother.reset (currentSampleRate, 0.010);
    keyHpfCutoffSmoother.reset (currentSampleRate, 0.020);
    keyHpfWetSmoother.reset (currentSampleRate, 0.010);
    ratioSmoother.reset (currentSampleRate, 0.010);
    ratioLSmoother.reset (currentSampleRate, 0.010);
    ratioRSmoother.reset (currentSampleRate, 0.010);
    ratioMSmoother.reset (currentSampleRate, 0.010);
    ratioSSmoother.reset (currentSampleRate, 0.010);
    makeupSTSmoother.reset (currentSampleRate, 0.010);
    makeupLSmoother.reset (currentSampleRate, 0.010);
    makeupRSmoother.reset (currentSampleRate, 0.010);
    makeupMSmoother.reset (currentSampleRate, 0.010);
    makeupSSmoother.reset (currentSampleRate, 0.010);
    mixSmoother.reset (currentSampleRate, 0.010);
    mixLSmoother.reset (currentSampleRate, 0.010);
    mixRSmoother.reset (currentSampleRate, 0.010);
    mixMSmoother.reset (currentSampleRate, 0.010);
    mixSSmoother.reset (currentSampleRate, 0.010);
    outputGainSmoother.reset (currentSampleRate, 0.010);

    inputGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::inputGainDb)->load()));
    keyGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::keyGainDb)->load()));
    const auto initialKeyHpfHz = apvts.getRawParameterValue (qqsc::params::keyHpfHz)->load();
    keyHpfCutoffSmoother.setCurrentAndTargetValue (
        qqsc::params::isKeyHpfEnabled (initialKeyHpfHz)
            ? qqsc::params::clampKeyHpfHz (initialKeyHpfHz)
            : qqsc::params::keyHpfMinHz);
    keyHpfWetSmoother.setCurrentAndTargetValue (
        qqsc::params::isKeyHpfEnabled (initialKeyHpfHz) ? 1.0f : 0.0f);
    updateKeyHighPassCoefficients (keyHpfCutoffSmoother.getCurrentValue());
    resetKeyHighPassState();
    ratioSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::ratio)->load());
    ratioLSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::ratioL)->load());
    ratioRSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::ratioR)->load());
    ratioMSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::ratioM)->load());
    ratioSSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::ratioS)->load());
    makeupSTSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load());
    makeupLSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load());
    makeupRSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load());
    makeupMSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load());
    makeupSSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load());
    mixSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::mix)->load() * 0.01f);
    mixLSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::mixL)->load() * 0.01f);
    mixRSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::mixR)->load() * 0.01f);
    mixMSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::mixM)->load() * 0.01f);
    mixSSmoother.setCurrentAndTargetValue (apvts.getRawParameterValue (qqsc::params::mixS)->load() * 0.01f);
    outputGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::outputGainDb)->load()));

    gainReductionHoldDurationSamples = juce::jmax<int64_t> (1, static_cast<int64_t> (std::llround (currentSampleRate * 2.0)));
    resetGainReductionHold();

    currentLookaheadSamplesBase = -1;
    currentLookaheadSamplesInternal = -1;
    currentOversamplingIndex = -1;
    currentKeySource = -1;
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

void QQSuperCompressionAudioProcessor::resetDetectorCoreState() noexcept
{
    detectorSampleCounter = 0;
    leftEngine.reset();
    rightEngine.reset();
    midEngine.reset();
    sideEngine.reset();
}

void QQSuperCompressionAudioProcessor::resetOversampledCoreState() noexcept
{
    oversampledLookaheadDelayBuffer.clear();
    oversampledKeyHistoryBuffer.clear();
    oversampledDelayWriteIndex = 0;
    resetDetectorCoreState();
}

void QQSuperCompressionAudioProcessor::resetDryDelayState() noexcept
{
    dryDelayBuffer.clear();
    originalDryDelayBuffer.clear();
    keyListenDelayBuffer.clear();
    dryDelayWriteIndex = 0;
}

void QQSuperCompressionAudioProcessor::resetKeyHighPassState() noexcept
{
    for (auto& state : keyHighPassStates)
        state.reset();

    keyHpfCoefficientCountdown = 0;
}

void QQSuperCompressionAudioProcessor::updateKeyHighPassCoefficients (float cutoffHz) noexcept
{
    const auto safeMaximum = juce::jmax (0.1, currentSampleRate * 0.45);
    const auto cutoff = juce::jlimit (0.1, safeMaximum,
                                      static_cast<double> (qqsc::params::clampKeyHpfHz (cutoffHz)));
    const auto omega = 2.0 * juce::MathConstants<double>::pi * cutoff / currentSampleRate;
    const auto sine = std::sin (omega);
    const auto cosine = std::cos (omega);
    constexpr double butterworthQ = 0.70710678118654752440;
    const auto alpha = sine / (2.0 * butterworthQ);
    const auto a0 = 1.0 + alpha;

    keyHighPassCoefficients.b0 = static_cast<float> (((1.0 + cosine) * 0.5) / a0);
    keyHighPassCoefficients.b1 = static_cast<float> (-(1.0 + cosine) / a0);
    keyHighPassCoefficients.b2 = keyHighPassCoefficients.b0;
    keyHighPassCoefficients.a1 = static_cast<float> ((-2.0 * cosine) / a0);
    keyHighPassCoefficients.a2 = static_cast<float> ((1.0 - alpha) / a0);
}

void QQSuperCompressionAudioProcessor::resetAllProcessingState() noexcept
{
    resetOversampledCoreState();
    resetDryDelayState();
    resetKeyHighPassState();
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
    const auto requestedKeySource = juce::jlimit (static_cast<int> (qqsc::params::keyInternal),
                                                  static_cast<int> (qqsc::params::keyExternal),
                                                  juce::roundToInt (apvts.getRawParameterValue (qqsc::params::keySource)->load()));
    const auto requestedFactor = qqsc::params::oversamplingFactorForChoiceIndex (requestedOversamplingIndex);
    const auto requestedLookaheadInternal = requestedLookaheadBase * requestedFactor;
    const auto requestedOversamplingLatency = getOversamplingLatencySamples (requestedOversamplingIndex);
    const auto requestedTotalLatency = requestedLookaheadBase + requestedOversamplingLatency;

    const bool oversamplingChanged = requestedOversamplingIndex != currentOversamplingIndex;
    const bool lookaheadChanged = requestedLookaheadBase != currentLookaheadSamplesBase;
    const bool keySourceChanged = requestedKeySource != currentKeySource;

    if (! force && ! oversamplingChanged && ! lookaheadChanged && ! keySourceChanged)
        return;

    const auto ratioCurrent = ratioSmoother.getCurrentValue();
    const auto ratioLCurrent = ratioLSmoother.getCurrentValue();
    const auto ratioRCurrent = ratioRSmoother.getCurrentValue();
    const auto ratioMCurrent = ratioMSmoother.getCurrentValue();
    const auto ratioSCurrent = ratioSSmoother.getCurrentValue();
    const auto ratioTarget = juce::jmax (1.0f, apvts.getRawParameterValue (qqsc::params::ratio)->load());
    const auto ratioLTarget = juce::jmax (1.0f, apvts.getRawParameterValue (qqsc::params::ratioL)->load());
    const auto ratioRTarget = juce::jmax (1.0f, apvts.getRawParameterValue (qqsc::params::ratioR)->load());
    const auto ratioMTarget = juce::jmax (1.0f, apvts.getRawParameterValue (qqsc::params::ratioM)->load());
    const auto ratioSTarget = juce::jmax (1.0f, apvts.getRawParameterValue (qqsc::params::ratioS)->load());

    currentOversamplingIndex = requestedOversamplingIndex;
    currentKeySource = requestedKeySource;
    currentOversamplingFactor = requestedFactor;
    currentLookaheadSamplesBase = requestedLookaheadBase;
    currentLookaheadSamplesInternal = requestedLookaheadInternal;
    currentTotalLatencySamples = requestedTotalLatency;

    leftEngine.setLookaheadSamples (requestedLookaheadInternal);
    rightEngine.setLookaheadSamples (requestedLookaheadInternal);
    midEngine.setLookaheadSamples (requestedLookaheadInternal);
    sideEngine.setLookaheadSamples (requestedLookaheadInternal);

    const auto resetRatioSmoother = [this, force] (auto& smoother, float current, float target)
    {
        smoother.reset (currentSampleRate * currentOversamplingFactor, 0.010);
        if (force)
            smoother.setCurrentAndTargetValue (target);
        else
        {
            smoother.setCurrentAndTargetValue (current);
            smoother.setTargetValue (target);
        }
    };

    resetRatioSmoother (ratioSmoother,  ratioCurrent,  ratioTarget);
    resetRatioSmoother (ratioLSmoother, ratioLCurrent, ratioLTarget);
    resetRatioSmoother (ratioRSmoother, ratioRCurrent, ratioRTarget);
    resetRatioSmoother (ratioMSmoother, ratioMCurrent, ratioMTarget);
    resetRatioSmoother (ratioSSmoother, ratioSCurrent, ratioSTarget);

    if (force || oversamplingChanged)
    {
        // A sample-rate-domain change invalidates Oversampling FIR history and
        // the detector queues. Clear Dry too so Wet/Dry/Bypass restart aligned.
        resetAllProcessingState();
    }
    else if (keySourceChanged)
    {
        // Never mix INT and EXT detector history. The carrier/dry delays remain
        // aligned, but the new source starts a clean future-window queue.
        oversampledKeyHistoryBuffer.clear();
        resetDetectorCoreState();
        resetKeyHighPassState();
    }
    else if (lookaheadChanged)
    {
        // Same internal rate/source, different future-window length: rebuild all
        // four queues from the exact stored detector-source domains.
        resetDetectorCoreState();
        const auto thresholdL = qqsc::params::thresholdLinear (
            apvts.getRawParameterValue (qqsc::params::thresholdLDb)->load());
        const auto thresholdR = qqsc::params::thresholdLinear (
            apvts.getRawParameterValue (qqsc::params::thresholdRDb)->load());
        const auto thresholdM = qqsc::params::thresholdLinear (
            apvts.getRawParameterValue (qqsc::params::thresholdMDb)->load());
        const auto thresholdS = qqsc::params::thresholdLinear (
            apvts.getRawParameterValue (qqsc::params::thresholdSDb)->load());

        for (int age = requestedLookaheadInternal; age >= 1; --age)
        {
            int index = oversampledDelayWriteIndex - age;
            while (index < 0)
                index += oversampledDelayCapacity;

            const auto keyL = oversampledKeyHistoryBuffer.getSample (0, index);
            const auto keyR = oversampledKeyHistoryBuffer.getSample (1, index);
            const auto keyM = oversampledKeyHistoryBuffer.getSample (2, index);
            const auto keyS = oversampledKeyHistoryBuffer.getSample (3, index);

            leftEngine.processSample  (keyL, ratioLTarget, thresholdL, detectorSampleCounter);
            rightEngine.processSample (keyR, ratioRTarget, thresholdR, detectorSampleCounter);
            midEngine.processSample   (keyM, ratioMTarget, thresholdM, detectorSampleCounter);
            sideEngine.processSample  (keyS, ratioSTarget, thresholdS, detectorSampleCounter);
            ++detectorSampleCounter;
        }
    }

    resetGainReductionHold();
    setLatencySamples (currentTotalLatencySamples);
}

bool QQSuperCompressionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;

    const auto& keyIn = layouts.getChannelSet (true, 1);
    return keyIn.isDisabled()
        || keyIn == juce::AudioChannelSet::mono()
        || keyIn == juce::AudioChannelSet::stereo();
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

    const int numInputChannels = getMainBusNumInputChannels();
    const int numOutputChannels = getMainBusNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    auto externalKeyBuffer = getBusBuffer (buffer, true, 1);
    const int externalKeyChannels = externalKeyBuffer.getNumChannels();
    const bool externalKeyBusAvailable = externalKeyChannels > 0;
    meterState.externalKeyBusAvailable.store (externalKeyBusAvailable, std::memory_order_relaxed);

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

    // Threshold remains only a lower boundary around the transparent QQ law.
    // ST uses the legacy parameter; LR and MS use independent pairs. None of
    // these values changes the future-window detector definition.
    const auto thresholdSTLinear = qqsc::params::thresholdLinear (
        apvts.getRawParameterValue (qqsc::params::thresholdDb)->load());
    const auto thresholdLLinear = qqsc::params::thresholdLinear (
        apvts.getRawParameterValue (qqsc::params::thresholdLDb)->load());
    const auto thresholdRLinear = qqsc::params::thresholdLinear (
        apvts.getRawParameterValue (qqsc::params::thresholdRDb)->load());
    const auto thresholdMLinear = qqsc::params::thresholdLinear (
        apvts.getRawParameterValue (qqsc::params::thresholdMDb)->load());
    const auto thresholdSLinear = qqsc::params::thresholdLinear (
        apvts.getRawParameterValue (qqsc::params::thresholdSDb)->load());

    if (mode != gainReductionHoldMode)
        resetGainReductionHold (mode);

    inputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::inputGainDb)->load()));
    ratioSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::ratio)->load());
    ratioLSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::ratioL)->load());
    ratioRSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::ratioR)->load());
    ratioMSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::ratioM)->load());
    ratioSSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::ratioS)->load());
    makeupSTSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load());
    makeupLSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load());
    makeupRSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load());
    makeupMSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load());
    makeupSSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load());
    mixSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::mix)->load() * 0.01f);
    mixLSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::mixL)->load() * 0.01f);
    mixRSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::mixR)->load() * 0.01f);
    mixMSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::mixM)->load() * 0.01f);
    mixSSmoother.setTargetValue (apvts.getRawParameterValue (qqsc::params::mixS)->load() * 0.01f);
    outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::outputGainDb)->load()));
    keyGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (
        apvts.getRawParameterValue (qqsc::params::keyGainDb)->load()));
    const auto keyHpfHz = apvts.getRawParameterValue (qqsc::params::keyHpfHz)->load();
    const auto keyHpfEnabled = qqsc::params::isKeyHpfEnabled (keyHpfHz);
    keyHpfCutoffSmoother.setTargetValue (
        keyHpfEnabled ? qqsc::params::clampKeyHpfHz (keyHpfHz) : qqsc::params::keyHpfMinHz);
    keyHpfWetSmoother.setTargetValue (keyHpfEnabled ? 1.0f : 0.0f);

    const auto keySource = juce::jlimit (static_cast<int> (qqsc::params::keyInternal),
                                         static_cast<int> (qqsc::params::keyExternal),
                                         juce::roundToInt (apvts.getRawParameterValue (qqsc::params::keySource)->load()));
    const bool useExternalKey = keySource == qqsc::params::keyExternal;

    const bool detectorKeyIsStereo = useExternalKey ? externalKeyChannels >= 2 : stereoBus;
    const auto displayHistoryForBlock = std::atomic_load_explicit (
        &displayKeyHistoryStorage, std::memory_order_acquire);

    float meterMaxGrDb[2] { 0.0f, 0.0f };
    float displayDetectorPeak[2] { 0.0f, 0.0f };
    float keyInputPeak = 0.0f;

    // v1.0.4 INT is preserved exactly: the detector follows the post-Input-Gain
    // main signal. EXT replaces only that detector source and applies its own
    // smoothed Key Gain; it never enters the audible carrier path.
    for (int i = 0; i < numSamples; ++i)
    {
        const float originalL = buffer.getSample (0, i);
        const float originalR = stereoBus ? buffer.getSample (1, i) : 0.0f;
        originalInputBuffer.setSample (0, i, originalL);
        originalInputBuffer.setSample (1, i, originalR);

        const auto inputGain = inputGainSmoother.getNextValue();
        const float mainL = originalL * inputGain;
        const float mainR = stereoBus ? originalR * inputGain : 0.0f;
        buffer.setSample (0, i, mainL);
        if (stereoBus)
            buffer.setSample (1, i, mainR);

        const auto keyGain = keyGainSmoother.getNextValue();
        float rawDisplayKeyL = originalL;
        float rawDisplayKeyR = originalR;
        float selectedKeyL = mainL;
        float selectedKeyR = mainR;

        if (useExternalKey)
        {
            const float externalL = externalKeyBusAvailable ? externalKeyBuffer.getSample (0, i) : 0.0f;
            const float externalR = externalKeyChannels >= 2 ? externalKeyBuffer.getSample (1, i) : externalL;
            rawDisplayKeyL = externalL;
            rawDisplayKeyR = externalR;
            selectedKeyL = externalL * keyGain;
            selectedKeyR = externalR * keyGain;
        }

        if (displayHistoryForBlock != nullptr)
            displayHistoryForBlock->push (rawDisplayKeyL, rawDisplayKeyR,
                                          keySource, detectorKeyIsStereo);

        const auto keyHpfCutoff = keyHpfCutoffSmoother.getNextValue();
        const auto keyHpfWet = keyHpfWetSmoother.getNextValue();
        if (keyHpfCoefficientCountdown-- <= 0)
        {
            updateKeyHighPassCoefficients (keyHpfCutoff);
            keyHpfCoefficientCountdown = 16;
        }

        const auto filteredKeyL = keyHighPassStates[0].process (selectedKeyL, keyHighPassCoefficients);
        const auto filteredKeyR = keyHighPassStates[1].process (selectedKeyR, keyHighPassCoefficients);
        selectedKeyL += (filteredKeyL - selectedKeyL) * keyHpfWet;
        selectedKeyR += (filteredKeyR - selectedKeyR) * keyHpfWet;

        keyInputBuffer.setSample (0, i, selectedKeyL);
        keyInputBuffer.setSample (1, i, selectedKeyR);
        keyInputPeak = juce::jmax (keyInputPeak, maxAbs (selectedKeyL, selectedKeyR));

        oversamplingInputBuffer.setSample (0, i, mainL);
        oversamplingInputBuffer.setSample (1, i, mainR);
        oversamplingInputBuffer.setSample (2, i, selectedKeyL);
        oversamplingInputBuffer.setSample (3, i, selectedKeyR);
        oversamplingInputBuffer.setSample (4, i, 0.0f);
        oversamplingInputBuffer.setSample (5, i, 0.0f);
    }

    // Main and selected Key enter the same effective 1x/8x/16x internal domain.
    // Channels 2/3 carry the Key only until the detector consumes them; all six
    // channels are then overwritten with the established wet variants.
    const juce::dsp::AudioBlock<const float> fullHostInputBlock (oversamplingInputBuffer);
    const auto hostInputBlock = fullHostInputBlock.getSubBlock (0, static_cast<size_t> (numSamples));
    auto& oversampler = getCurrentOversampler();
    auto oversampledBlock = oversampler.processSamplesUp (hostInputBlock);

    const auto internalNumSamples = static_cast<int> (oversampledBlock.getNumSamples());
    const auto expectedInternalSamples = numSamples * currentOversamplingFactor;
    jassert (internalNumSamples == expectedInternalSamples);
    jassert (oversampledBlock.getNumChannels() >= 6u);
    if (internalNumSamples != expectedInternalSamples || oversampledBlock.getNumChannels() < 6u)
    {
        buffer.clear();
        return;
    }

    for (int i = 0; i < internalNumSamples; ++i)
    {
        const float inputL = oversampledBlock.getSample (0, i);
        const float inputR = stereoBus ? oversampledBlock.getSample (1, i) : 0.0f;

        // Selected Key is already post-Key-Gain/post-HPF in channels 2/3. With
        // HPF OFF these samples are the exact v1.1.0 INT/EXT detector signal.
        const float keyL = oversampledBlock.getSample (2, i);
        const float keyR = oversampledBlock.getSample (3, i);
        const bool stereoKey = detectorKeyIsStereo;
        // A mono external key is deliberately common to every independent
        // detector domain. This lets one kick drive L/R or M/S together instead
        // of leaving the Side detector untriggered merely because the key is mono.
        const float keyM = stereoKey ? 0.5f * (keyL + keyR) : keyL;
        const float keyS = stereoKey ? 0.5f * (keyL - keyR)
                                     : (useExternalKey ? keyL : 0.0f);

        const auto ratioSTNow = ratioSmoother.getNextValue();
        const auto ratioLNow = ratioLSmoother.getNextValue();
        const auto ratioRNow = ratioRSmoother.getNextValue();
        const auto ratioMNow = ratioMSmoother.getNextValue();
        const auto ratioSNow = ratioSSmoother.getNextValue();

        // LR/MS have independent Ratio+Threshold values. The detector itself is
        // still the same future-window peak engine in every domain.
        const auto gainL = leftEngine.processSample  (keyL, ratioLNow, thresholdLLinear, detectorSampleCounter);
        const auto gainR = rightEngine.processSample (keyR, ratioRNow, thresholdRLinear, detectorSampleCounter);
        const auto gainM = midEngine.processSample   (keyM, ratioMNow, thresholdMLinear, detectorSampleCounter);
        const auto gainS = sideEngine.processSample  (keyS, ratioSNow, thresholdSLinear, detectorSampleCounter);

        // ST has its own Ratio/Threshold without needing duplicate detector
        // queues: both L/R engines expose the exact current window peak. Stereo
        // linking uses the stronger window level, then applies one common gain.
        const auto linkedLevel = stereoBus ? juce::jmax (leftEngine.getCurrentLevel(), rightEngine.getCurrentLevel())
                                            : leftEngine.getCurrentLevel();
        const auto linkedGain = qqsc::StaticCompressionEngine::gainForLevel (
            linkedLevel, ratioSTNow, thresholdSTLinear);

        if (mode == qqsc::params::midSide)
        {
            displayDetectorPeak[0] = juce::jmax (displayDetectorPeak[0], midEngine.getCurrentLevel());
            displayDetectorPeak[1] = juce::jmax (displayDetectorPeak[1], sideEngine.getCurrentLevel());
        }
        else if (mode == qqsc::params::leftRight)
        {
            displayDetectorPeak[0] = juce::jmax (displayDetectorPeak[0], leftEngine.getCurrentLevel());
            displayDetectorPeak[1] = juce::jmax (displayDetectorPeak[1], rightEngine.getCurrentLevel());
        }
        else
        {
            displayDetectorPeak[0] = juce::jmax (displayDetectorPeak[0], linkedLevel);
        }
        oversampledLookaheadDelayBuffer.setSample (0, oversampledDelayWriteIndex, inputL);
        oversampledLookaheadDelayBuffer.setSample (1, oversampledDelayWriteIndex, inputR);
        oversampledKeyHistoryBuffer.setSample (0, oversampledDelayWriteIndex, keyL);
        oversampledKeyHistoryBuffer.setSample (1, oversampledDelayWriteIndex, keyR);
        oversampledKeyHistoryBuffer.setSample (2, oversampledDelayWriteIndex, keyM);
        oversampledKeyHistoryBuffer.setSample (3, oversampledDelayWriteIndex, keyS);

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
            const auto linkedReductionDb = juce::jmax (0.0f,
                -juce::Decibels::gainToDecibels (juce::jmax (linkedGain, 1.0e-9f), -180.0f));
            gr0 = linkedReductionDb;
            gr1 = linkedReductionDb;
        }

        meterMaxGrDb[0] = juce::jmax (meterMaxGrDb[0], gr0);
        meterMaxGrDb[1] = juce::jmax (meterMaxGrDb[1], stereoBus || mode == qqsc::params::midSide ? gr1 : 0.0f);
    }

    auto wetBlock = juce::dsp::AudioBlock<float> (wetBaseBuffer)
                        .getSubsetChannelBlock (0, 6)
                        .getSubBlock (0, static_cast<size_t> (numSamples));
    oversampler.processSamplesDown (wetBlock);

    float meterInputPeak[2]  { 0.0f, 0.0f };
    float meterOutputPeak[2] { 0.0f, 0.0f };
    float displayInputPeak[2]  { 0.0f, 0.0f };

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
        keyListenDelayBuffer.setSample (0, dryDelayWriteIndex, keyInputBuffer.getSample (0, i));
        keyListenDelayBuffer.setSample (1, dryDelayWriteIndex, keyInputBuffer.getSample (1, i));

        int dryReadIndex = dryDelayWriteIndex - currentTotalLatencySamples;
        while (dryReadIndex < 0)
            dryReadIndex += dryDelayCapacity;

        const float dryL = dryDelayBuffer.getSample (0, dryReadIndex);
        const float dryR = stereoBus ? dryDelayBuffer.getSample (1, dryReadIndex) : 0.0f;
        const float displayDryL = originalDryDelayBuffer.getSample (0, dryReadIndex);
        const float displayDryR = stereoBus ? originalDryDelayBuffer.getSample (1, dryReadIndex) : 0.0f;
        const float delayedKeyL = keyListenDelayBuffer.getSample (0, dryReadIndex);
        const float delayedKeyR = keyListenDelayBuffer.getSample (1, dryReadIndex);
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
        const auto mixST = mixSmoother.getNextValue();
        const auto mixL  = mixLSmoother.getNextValue();
        const auto mixR  = mixRSmoother.getNextValue();
        const auto mixM  = mixMSmoother.getNextValue();
        const auto mixS  = mixSSmoother.getNextValue();

        float wetL = wetLinkedL;
        float wetR = wetLinkedR;
        float mixedL = dryL + (wetLinkedL * makeupST - dryL) * mixST;
        float mixedR = dryR + (wetLinkedR * makeupST - dryR) * mixST;

        if (mode == qqsc::params::midSide)
        {
            wetL = stereoBus ? wetM + wetS : wetM;
            wetR = stereoBus ? wetM - wetS : 0.0f;

            // Independent M/S Mix must occur in the M/S domain before decoding
            // back to L/R. With equal Mix values this is exactly equivalent to
            // the legacy shared post-decode blend; unequal values now remain
            // genuinely independent.
            const float mixedM = dryM + (wetM * makeupM - dryM) * mixM;
            const float mixedS = dryS + (wetS * makeupS - dryS) * mixS;
            mixedL = stereoBus ? mixedM + mixedS : mixedM;
            mixedR = stereoBus ? mixedM - mixedS : 0.0f;

            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryM));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryS));
        }
        else if (mode == qqsc::params::leftRight)
        {
            wetL = wetIndependentL;
            wetR = wetIndependentR;
            mixedL = dryL + (wetL * makeupL - dryL) * mixL;
            mixedR = dryR + (wetR * makeupR - dryR) * mixR;

            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryL));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryR));
        }
        else
        {
            meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryL));
            meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryR));
        }
        const auto outputGain = outputGainSmoother.getNextValue();
        const float activeOutL = mixedL * outputGain;
        const float activeOutR = mixedR * outputGain;

        // True Bypass retains the same combined latency but bypasses Input Gain,
        // compression, Makeup, Mix and Output Gain. This preserves the established
        // latency-safe bypass contract while keeping Gain trims part of the effect.
        const float outL = forceBypass ? displayDryL : activeOutL;
        const float outR = forceBypass ? displayDryR : activeOutR;

        // v1.0.3 centered domain monitor: audition affects only what reaches the
        // headphones/speakers. Display, meters, Match and stored processing result
        // continue to use the normal pre-monitor outL/outR below. This mirrors the
        // mature QQ ChainScope Mixboard convention for centered L/R/Side audition.
        float audibleOutL = outL;
        float audibleOutR = outR;

        if (! forceBypass && sidechainListen.load (std::memory_order_relaxed))
        {
            audibleOutL = delayedKeyL;
            audibleOutR = delayedKeyR;
        }
        else if (! forceBypass && stereoBus)
        {
            const auto monitorSelection = getDomainMonitorSelection (mode);

            if (mode == qqsc::params::leftRight)
            {
                if (monitorSelection == qqsc::params::monitorFirst)
                    audibleOutL = audibleOutR = outL * centeredChannelMonitorGain;
                else if (monitorSelection == qqsc::params::monitorSecond)
                    audibleOutL = audibleOutR = outR * centeredChannelMonitorGain;
            }
            else if (mode == qqsc::params::midSide)
            {
                const float activeM = 0.5f * (outL + outR);
                const float activeS = 0.5f * (outL - outR);

                // Mid is already the centre component under this plug-in's
                // M=(L+R)/2 matrix, so no extra -3.01 dB is applied. Side is a
                // single derived component copied to both ears and therefore uses
                // the same 1/sqrt(2) centered-listening compensation as L/R.
                if (monitorSelection == qqsc::params::monitorFirst)
                    audibleOutL = audibleOutR = activeM;
                else if (monitorSelection == qqsc::params::monitorSecond)
                    audibleOutL = audibleOutR = activeS * centeredChannelMonitorGain;
            }
        }

        buffer.setSample (0, i, audibleOutL);
        if (stereoBus)
            buffer.setSample (1, i, audibleOutR);

        if (mode == qqsc::params::midSide)
        {
            const float outM = stereoBus ? 0.5f * (outL + outR) : outL;
            const float outS = stereoBus ? 0.5f * (outL - outR) : 0.0f;
            meterOutputPeak[0] = juce::jmax (meterOutputPeak[0], std::abs (outM));
            meterOutputPeak[1] = juce::jmax (meterOutputPeak[1], std::abs (outS));

            const float displayDryM = stereoBus ? 0.5f * (displayDryL + displayDryR) : displayDryL;
            const float displayDryS = stereoBus ? 0.5f * (displayDryL - displayDryR) : 0.0f;
            displayInputPeak[0] = juce::jmax (displayInputPeak[0], std::abs (displayDryM));
            displayInputPeak[1] = juce::jmax (displayInputPeak[1], std::abs (displayDryS));
        }
        else
        {
            meterOutputPeak[0] = juce::jmax (meterOutputPeak[0], std::abs (outL));
            meterOutputPeak[1] = juce::jmax (meterOutputPeak[1], std::abs (outR));

            if (mode == qqsc::params::leftRight)
            {
                displayInputPeak[0] = juce::jmax (displayInputPeak[0], std::abs (displayDryL));
                displayInputPeak[1] = juce::jmax (displayInputPeak[1], std::abs (displayDryR));
            }
            else
            {
                // ST remains one linked Display panel. Channel 1 is unused.
                displayInputPeak[0] = juce::jmax (displayInputPeak[0],
                                                  stereoBus ? maxAbs (displayDryL, displayDryR) : std::abs (displayDryL));
            }
        }
    }

    if (transportPlaying)
        updateMatchResults();

    lastTransportPlaying = transportPlaying;
    lastTransportSample = transportSample;
    lastTransportBlockSize = numSamples;

    float meterMix0 = mixSmoother.getCurrentValue();
    float meterMix1 = meterMix0;
    if (mode == qqsc::params::midSide)
    {
        meterMix0 = mixMSmoother.getCurrentValue();
        meterMix1 = mixSSmoother.getCurrentValue();
    }
    else if (mode == qqsc::params::leftRight)
    {
        meterMix0 = mixLSmoother.getCurrentValue();
        meterMix1 = mixRSmoother.getCurrentValue();
    }

    const auto effectiveGrForMeter = [] (float coreGrDb, float wetMix)
    {
        const auto compressedGain = juce::Decibels::decibelsToGain (-juce::jmax (0.0f, coreGrDb));
        return qqsc::StaticCompressionEngine::effectiveGainReductionDb (compressedGain, wetMix);
    };

    const auto effectiveGr0 = forceBypass ? 0.0f : effectiveGrForMeter (meterMaxGrDb[0], meterMix0);
    const auto effectiveGr1 = forceBypass ? 0.0f : effectiveGrForMeter (meterMaxGrDb[1], meterMix1);

    meterState.inputDb0.store  (peakToDb (meterInputPeak[0]), std::memory_order_relaxed);
    meterState.inputDb1.store  (peakToDb (meterInputPeak[1]), std::memory_order_relaxed);
    meterState.outputDb0.store (peakToDb (meterOutputPeak[0]), std::memory_order_relaxed);
    meterState.outputDb1.store (peakToDb (meterOutputPeak[1]), std::memory_order_relaxed);

    meterState.displayInputDb0.store  (peakToDb (displayInputPeak[0]), std::memory_order_relaxed);
    meterState.displayInputDb1.store  (peakToDb (displayInputPeak[1]), std::memory_order_relaxed);
    meterState.displayDetectorDb0.store (peakToDb (displayDetectorPeak[0]), std::memory_order_relaxed);
    meterState.displayDetectorDb1.store (peakToDb (displayDetectorPeak[1]), std::memory_order_relaxed);

    meterState.gainReductionDb0.store (effectiveGr0, std::memory_order_relaxed);
    meterState.gainReductionDb1.store (effectiveGr1, std::memory_order_relaxed);
    meterState.keyInputDb.store (peakToDb (keyInputPeak), std::memory_order_relaxed);

    updateGainReductionHoldChannel (0, effectiveGr0, numSamples);
    updateGainReductionHoldChannel (1, stereoBus ? effectiveGr1 : 0.0f, numSamples);
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
    snapshot.ratioL = apvts.getRawParameterValue (qqsc::params::ratioL)->load();
    snapshot.ratioR = apvts.getRawParameterValue (qqsc::params::ratioR)->load();
    snapshot.ratioM = apvts.getRawParameterValue (qqsc::params::ratioM)->load();
    snapshot.ratioS = apvts.getRawParameterValue (qqsc::params::ratioS)->load();
    snapshot.thresholdDb = apvts.getRawParameterValue (qqsc::params::thresholdDb)->load();
    snapshot.thresholdLDb = apvts.getRawParameterValue (qqsc::params::thresholdLDb)->load();
    snapshot.thresholdRDb = apvts.getRawParameterValue (qqsc::params::thresholdRDb)->load();
    snapshot.thresholdMDb = apvts.getRawParameterValue (qqsc::params::thresholdMDb)->load();
    snapshot.thresholdSDb = apvts.getRawParameterValue (qqsc::params::thresholdSDb)->load();
    snapshot.makeupST = apvts.getRawParameterValue (qqsc::params::makeupGainDb)->load();
    snapshot.makeupL = apvts.getRawParameterValue (qqsc::params::makeupGainLDb)->load();
    snapshot.makeupR = apvts.getRawParameterValue (qqsc::params::makeupGainRDb)->load();
    snapshot.makeupM = apvts.getRawParameterValue (qqsc::params::makeupGainMDb)->load();
    snapshot.makeupS = apvts.getRawParameterValue (qqsc::params::makeupGainSDb)->load();
    snapshot.mix = apvts.getRawParameterValue (qqsc::params::mix)->load();
    snapshot.mixL = apvts.getRawParameterValue (qqsc::params::mixL)->load();
    snapshot.mixR = apvts.getRawParameterValue (qqsc::params::mixR)->load();
    snapshot.mixM = apvts.getRawParameterValue (qqsc::params::mixM)->load();
    snapshot.mixS = apvts.getRawParameterValue (qqsc::params::mixS)->load();
    snapshot.outputGainDb = apvts.getRawParameterValue (qqsc::params::outputGainDb)->load();
    snapshot.lookaheadMs = qqsc::params::snapLookaheadMs (apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load());
    snapshot.oversampling = juce::jlimit (0, 2, juce::roundToInt (apvts.getRawParameterValue (qqsc::params::oversampling)->load()));
    snapshot.mode = juce::roundToInt (apvts.getRawParameterValue (qqsc::params::processingMode)->load());
    snapshot.keySource = juce::jlimit (0, 1, juce::roundToInt (apvts.getRawParameterValue (qqsc::params::keySource)->load()));
    snapshot.keyGainDb = apvts.getRawParameterValue (qqsc::params::keyGainDb)->load();
    snapshot.keyHpfHz = apvts.getRawParameterValue (qqsc::params::keyHpfHz)->load();
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
    setActualParameterValue (qqsc::params::ratioL, snapshot.ratioL);
    setActualParameterValue (qqsc::params::ratioR, snapshot.ratioR);
    setActualParameterValue (qqsc::params::ratioM, snapshot.ratioM);
    setActualParameterValue (qqsc::params::ratioS, snapshot.ratioS);
    setActualParameterValue (qqsc::params::thresholdDb, snapshot.thresholdDb);
    setActualParameterValue (qqsc::params::thresholdLDb, snapshot.thresholdLDb);
    setActualParameterValue (qqsc::params::thresholdRDb, snapshot.thresholdRDb);
    setActualParameterValue (qqsc::params::thresholdMDb, snapshot.thresholdMDb);
    setActualParameterValue (qqsc::params::thresholdSDb, snapshot.thresholdSDb);
    setActualParameterValue (qqsc::params::makeupGainDb, snapshot.makeupST);
    setActualParameterValue (qqsc::params::makeupGainLDb, snapshot.makeupL);
    setActualParameterValue (qqsc::params::makeupGainRDb, snapshot.makeupR);
    setActualParameterValue (qqsc::params::makeupGainMDb, snapshot.makeupM);
    setActualParameterValue (qqsc::params::makeupGainSDb, snapshot.makeupS);
    setActualParameterValue (qqsc::params::mix, snapshot.mix);
    setActualParameterValue (qqsc::params::mixL, snapshot.mixL);
    setActualParameterValue (qqsc::params::mixR, snapshot.mixR);
    setActualParameterValue (qqsc::params::mixM, snapshot.mixM);
    setActualParameterValue (qqsc::params::mixS, snapshot.mixS);
    setActualParameterValue (qqsc::params::outputGainDb, snapshot.outputGainDb);
    const auto snapshotLookaheadMs = qqsc::params::snapLookaheadMs (snapshot.lookaheadMs);
    setActualParameterValue (qqsc::params::lookaheadMs, snapshotLookaheadMs);
    setActualParameterValue (qqsc::params::oversampling, static_cast<float> (juce::jlimit (0, 2, snapshot.oversampling)));
    notifyHostProcessingLatency();
    setActualParameterValue (qqsc::params::processingMode, static_cast<float> (snapshot.mode));
    setActualParameterValue (qqsc::params::keySource, static_cast<float> (juce::jlimit (0, 1, snapshot.keySource)));
    setActualParameterValue (qqsc::params::keyGainDb, snapshot.keyGainDb);
    setActualParameterValue (qqsc::params::keyHpfHz, snapshot.keyHpfHz);
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
        state.setProperty (abProperty (prefix + "ratioL"), s.ratioL, nullptr);
        state.setProperty (abProperty (prefix + "ratioR"), s.ratioR, nullptr);
        state.setProperty (abProperty (prefix + "ratioM"), s.ratioM, nullptr);
        state.setProperty (abProperty (prefix + "ratioS"), s.ratioS, nullptr);
        state.setProperty (abProperty (prefix + "thresholdDb"), s.thresholdDb, nullptr);
        state.setProperty (abProperty (prefix + "thresholdLDb"), s.thresholdLDb, nullptr);
        state.setProperty (abProperty (prefix + "thresholdRDb"), s.thresholdRDb, nullptr);
        state.setProperty (abProperty (prefix + "thresholdMDb"), s.thresholdMDb, nullptr);
        state.setProperty (abProperty (prefix + "thresholdSDb"), s.thresholdSDb, nullptr);
        state.setProperty (abProperty (prefix + "makeupST"), s.makeupST, nullptr);
        state.setProperty (abProperty (prefix + "makeupL"), s.makeupL, nullptr);
        state.setProperty (abProperty (prefix + "makeupR"), s.makeupR, nullptr);
        state.setProperty (abProperty (prefix + "makeupM"), s.makeupM, nullptr);
        state.setProperty (abProperty (prefix + "makeupS"), s.makeupS, nullptr);
        state.setProperty (abProperty (prefix + "mix"), s.mix, nullptr);
        state.setProperty (abProperty (prefix + "mixL"), s.mixL, nullptr);
        state.setProperty (abProperty (prefix + "mixR"), s.mixR, nullptr);
        state.setProperty (abProperty (prefix + "mixM"), s.mixM, nullptr);
        state.setProperty (abProperty (prefix + "mixS"), s.mixS, nullptr);
        state.setProperty (abProperty (prefix + "outputGainDb"), s.outputGainDb, nullptr);
        state.setProperty (abProperty (prefix + "lookaheadMs"), s.lookaheadMs, nullptr);
        state.setProperty (abProperty (prefix + "oversampling"), s.oversampling, nullptr);
        state.setProperty (abProperty (prefix + "mode"), s.mode, nullptr);
        state.setProperty (abProperty (prefix + "keySource"), s.keySource, nullptr);
        state.setProperty (abProperty (prefix + "keyGainDb"), s.keyGainDb, nullptr);
        state.setProperty (abProperty (prefix + "keyHpfHz"), s.keyHpfHz, nullptr);
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
            s.ratioL = static_cast<float> (state.getProperty (abProperty (prefix + "ratioL"), s.ratio));
            s.ratioR = static_cast<float> (state.getProperty (abProperty (prefix + "ratioR"), s.ratio));
            s.ratioM = static_cast<float> (state.getProperty (abProperty (prefix + "ratioM"), s.ratio));
            s.ratioS = static_cast<float> (state.getProperty (abProperty (prefix + "ratioS"), s.ratio));
            s.thresholdDb = static_cast<float> (state.getProperty (abProperty (prefix + "thresholdDb"), s.thresholdDb));
            s.thresholdLDb = static_cast<float> (state.getProperty (abProperty (prefix + "thresholdLDb"), s.thresholdDb));
            s.thresholdRDb = static_cast<float> (state.getProperty (abProperty (prefix + "thresholdRDb"), s.thresholdDb));
            s.thresholdMDb = static_cast<float> (state.getProperty (abProperty (prefix + "thresholdMDb"), s.thresholdDb));
            s.thresholdSDb = static_cast<float> (state.getProperty (abProperty (prefix + "thresholdSDb"), s.thresholdDb));
            s.makeupST = static_cast<float> (state.getProperty (abProperty (prefix + "makeupST"), s.makeupST));
            s.makeupL = static_cast<float> (state.getProperty (abProperty (prefix + "makeupL"), s.makeupL));
            s.makeupR = static_cast<float> (state.getProperty (abProperty (prefix + "makeupR"), s.makeupR));
            s.makeupM = static_cast<float> (state.getProperty (abProperty (prefix + "makeupM"), s.makeupM));
            s.makeupS = static_cast<float> (state.getProperty (abProperty (prefix + "makeupS"), s.makeupS));
            s.mix = static_cast<float> (state.getProperty (abProperty (prefix + "mix"), s.mix));
            s.mixL = static_cast<float> (state.getProperty (abProperty (prefix + "mixL"), s.mix));
            s.mixR = static_cast<float> (state.getProperty (abProperty (prefix + "mixR"), s.mix));
            s.mixM = static_cast<float> (state.getProperty (abProperty (prefix + "mixM"), s.mix));
            s.mixS = static_cast<float> (state.getProperty (abProperty (prefix + "mixS"), s.mix));
            s.outputGainDb = static_cast<float> (state.getProperty (abProperty (prefix + "outputGainDb"), s.outputGainDb));
            s.lookaheadMs = qqsc::params::snapLookaheadMs (
                static_cast<float> (state.getProperty (abProperty (prefix + "lookaheadMs"), s.lookaheadMs)));
            const auto storedOversampling = static_cast<int> (state.getProperty (abProperty (prefix + "oversampling"), s.oversampling));
            s.oversampling = legacyOversamplingSchema
                ? (storedOversampling <= 0 ? 0 : 1)
                : juce::jlimit (0, 2, storedOversampling);
            s.mode = static_cast<int> (state.getProperty (abProperty (prefix + "mode"), s.mode));
            s.keySource = juce::jlimit (0, 1, static_cast<int> (
                state.getProperty (abProperty (prefix + "keySource"), qqsc::params::keyInternal)));
            s.keyGainDb = static_cast<float> (
                state.getProperty (abProperty (prefix + "keyGainDb"), 0.0f));
            s.keyHpfHz = static_cast<float> (
                state.getProperty (abProperty (prefix + "keyHpfHz"), qqsc::params::keyHpfOffHz));
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

int QQSuperCompressionAudioProcessor::getDomainMonitorSelection (int processingMode) const noexcept
{
    if (processingMode == qqsc::params::leftRight)
        return juce::jlimit (0, 2, monitorLRSelection.load (std::memory_order_relaxed));

    if (processingMode == qqsc::params::midSide)
        return juce::jlimit (0, 2, monitorMSSelection.load (std::memory_order_relaxed));

    return qqsc::params::monitorAll;
}

void QQSuperCompressionAudioProcessor::setDomainMonitorSelection (int processingMode, int selection) noexcept
{
    const auto clamped = juce::jlimit (0, 2, selection);

    if (processingMode == qqsc::params::leftRight)
        monitorLRSelection.store (clamped, std::memory_order_relaxed);
    else if (processingMode == qqsc::params::midSide)
        monitorMSSelection.store (clamped, std::memory_order_relaxed);
}

juce::AudioProcessorParameter* QQSuperCompressionAudioProcessor::getBypassParameter() const
{
    return apvts.getParameter (qqsc::params::bypass);
}

void QQSuperCompressionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (stateSchemaProperty, currentStateSchemaVersion, nullptr);
    state.setProperty (monitorLRProperty, getDomainMonitorSelection (qqsc::params::leftRight), nullptr);
    state.setProperty (monitorMSProperty, getDomainMonitorSelection (qqsc::params::midSide), nullptr);
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
            const bool stateHasThreshold = stateContainsParameter (state, qqsc::params::thresholdDb);
            const bool stateHasRatioL = stateContainsParameter (state, qqsc::params::ratioL);
            const bool stateHasRatioR = stateContainsParameter (state, qqsc::params::ratioR);
            const bool stateHasRatioM = stateContainsParameter (state, qqsc::params::ratioM);
            const bool stateHasRatioS = stateContainsParameter (state, qqsc::params::ratioS);
            const bool stateHasThresholdL = stateContainsParameter (state, qqsc::params::thresholdLDb);
            const bool stateHasThresholdR = stateContainsParameter (state, qqsc::params::thresholdRDb);
            const bool stateHasThresholdM = stateContainsParameter (state, qqsc::params::thresholdMDb);
            const bool stateHasThresholdS = stateContainsParameter (state, qqsc::params::thresholdSDb);
            const bool stateHasDomainLink = stateContainsParameter (state, qqsc::params::domainLink);
            const bool stateHasMixL = stateContainsParameter (state, qqsc::params::mixL);
            const bool stateHasMixR = stateContainsParameter (state, qqsc::params::mixR);
            const bool stateHasMixM = stateContainsParameter (state, qqsc::params::mixM);
            const bool stateHasMixS = stateContainsParameter (state, qqsc::params::mixS);
            const bool stateHasKeySource = stateContainsParameter (state, qqsc::params::keySource);
            const bool stateHasKeyGain = stateContainsParameter (state, qqsc::params::keyGainDb);
            const bool stateHasKeyHpf = stateContainsParameter (state, qqsc::params::keyHpfHz);
            const auto legacyOversamplingNormalised = stateParameterNormalisedValue (state, qqsc::params::oversampling);
            const auto restoredMonitorLR = static_cast<int> (state.getProperty (monitorLRProperty, qqsc::params::monitorAll));
            const auto restoredMonitorMS = static_cast<int> (state.getProperty (monitorMSProperty, qqsc::params::monitorAll));
            apvts.replaceState (state);
            setDomainMonitorSelection (qqsc::params::leftRight, restoredMonitorLR);
            setDomainMonitorSelection (qqsc::params::midSide, restoredMonitorMS);

            // Pre-0.9.2 projects have no trim parameters. They migrate explicitly
            // to unity gain so loading an older project cannot acquire a hidden
            // level change from whatever value a newly-created instance had.
            if (! stateHasInputGain)
                setActualParameterValue (qqsc::params::inputGainDb, 0.0f);
            if (! stateHasOutputGain)
                setActualParameterValue (qqsc::params::outputGainDb, 0.0f);
            // Any project from the v0.9.4 baseline or earlier has no Threshold.
            // Migrate to OFF so it remains sample-for-sample on the legacy law.
            if (! stateHasThreshold)
                setActualParameterValue (qqsc::params::thresholdDb, qqsc::params::thresholdOffDb);

            // v1.0.0 split LR/MS Ratio and Threshold into independent domains.
            // Older states used one common Ratio/Threshold, so copy those legacy
            // values into every missing domain parameter to preserve their sound.
            const auto legacyRatio = apvts.getRawParameterValue (qqsc::params::ratio)->load();
            const auto legacyThreshold = apvts.getRawParameterValue (qqsc::params::thresholdDb)->load();
            if (! stateHasRatioL) setActualParameterValue (qqsc::params::ratioL, legacyRatio);
            if (! stateHasRatioR) setActualParameterValue (qqsc::params::ratioR, legacyRatio);
            if (! stateHasRatioM) setActualParameterValue (qqsc::params::ratioM, legacyRatio);
            if (! stateHasRatioS) setActualParameterValue (qqsc::params::ratioS, legacyRatio);
            if (! stateHasThresholdL) setActualParameterValue (qqsc::params::thresholdLDb, legacyThreshold);
            if (! stateHasThresholdR) setActualParameterValue (qqsc::params::thresholdRDb, legacyThreshold);
            if (! stateHasThresholdM) setActualParameterValue (qqsc::params::thresholdMDb, legacyThreshold);
            if (! stateHasThresholdS) setActualParameterValue (qqsc::params::thresholdSDb, legacyThreshold);
            if (! stateHasDomainLink) setActualParameterValue (qqsc::params::domainLink, 1.0f);

            // v1.0.1 revision splits Mix per LR/MS domain. Older projects had
            // one shared Mix, so copy that exact value into every missing domain.
            const auto legacyMix = apvts.getRawParameterValue (qqsc::params::mix)->load();
            if (! stateHasMixL) setActualParameterValue (qqsc::params::mixL, legacyMix);
            if (! stateHasMixR) setActualParameterValue (qqsc::params::mixR, legacyMix);
            if (! stateHasMixM) setActualParameterValue (qqsc::params::mixM, legacyMix);
            if (! stateHasMixS) setActualParameterValue (qqsc::params::mixS, legacyMix);

            // v1.0.4 and older have no External Key parameters. Explicit INT / 0 dB
            // migration guarantees that old projects retain their detector source
            // and sound even if a fresh instance had been edited before restore.
            if (! stateHasKeySource)
                setActualParameterValue (qqsc::params::keySource, static_cast<float> (qqsc::params::keyInternal));
            if (! stateHasKeyGain)
                setActualParameterValue (qqsc::params::keyGainDb, 0.0f);

            // v1.1.0 and older have no detector HPF. OFF preserves their detector
            // waveform and therefore their exact compression behaviour.
            if (! stateHasKeyHpf)
                setActualParameterValue (qqsc::params::keyHpfHz, qqsc::params::keyHpfOffHz);

            // SC Listen is intentionally never restored from project state.
            sidechainListen.store (false, std::memory_order_relaxed);

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
