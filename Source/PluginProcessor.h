#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>
#include "Parameters.h"
#include "StaticCompressionEngine.h"
#include "MeterState.h"
#include "BS1770LoudnessMatch.h"

class QQSuperCompressionAudioProcessor final : public juce::AudioProcessor
{
public:
    QQSuperCompressionAudioProcessor();
    ~QQSuperCompressionAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const noexcept { return apvts; }
    qqsc::MeterState& getMeterState() noexcept { return meterState; }
    juce::UndoManager& getUndoManager() noexcept { return undoManager; }

    // UI/state changes can tell the host about the combined Lookahead +
    // Oversampling filter latency immediately, even while transport is stopped.
    // The audio-thread configuration follows from the same APVTS parameters on
    // the next process block.
    void notifyHostProcessingLatency();

    // Headphone-reference audition monitor for the independent LR/MS domains.
    // This is deliberately not an APVTS parameter: it affects only the final
    // audible monitoring path, is excluded from A/B and host automation, and is
    // persisted as project-local workflow state. LR and MS remember separately.
    int getDomainMonitorSelection (int processingMode) const noexcept;
    void setDomainMonitorSelection (int processingMode, int selection) noexcept;

    // External Key audition is a safety-oriented workflow state: it is not
    // host-automatable, not stored in A/B, and resets OFF with a new instance.
    bool isSidechainListenEnabled() const noexcept { return sidechainListen.load (std::memory_order_relaxed); }
    void setSidechainListenEnabled (bool enabled) noexcept { sidechainListen.store (enabled, std::memory_order_relaxed); }
    bool isExternalSidechainBusAvailable() const noexcept
    {
        return meterState.externalKeyBusAvailable.load (std::memory_order_relaxed);
    }

    // UI A/B comparison. A/B stores the complete user sound-setting state
    // (Input/Output Gain, all Ratio/Threshold/Makeup/Mix domain values, Lookahead, Oversampling and Mode). Bypass is intentionally global
    // and is not part of A/B snapshots.
    int getActiveABSlot() const noexcept { return activeABSlot.load (std::memory_order_relaxed); }
    void selectABSlot (int slot);
    void copyAToB();
    void copyBToA();

    // Strict Integrated LUFS Match (ITU-R BS.1770 / EBU R128 gating). During
    // host playback the processor measures Dry vs compressed Wet pre-Makeup in
    // ST, LR and MS domains simultaneously. Match writes the relevant Makeup.
    bool hasMatchData() const noexcept;
    bool applyMatchForCurrentMode();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct KeyHighPassCoefficients
    {
        float b0 = 1.0f;
        float b1 = 0.0f;
        float b2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
    };

    struct KeyHighPassState
    {
        float z1 = 0.0f;
        float z2 = 0.0f;

        float process (float input, const KeyHighPassCoefficients& c) noexcept
        {
            const auto output = c.b0 * input + z1;
            z1 = c.b1 * input - c.a1 * output + z2;
            z2 = c.b2 * input - c.a2 * output;
            return output;
        }

        void reset() noexcept { z1 = z2 = 0.0f; }
    };

    struct ParameterSnapshot
    {
        float inputGainDb = 0.0f;
        float ratio = 8.0f;
        float ratioL = 8.0f;
        float ratioR = 8.0f;
        float ratioM = 8.0f;
        float ratioS = 8.0f;
        float thresholdDb = qqsc::params::thresholdOffDb;
        float thresholdLDb = qqsc::params::thresholdOffDb;
        float thresholdRDb = qqsc::params::thresholdOffDb;
        float thresholdMDb = qqsc::params::thresholdOffDb;
        float thresholdSDb = qqsc::params::thresholdOffDb;
        float makeupST = 0.0f;
        float makeupL = 0.0f;
        float makeupR = 0.0f;
        float makeupM = 0.0f;
        float makeupS = 0.0f;
        float mix = 100.0f;
        float mixL = 100.0f;
        float mixR = 100.0f;
        float mixM = 100.0f;
        float mixS = 100.0f;
        float outputGainDb = 0.0f;
        float lookaheadMs = 26.0f;
        int oversampling = 1;
        int mode = qqsc::params::stereoLinked;
        int keySource = qqsc::params::keyInternal;
        float keyGainDb = 0.0f;
        float keyHpfHz = qqsc::params::keyHpfOffHz;
    };

    void processBlockInternal (juce::AudioBuffer<float>&, bool forceBypass);

    ParameterSnapshot captureCurrentSnapshot() const noexcept;
    void applySnapshot (const ParameterSnapshot&);
    void refreshActiveSnapshot();
    void writeABStateTo (juce::ValueTree& state);
    void readABStateFrom (const juce::ValueTree& state, bool legacyOversamplingSchema);
    void setActualParameterValue (const char* parameterID, float actualValue);

    void updateProcessingConfiguration (bool force = false);
    void resetOversampledCoreState() noexcept;
    void resetDetectorCoreState() noexcept;
    void resetDryDelayState() noexcept;
    void resetKeyHighPassState() noexcept;
    void updateKeyHighPassCoefficients (float cutoffHz) noexcept;
    void resetAllProcessingState() noexcept;
    juce::dsp::Oversampling<float>& getCurrentOversampler() noexcept;
    int getOversamplingLatencySamples (int oversamplingIndex) const noexcept;
    int getCombinedLatencySamples (float requestedLookaheadMs, int oversamplingIndex) const noexcept;

    void resetMatchAccumulator() noexcept;
    void updateMatchResults() noexcept;

