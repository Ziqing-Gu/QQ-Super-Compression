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

    // UI A/B comparison. A/B stores the complete user sound-setting state
    // (Ratio, all Makeup values, Mix, Lookahead, Oversampling and Mode). Bypass is intentionally global
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
    struct ParameterSnapshot
    {
        float ratio = 8.0f;
        float makeupST = 0.0f;
        float makeupL = 0.0f;
        float makeupR = 0.0f;
        float makeupM = 0.0f;
        float makeupS = 0.0f;
        float mix = 100.0f;
        float lookaheadMs = 26.0f;
        int oversampling = 1;
        int mode = qqsc::params::stereoLinked;
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
    void resetDryDelayState() noexcept;
    void resetAllProcessingState() noexcept;
    juce::dsp::Oversampling<float>& getCurrentOversampler() noexcept;
    int getOversamplingLatencySamples (int oversamplingIndex) const noexcept;
    int getCombinedLatencySamples (float lookaheadMs, int oversamplingIndex) const noexcept;

    void resetMatchAccumulator() noexcept;
    void updateMatchResults() noexcept;

    void resetGainReductionHold (int mode = -1) noexcept;
    void updateGainReductionHoldChannel (int channel, float blockPeakGrDb, int blockSamples) noexcept;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    // All four domain lookahead peak analysers run continuously so ST/MS/LR
    // switching uses the same future-window definition in every domain.
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

    // The detector and Ratio gain application run in the effective internal
    // domain. Only 0 ms may be 8x/16x; every non-zero Lookahead is forced 1x.
    // Thus the oversampled mode never needs a non-zero internal Lookahead delay,
    // while the established 10-100 ms future-window semantics stay host-rate.
    juce::AudioBuffer<float> oversampledLookaheadDelayBuffer;
    int oversampledDelayCapacity = 1;
    int oversampledDelayWriteIndex = 0;
    int maxLookaheadSamplesBase = 0;
    int maxLookaheadSamplesInternal = 0;
    int currentLookaheadSamplesBase = -1;
    int currentLookaheadSamplesInternal = -1;
    int64_t detectorSampleCounter = 0;

    // Dry stays at host sample rate. It is delayed by Lookahead plus the exact
    // integer latency reported by the selected oversampling filter, so Dry, Wet,
    // Mix and Bypass remain sample-aligned.
    juce::AudioBuffer<float> dryDelayBuffer;
    int dryDelayCapacity = 1;
    int dryDelayWriteIndex = 0;
    int currentOversamplingIndex = -1;
    int currentOversamplingFactor = 1;
    int currentTotalLatencySamples = 0;
    int configuredMaximumBlockSize = 1;
    double currentSampleRate = 44100.0;

    // Parameter smoothing only prevents zipper noise while controls move. It is
    // not the compressor's user Attack/Release behaviour.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ratioSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupSTSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupLSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupRSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupMSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> makeupSSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;

    mutable juce::CriticalSection abLock;
    ParameterSnapshot snapshotA;
    ParameterSnapshot snapshotB;
    std::atomic<int> activeABSlot { 0 };

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
