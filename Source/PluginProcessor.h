#pragma once

#include <JuceHeader.h>
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

    // UI/state changes can tell the host about the new PDC value immediately,
    // even while transport is stopped. The audio-thread configuration follows
    // from the same APVTS parameter on the next process block.
    void notifyHostLookaheadLatency (float lookaheadMs);

    // UI A/B comparison. A/B stores the complete user sound-setting state
    // (Ratio, all Makeup values, Mix, Lookahead and Mode). Bypass is intentionally global
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
        int mode = qqsc::params::stereoLinked;
    };

    void processBlockInternal (juce::AudioBuffer<float>&, bool forceBypass);

    ParameterSnapshot captureCurrentSnapshot() const noexcept;
    void applySnapshot (const ParameterSnapshot&);
    void refreshActiveSnapshot();
    void writeABStateTo (juce::ValueTree& state);
    void readABStateFrom (const juce::ValueTree& state);
    void setActualParameterValue (const char* parameterID, float actualValue);

    void updateLookaheadConfiguration (bool force = false);
    void resetLookaheadState() noexcept;

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

    // The audio path is delayed by the exact user Lookahead amount. The detector
    // receives current input while gain is applied to the corresponding delayed
    // sample, so each delayed sample can use its future [n..n+Lookahead] window.
    juce::AudioBuffer<float> lookaheadDelayBuffer;
    double currentSampleRate = 44100.0;
    int delayCapacity = 1;
    int delayWriteIndex = 0;
    int maxLookaheadSamples = 0;
    int currentLookaheadSamples = -1;
    int64_t detectorSampleCounter = 0;

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