    void resetGainReductionHold (int mode = -1) noexcept;
    void updateGainReductionHoldChannel (int channel, float blockPeakGrDb, int blockSamples) noexcept;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    // All four future-window peak analysers run continuously so ST/MS/LR
    // switching uses the same transparent detector semantics in every domain.
    // ST derives its linked gain from the current L/R window levels.
    qqsc::StaticCompressionEngine leftEngine;
    qqsc::StaticCompressionEngine rightEngine;
    qqsc::StaticCompressionEngine midEngine;
    qqsc::StaticCompressionEngine sideEngine;

    qqsc::MeterState meterState;

    // 0.1.10: Oversampling is deliberately a Lookahead=0 ms-only option.
    // User-facing choices are 1x/8x/16x; 10 ms and longer always use the 1x
    // path regardless of the stored 0 ms choice. All three paths are created
    // ahead of time so switching never constructs filters on the audio thread.
    // Index 0 is JUCE's dummy 1x stage; indices 1/2 are 8x/16x maximum-quality
    // linear-phase FIR stages with integer latency compensation enabled.
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 3> oversamplers;
    juce::AudioBuffer<float> wetBaseBuffer;
    // Six-channel staging keeps main L/R and optional Key L/R explicit before
    // the shared 1x/8x/16x Oversampling stage. Key channels are overwritten by
    // wet variants only after the detector has consumed them.
    juce::AudioBuffer<float> oversamplingInputBuffer;
    juce::AudioBuffer<float> keyInputBuffer;
    // v0.9.2 keeps a host-rate copy of the untouched input so the Dynamic
    // Display Dry/Input reference and true bypass remain pre-Input-Gain.
    juce::AudioBuffer<float> originalInputBuffer;

    // Internal detector/audio delay ring. Only the original L/R input is stored;
    // the corresponding delayed samples are multiplied by the gains derived from
    // the same future window. Non-zero Lookahead is always 1x; only 0 ms may use
    // 8x/16x.
    juce::AudioBuffer<float> oversampledLookaheadDelayBuffer;
    // Stores the exact L/R/M/S detector source history so changing Lookahead
    // can rebuild the future-window queues for either INT or EXT without ever
    // substituting the main carrier for an external key.
    juce::AudioBuffer<float> oversampledKeyHistoryBuffer;
    int oversampledDelayCapacity = 1;
    int oversampledDelayWriteIndex = 0;
    int maxLookaheadSamplesBase = 0;
    int maxLookaheadSamplesInternal = 0;
    int currentLookaheadSamplesBase = -1;
    int currentLookaheadSamplesInternal = -1;
    int64_t detectorSampleCounter = 0;

    // Dry stays at host sample rate. It is delayed by Lookahead plus the exact
    // integer latency of the selected 0 ms Oversampling filter, so Dry, Wet, Mix
    // and Bypass remain sample-aligned.
    juce::AudioBuffer<float> dryDelayBuffer;
    juce::AudioBuffer<float> originalDryDelayBuffer;
    juce::AudioBuffer<float> keyListenDelayBuffer;
    int dryDelayCapacity = 1;
    int dryDelayWriteIndex = 0;
    int currentOversamplingIndex = -1;
    int currentKeySource = -1;
    int currentOversamplingFactor = 1;
    int currentTotalLatencySamples = 0;
    int configuredMaximumBlockSize = 1;
    double currentSampleRate = 44100.0;

    // Parameter smoothing only prevents zipper noise while controls move. It is
    // not the compressor's user Attack/Release behaviour.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> keyGainSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> keyHpfCutoffSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> keyHpfWetSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ratioSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ratioLSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ratioRSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ratioMSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ratioSSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupSTSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupLSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupRSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupMSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupSSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixLSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixRSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixMSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoother;

    KeyHighPassCoefficients keyHighPassCoefficients;
    std::array<KeyHighPassState, 2> keyHighPassStates;
    int keyHpfCoefficientCountdown = 0;

    mutable juce::CriticalSection abLock;
    ParameterSnapshot snapshotA;
    ParameterSnapshot snapshotB;
    std::atomic<int> activeABSlot { 0 };

    // v1.0.3: final audible-only centered monitor state. Keep LR and MS
    // selections independent so changing processing mode never silently maps
    // an L solo into M (or R into S). Defaults are ALL.
    std::atomic<int> monitorLRSelection { qqsc::params::monitorAll };
    std::atomic<int> monitorMSSelection { qqsc::params::monitorAll };
    std::atomic<bool> sidechainListen { false };

    qqsc::BS1770LoudnessMatch loudnessMatch;
    std::atomic<float> matchSTDb { 0.0f };
    std::atomic<float> matchLDb  { 0.0f };
    std::atomic<float> matchRDb  { 0.0f };
    std::atomic<float> matchMDb  { 0.0f };
    std::atomic<float> matchSDb  { 0.0f };
    std::atomic<bool> matchSTValid { false };
    std::atomic<bool> matchLValid  { false };
    std::atomic<bool> matchRValid  { false };
    std::atomic<bool> matchMValid  { false };
    std::atomic<bool> matchSValid  { false };
    std::atomic<bool> matchReady { false };
    std::atomic<bool> resetMatchOnNextPlaybackBlock { true };
    bool lastTransportPlaying = false;
    int64_t lastTransportSample = -1;
    int lastTransportBlockSize = 0;

    // Meter-only 2 second automatic GR peak hold. This does not affect audio,
    // detector gain, Match, or any parameter state.
    float gainReductionHoldDb[2] { 0.0f, 0.0f };
    int64_t gainReductionHoldSamplesRemaining[2] { 0, 0 };
    int64_t gainReductionHoldDurationSamples = 1;
    int gainReductionHoldMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QQSuperCompressionAudioProcessor)
};
