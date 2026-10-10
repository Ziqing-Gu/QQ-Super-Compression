#pragma once
#include "ABTransfer.h"
#include "OutputCeiling.h"

#include <JuceHeader.h>
#include <array>
#include <cstdint>
#include <memory>
#include <limits>
#include <vector>
#include "Parameters.h"
#include "StaticCompressionEngine.h"
#include "MeterState.h"
#include "BS1770LoudnessMatch.h"
#include "IntegratedLoudnessMeter.h"

class QQSuperCompressionAudioProcessor final : public juce::AudioProcessor,
                                                private juce::AudioProcessorValueTreeState::Listener,
                                                private juce::Timer
{
public:
    explicit QQSuperCompressionAudioProcessor(std::unique_ptr<juce::PropertiesFile> initialPreferencesOverride = {});
    ~QQSuperCompressionAudioProcessor() override;

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
    // Instance workflow state, saved with the project and independent of A/B.
    // FULL keeps background analysis running; ECO sleeps this instance's
    // UI-only analysis whenever its editor is closed. New instances load the
    // last explicit user choice (first-run fallback FULL); project state wins.
    bool isEcoMode() const noexcept { return ecoMode.load (std::memory_order_acquire); }
    void setEcoMode (bool eco);
    void setEditorOpen (bool open) noexcept { editorOpen.store (open, std::memory_order_release); }
    bool isEditorOpen() const noexcept { return editorOpen.load (std::memory_order_acquire); }
    bool shouldRunUiAnalysis() const noexcept { return ! isEcoMode() || isEditorOpen(); }
    juce::String getPerformanceDiagnostics() const;
    // Effective Ceiling OS promotes TP+1x to 8x without rewriting the stored
    // user choice. UI and latency reporting read the same effective setting.
    int getEffectiveCeilingOversamplingChoice() const noexcept;
    uint64_t getDisplayProjectionRevision() const noexcept
    { return displayProjectionRevision.load (std::memory_order_relaxed); }
    void resetTruePeakHold() noexcept;
    juce::UndoManager& getUndoManager() noexcept { return undoManager; }

    // UI/state changes can tell the host about the combined Lookahead +
    // Oversampling filter latency immediately, even while transport is stopped.
    // The audio-thread configuration follows from the same APVTS parameters on
    // the next process block.
    void notifyHostProcessingLatency();
    void setLookaheadFromEditor(float ms);
    float getDisplayCoreDelaySeconds() const noexcept { return displayCoreDelaySeconds.load(std::memory_order_acquire); }

    // Canonical pair reads include collision pushes immediately, including host
    // automation on the audio thread. Domain order: ST, L, R, M, S.
    float getBoundaryForDomainDb (bool dual, bool upper, int domain) const noexcept;
    float getDynamicsGainForDomain (float detectorLevel, int domain) const noexcept;
    void fillDynamicsGainForDomain (const float* detectorLevels, float* gains,
                                    size_t count, int domain) const noexcept;
    // UI-only explicit edit, including when JUCE's raw value has not caught up
    // with a pushed companion yet. Caller owns the surrounding host gesture.
    void setBoundaryForDomainDb (bool dual, bool upper, int domain, float value);
    void initialiseDualRatioLinkPreference (bool enabled);
    void initialiseInputOutputLinkPreference (bool enabled);
    void initialiseAlgorithmPreference (int algorithm);
    void initialiseDualAlgorithmPreferences (int up, int down);
    bool isLimiterMode() const noexcept { return apvts.getRawParameterValue (qqsc::params::limiterMode)->load() >= 0.5f; }
    // Selection is saved with the sound; the output guard only acts in Limiter.
    bool isTruePeakSelected() const noexcept { return apvts.getRawParameterValue (qqsc::params::truePeakLimiting)->load() >= 0.5f; }
    int getTpRecoveryMode() const noexcept
    {
        return juce::jlimit(int(qqsc::params::tpTight),int(qqsc::params::tpSmooth),
            juce::roundToInt(apvts.getRawParameterValue(qqsc::params::tpRecoveryMode)->load()));
    }
    bool isUnityMonitorEnabled() const noexcept { return unityMonitor.load(std::memory_order_relaxed); }
    bool isUnityMonitorActive() const noexcept { return isLimiterMode() && isUnityMonitorEnabled(); }
    void setUnityMonitorEnabled(bool enabled);
    float getUnityMonitorCeilingDb() const noexcept;
    bool isLimiterLinked() const noexcept { return isLimiterMode() && apvts.getRawParameterValue (qqsc::params::limiterLink)->load() >= 0.5f; }
    const char* soundParameterID (const char*) const noexcept;
    float readSoundParameter (const char*) const noexcept;
    void enterLimiterMode();
    void leaveLimiterMode();
    // Message-thread UI entry: publish coupled boundary changes and record
    // the mode switch as one undo transaction before controls are rebound.
    void setCompressionModeFromEditor (int mode);
    float getBoundaryForBankDb (int bank, bool upper, int domain) const noexcept;
    float getActiveOutputGainDb() const noexcept;
    // Fixed-level reference retained for MATCH/calibration helpers; Limiter UI LINK is strict 1:1 dB.
    float getLimiterReferencePeakDb (float downThresholdShift = 0.0f, bool includeMix = true) const noexcept;
    float getLimiterLinkReferencePeakDb(float shift = 0.0f) const noexcept
    { return getLimiterReferencePeakDb(shift, true); }
    bool isRestoringSoundState() const noexcept
    { return restoringDynamicsState.load(std::memory_order_acquire); }
    float effectiveSingleRatio (size_t domain) const noexcept;
    bool isClassicAlgorithm() const noexcept { return readSoundParameter(qqsc::params::algorithmMode) < 0.5f; }
    bool isClassicBoundary(bool upper) const noexcept
    { return readSoundParameter(qqsc::params::compressionMode)>=0.5f
        ? readSoundParameter(upper ? "downAlgorithmMode" : "upAlgorithmMode")<0.5f : isClassicAlgorithm(); }
    float boundaryMinimumDb(bool upper) const noexcept
    {
        if (isLimiterMode()) return qqsc::params::limiterThresholdMinimumDb;
        return isClassicBoundary(upper)
            ? qqsc::classicThresholdMinimumDb : qqsc::params::thresholdOffDb;
    }


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

    // Display-only raw detector history. The audio thread writes the selected
    // INT/EXT key before Input/Key Gain and before the Side Chain HPF into a
    // bounded lock-free ring while the editor is open. DynamicDisplay copies a
    // stable snapshot on its worker thread only when an HPF gesture ends, then
    // replays the filter and detector without touching the audible DSP path.
    struct DisplayKeyHistoryPosition
    {
        uint64_t generation = 0;
        uint64_t counter = 0;
        double sampleRate = 44100.0;
    };

    struct DisplayKeyHistorySnapshot
    {
        uint64_t generation = 0;
        uint64_t firstCounter = 0;
        double sampleRate = 44100.0;
        int keySource = qqsc::params::keyInternal;
        bool stereoKey = false;
        std::vector<float> left;
        std::vector<float> right;
    };

    void setDisplayKeyHistoryCaptureEnabled (bool enabled);
    DisplayKeyHistoryPosition getDisplayKeyHistoryPosition() const noexcept;
    bool copyDisplayKeyHistory (uint64_t generation, uint64_t startCounter,
                                uint64_t endCounter,
                                DisplayKeyHistorySnapshot& destination) const;

    // UI A/B comparison. A/B stores the complete user sound-setting state
    // (Input/Output Gain, all Ratio/Threshold/Makeup/Mix domain values, Lookahead, Oversampling and Mode). Bypass is intentionally global
    // and is not part of A/B snapshots.
    int getActiveABSlot() const noexcept { return activeABSlot.load (std::memory_order_relaxed); }
    void selectABSlot (int slot);
    void copyAToB();
    void copyBToA();

    // K-weighted loudness Match with relative gating, without the -70 LUFS
    // absolute gate (not a strict EBU R128 integrated meter). During
    // host playback the processor measures Dry vs compressed Wet pre-Makeup in
    // ST, LR and MS domains simultaneously. Match writes the relevant Makeup.
    // Message-thread/offline-driver service; never call from the audio callback.
    void refreshMatchResults();
    bool hasMatchData() const noexcept;
    bool applyMatchForCurrentMode();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout (const std::atomic<bool>* classicForText = nullptr);

private:
    friend struct QQSCReviewCheck;
    struct DisplayKeyHistoryStorage;
    void parameterChanged (const juce::String&, float) override;
    void timerCallback() override;
    void rebuildBoundaryPairs() noexcept;
    void continueLimiterCompressionMode (bool destinationIsDual) noexcept;
    void writeCanonicalBoundariesTo (juce::ValueTree&) const;

    // Packed float pairs allow one atomic transition for both boundaries, with
    // no allocations, locks or recursive host calls on an automation callback.
    std::array<std::atomic<uint64_t>, 20> boundaryPairs {};
    // Every host/UI parameter change advances one cheap revision. DynamicDisplay
    // uses it to invalidate retrospective projection caches without polling and
    // re-rendering the full history unconditionally at 60 Hz.
    std::atomic<uint64_t> displayProjectionRevision { 1 };
    // Publish a new Limiter Single/Dual selection only after its destination
    // boundaries are ready. Host automation never waits for an editor/timer.
    std::atomic<int> limiterCompressionModeForAudio { 0 };
    std::atomic<bool> restoringDynamicsState { false };
    std::atomic<bool> limiterBankInitialised { false };
    std::atomic<bool> dualRatioLinkPreferenceInitialised { false };
    std::atomic<bool> inputOutputLinkPreferenceInitialised { false };
    std::atomic<bool> algorithmPreferenceInitialised { false };
    std::array<std::atomic<bool>,2> dualAlgorithmPreferenceInitialised {};


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
        float ratio = 1.0f;
        float ratioL = 1.0f;
        float ratioR = 1.0f;
        float ratioM = 1.0f;
        float ratioS = 1.0f;
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
        std::array<float,35> limiterSound {};
        std::array<float,33> limiterSettings {};
        bool limiterInitialised = false;
        bool domainLink = true, dualRatioLink = true, inputOutputLink = true;
        bool limiterMode = false, limiterLink = true, truePeakLimiting = true;
        int tpRecoveryMode = qqsc::params::tpAuto;
        int ceilingOversampling = qqsc::params::ceiling8x;
        float ceilingDb = 0.0f, limiterOutputDb = 0.0f, limiterCalibrationDb = 0.0f;
        float lookaheadMs = 26.0f;
        float limiterStereoLink = 0.0f;
        float distort = 0.0f;
        float detectorWindowMs = 100.0f; // Effective value is capped by the active Lookahead.
        int detectorMode = 0;
        int oversampling = qqsc::params::os8x;
        int mode = qqsc::params::stereoLinked;
        int keySource = qqsc::params::keyInternal;
        float keyGainDb = 0.0f;
        float keyHpfHz = qqsc::params::keyHpfOffHz;
        int algorithmMode = qqsc::params::classicAlgorithm;
        int upAlgorithmMode = 0, downAlgorithmMode = 0;
        int compressionMode = qqsc::params::singleCompression;
        std::array<float, 5> range { qqsc::params::rangeOffDb, qqsc::params::rangeOffDb, qqsc::params::rangeOffDb, qqsc::params::rangeOffDb, qqsc::params::rangeOffDb };
        std::array<float, 5> upThreshold { -120, -120, -120, -120, -120 };
        std::array<float, 5> downThreshold { 0, 0, 0, 0, 0 };
        std::array<float, 5> upRatio { 1, 1, 1, 1, 1 };
        std::array<float, 5> downRatio { 1, 1, 1, 1, 1 };
        std::array<bool, 5> upEnabled { true, true, true, true, true };
        std::array<bool, 5> downEnabled { true, true, true, true, true };
    };

    void processBlockInternal (juce::AudioBuffer<float>&, bool forceBypass);

    float effectiveDualRatio (size_t domain, bool upward) const noexcept;

    ParameterSnapshot captureCurrentSnapshot() const noexcept;
    static void migrateLimiterStereoSnapshot(ParameterSnapshot&) noexcept;
    static ParameterSnapshot activeSnapshot(ParameterSnapshot) noexcept;
    static qqsc::ABTransfer makeABTransfer (const ParameterSnapshot&) noexcept;
    void queueABTransfer (const ParameterSnapshot&, const ParameterSnapshot&);
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
    void resumeFromEcoTransportStop(bool forceBypass) noexcept;
    juce::dsp::Oversampling<float>& getCurrentOversampler() noexcept;
    int getOversamplingLatencySamples (int oversamplingIndex) const noexcept;
    int getCombinedLatencySamples (float requestedLookaheadMs, int oversamplingIndex) const noexcept;
    bool shouldShareCeilingOversampling (int coreFactor, int ceilingChoice, int lookaheadSamplesBase) const noexcept;
    int getCurrentCeilingAdditionalLatency() const noexcept;

    void resetMatchAccumulator() noexcept;
    void updateMatchResults() noexcept;

    void resetGainReductionHold (int mode = -1) noexcept;
    void updateGainReductionHoldChannel (int channel, float blockPeakGrDb, int blockSamples) noexcept;
    std::shared_ptr<DisplayKeyHistoryStorage> createDisplayKeyHistoryStorage();

    juce::UndoManager undoManager;
    // Constructed before APVTS; host text callbacks never access a partly
    // constructed parameter tree, and never take locks on the audio thread.
    std::atomic<bool> classicAlgorithmForText { true };
    juce::AudioProcessorValueTreeState apvts;

    // All four future-window peak analysers run continuously so ST/MS/LR
    // switching uses the same transparent detector semantics in every domain.
    // ST derives its linked gain from the current L/R window levels.
    qqsc::StaticCompressionEngine leftEngine;
    qqsc::StaticCompressionEngine rightEngine;
    qqsc::StaticCompressionEngine midEngine;
    qqsc::StaticCompressionEngine sideEngine;

    qqsc::MeterState meterState;
    juce::dsp::Oversampling<float> truePeakOversampler { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, false };
    juce::AudioBuffer<float> truePeakBuffer;
    qqsc::OutputCeiling outputCeiling;
    std::vector<std::array<float,10>> ceilingReferenceDelay;
    size_t ceilingReferenceIndex=0;
    int ceilingReferenceDelaySamples=0;
    bool currentCeilingSharesCoreOversampling=false;
    int currentCeilingOversamplingChoice=qqsc::params::ceiling8x;
    float truePeakHold = -120.0f;
    int64_t truePeakRemaining = 0;
    std::atomic<bool> truePeakResetRequested { false };
    void updateTruePeakMeter (int numSamples);
    void updateTruePeakHold (float db, int numSamples);
    std::atomic<bool> unityMonitor { false };
    juce::SmoothedValue<float> unityMonitorFade, unityOutputSmoother, bypassFade;
    bool bypassInitialised=false;
    int unityMatchSettling=0;

    // 0.1.10: Oversampling is deliberately a Lookahead=0 ms-only option.
    // User-facing choices are 1x/4x/8x/16x; 10 ms and longer always use the 1x
    // path regardless of the stored 0 ms choice. All four paths are created
    // ahead of time so switching never constructs filters on the audio thread.
    // Index 0 is JUCE's dummy 1x stage; indices 1/2/3 are 4x/8x/16x high-quality
    // linear-phase FIR stages with integer latency compensation enabled.
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 4> oversamplers;
    juce::AudioBuffer<float> wetBaseBuffer;
    // Six-channel staging keeps main L/R and optional Key L/R explicit before
    // the shared 1x/4x/8x/16x Oversampling stage. Key channels are overwritten by
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
    int currentDetectorWindowSamplesInternal = -1;
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
    bool currentLimiterCeilingActive = false;
    int configuredMaximumBlockSize = 1;
    double currentSampleRate = 44100.0;
    std::atomic<bool> ecoMode { false };
    std::atomic<bool> editorOpen { false };
    bool previousUiAnalysisEnabled = true;
    // FULL also admits sub-audible residuals, guarded by raw level, gain,
    // measured output and settled tails. ECO suspends on a known host Stop.
    bool ecoTransportSuspended = false;
    static constexpr float fullIdleRawFloor = 1.e-9f; // -180 dBFS, stopped FULL only.
    static constexpr float fullIdleOutputFloor = 1.e-8f; // -160 dBFS, includes gain/monitor safety.
    double fullIdleGainBound() const noexcept;
    int64_t stoppedQuietOutputSamples = 0;
    int64_t stoppedSilentSamples = 0;
    uint64_t stoppedSilenceRevision = 0;
    bool stoppedDspSleeping = false;
    uint64_t stoppedFastPathBlocks = 0;
    struct IdleInputDiagnostics
    {
        float mainPeak=0.0f, externalPeak=0.0f;
        uint64_t nonFiniteSamples=0;
        bool externalSelected=false;
        int externalChannels=0;
    };
    static float measureInputPeak(const juce::AudioBuffer<float>&, int channels, uint64_t& nonFinite) noexcept;
    struct PerformanceWindow
    {
        std::atomic<uint64_t> blocks {0}, ticks {0}, sleepBlocks {0}, zeroBlocks {0}, playingBlocks {0}, unknownBlocks {0};
        std::atomic<int> blockSize {0}, coreFactor {1}, ceilingFactor {1}, idleGate {0};
        std::atomic<uint64_t> stoppedInputBlocks {0}, nonFiniteSamples {0};
        std::atomic<float> mainPeakMin {0}, mainPeakMax {0}, externalPeakMin {0}, externalPeakMax {0};
        std::atomic<int> selectedKey {0}, externalChannels {0};
    };
    // Last uninterrupted visit to each FULL/ECO x editor open/closed state.
    // Closed-window measurements remain readable after the editor reopens.
    std::array<PerformanceWindow, 4> performanceWindows;
    int performancePhase = -1;
    void recordPerformanceBlock(int phase, int64_t start, int samples, bool zero,
                                bool playing, bool known, bool sleeping, int idleGate, const IdleInputDiagnostics&) noexcept;
    std::atomic<bool> latencyRefreshPending { false };

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
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 5> upRatioSmoothers;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 5> downRatioSmoothers;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 5> upEnableFades, downEnableFades;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> algorithmFade; // 0 Classic, 1 Super
    juce::SmoothedValue<float> limiterStereoLinkSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> detectorModeFade; // 0 Original, 1 Bilateral
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> upAlgorithmFade, downAlgorithmFade;
    juce::AudioBuffer<float> mixControlBuffer; // five Makeup, five Mix, Output, four Dry matrix coefficients
    juce::SpinLock abTransferLock;
    std::atomic<bool> abTransferPending { false };
    qqsc::ABTransfer requestedABFrom, requestedABTo, abFrom, abTo;
    qqsc::ABTransfer::Matrix abFrozen {1,0,0,1}, abLastMatrix {1,0,0,1};
    qqsc::ABTransfer::Matrix abFrozenDry {}, abLastDryMatrix {};
    float abFrozenUnityOutput=1,abLastUnityOutput=1;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> abFade;
    bool abActive = false, abFromFrozen = false;
    bool requestedABCompatible = true;
    int abTailSamples = 0;
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

    // The shared_ptr is exchanged atomically once per audio block, so opening
    // or closing the editor never frees a ring that the audio/Display worker is
    // still using. Slots contain packed atomic L/R floats; no audio-thread lock
    // or allocation is involved. At high host rates the display-only stream is
    // boxcar-decimated to at most 48 kHz before it enters this ring.
    std::shared_ptr<DisplayKeyHistoryStorage> displayKeyHistoryStorage;
    std::atomic<bool> displayKeyHistoryCaptureEnabled { false };
    std::atomic<double> displayKeyHistoryHostSampleRate { 44100.0 };
    std::atomic<uint64_t> displayKeyHistoryGenerationCounter { 0 };

    mutable juce::CriticalSection abLock;
    std::atomic<float> distortForAudio { 0.0f };
    std::array<std::atomic<float>,2> observedLookahead {{26.0f,26.0f}};
    std::atomic<float> displayCoreDelaySeconds {0.026f};
    ParameterSnapshot snapshotA;
    ParameterSnapshot snapshotB;
    std::atomic<int> activeABSlot { 0 };
    std::atomic<uint64_t> matchGeneration { 0 }, matchPublishedGeneration { 0 };

    // v1.0.3: final audible-only centered monitor state. Keep LR and MS
    // selections independent so changing processing mode never silently maps
    // an L solo into M (or R into S). Defaults are ALL.
    std::atomic<int> monitorLRSelection { qqsc::params::monitorAll };
    std::atomic<int> monitorMSSelection { qqsc::params::monitorAll };
    std::atomic<bool> sidechainListen { false };

    qqsc::IntegratedLoudnessMeter outputLoudness;
    bool lufsWasMeasuring = false;
    qqsc::BS1770LoudnessMatch loudnessMatch;
    qqsc::BS1770LoudnessMatch unityMixMatch;
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
    std::atomic<bool> applyingCumulativeMatch { false };
    std::atomic<bool> matchPassComplete { true };
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
