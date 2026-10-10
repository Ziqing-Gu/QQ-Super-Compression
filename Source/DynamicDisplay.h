#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>
#include <deque>
#include "PluginProcessor.h"

class DynamicDisplay final : public juce::Component,
                             private juce::Timer
{
public:
    explicit DynamicDisplay (QQSuperCompressionAudioProcessor&);
    ~DynamicDisplay() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Rectangle<float> getLoudnessReadoutBounds() const noexcept;
    void beginKeyHpfGesture() noexcept;
    void endKeyHpfGesture();

    // Shared geometry for boundary lines and the adjacent fader thumbs.
    // Domain indices: 0 ST, 1 L, 2 R, 3 M, 4 S. Coordinates are Display-local.
    juce::Rectangle<float> getBoundaryPlotForDomain (int parameterDomainIndex) const noexcept;
    float getBoundaryYForDomainDb (int parameterDomainIndex, float detectorDb, bool upper=false) const noexcept;
    float getBoundaryDbForY (int parameterDomainIndex, float localY, bool upper=false) const noexcept;
    std::function<void()> onScaleChanged;

private:
    friend struct QQSCVisualCheck;
    friend struct QQSCLimiterCheck;
    void drawLoudnessReadout(juce::Graphics&);
    class HpfReplayWorker;

    // Sixty display samples per second keeps scrolling and parameter
    // projection fluid while preserving the previous eight-second window.
    static constexpr int displayRefreshHz = 60;
    static constexpr int historyLength = 480;
    static constexpr float historyWindowSeconds = static_cast<float> (historyLength) / static_cast<float> (displayRefreshHz);
    static constexpr int gainReductionShadeSegments = 160;
    static constexpr int dynamicsLutSize = 4097;

    struct HistoryPoint
    {
        float inputDb = -120.0f;
        float detectorDb = -120.0f;
        float truePeakExcessDb = 0.0f;
        float capturedInputGainDb = 0.0f;
        float capturedKeyGainDb = 0.0f;
        float replayedDetectorDb = -120.0f;
        uint64_t captureGeneration = 0;
        uint64_t captureCounter = 0;
        float carrierDelaySeconds = -1.0f;
        bool hasReplayedDetector = false;
    };

    struct ReplayRequest
    {
        uint64_t requestGeneration = 0;
        uint64_t captureGeneration = 0;
        uint64_t requestedStartCounter = 0;
        int mode = qqsc::params::stereoLinked;
        int keySource = qqsc::params::keyInternal;
        float hpfHz = qqsc::params::keyHpfOffHz;
        float lookaheadMs = 0.0f;
        float detectorWindowMs = 0.0f;
        int detectorMode = 0;
        std::vector<uint64_t> markers;
        std::vector<float> carrierDelays;
        int oversampling = 0;
    };

    struct ReplayResult
    {
        ReplayRequest request;
        std::vector<float> detectorDb0;
        std::vector<float> detectorDb1;
        size_t firstValidMarkerIndex = 0;
    };

    struct HistorySet
    {
        std::deque<HistoryPoint> points;
    };

    struct ProjectedHistory
    {
        std::array<float, historyLength> input {};
        std::array<float, historyLength> gainReductionBoundary {};
        std::array<float, historyLength> output {};
        std::array<float, historyLength> externalKey {};
        std::array<float, historyLength> effectiveGainReduction {};
        std::array<uint64_t, historyLength> captureCounter {};
        size_t size = 0;
    };

    struct RenderCache
    {
        ProjectedHistory projected;
        juce::Path inputPath;
        juce::Path gainReductionPath;
        juce::Path gainIncreasePath;
        juce::Path outputPath;
        juce::Path externalKeyPath;
        juce::Path gainReductionShadePath;
        juce::Path gainIncreaseShadePath;
        float currentGainReductionDb = 0.0f;
        std::array<float, dynamicsLutSize> dynamicsGainLut {};
        uint64_t dynamicsLutSignature = 0;
        bool dynamicsLutValid = false;
        bool dynamicsLutUnity = false;
        bool valid = false;
    };

    void timerCallback() override;
    void pushHistory (HistorySet&, HistoryPoint);
    void updatePath (juce::Path&, const std::array<float, historyLength>& values,
                     const std::array<uint64_t, historyLength>& captureCounters,
                     size_t valueCount, juce::Rectangle<float> plot) const;
    void updateGainChangePaths (juce::Path& reductionPath, juce::Path& increasePath,
                                const ProjectedHistory&, juce::Rectangle<float> plot) const;
    void updateGainReductionShadePath (juce::Path& reductionPath, juce::Path& increasePath,
                                       const std::array<float, historyLength>& upper,
                                       const std::array<float, historyLength>& lower,
                                       const std::array<float, historyLength>& gainReduction,
                                       const std::array<uint64_t, historyLength>& captureCounters,
                                       size_t valueCount, juce::Rectangle<float> plot) const;
    float dbToY (float db, juce::Rectangle<float> plot) const noexcept;
    float historyXForCounter (uint64_t captureCounter, juce::Rectangle<float> plot) const noexcept;
    void trimHistoryToVisibleWindow (HistorySet&, uint64_t referenceCounter) const;
    juce::Rectangle<float> domainPanelBounds (int domainIndex, int mode) const noexcept;
    static juce::Rectangle<float> plotBoundsForPanel (juce::Rectangle<float> panel) noexcept;
    void refreshRenderCaches (int mode);
    uint64_t dynamicsLutSignatureForDomain (int domainIndex, int mode) const noexcept;
    void rebuildDynamicsLut (RenderCache&, int domainIndex, int mode, uint64_t signature);
    float dynamicsGainFromLut (const RenderCache&, float detectorDb) const noexcept;
    void drawDomainPanel (juce::Graphics&, juce::Rectangle<float> panel, int domainIndex,
                          const juce::String& domainName, int mode);
    float thresholdDbForDomain (int domainIndex, int mode) const noexcept;
    float upperBoundaryDbForDomain (int domainIndex, int mode) const noexcept;
    float makeupDbForDomain (int domainIndex, int mode) const noexcept;
    float mixForDomain (int domainIndex, int mode) const noexcept;
    void projectHistory (int domainIndex, int mode, bool externalKey,
                         bool bypassed, const RenderCache&, ProjectedHistory&) const;
    bool buildHpfReplay (const ReplayRequest&, ReplayResult&, juce::Thread&) const;
    bool requestHpfHistoryRefresh (bool retrying = false);
    void scheduleHpfReplayRetry (uint64_t failedEndCounter);
    void handleHpfReplayFailure (uint64_t requestGeneration, uint64_t failedEndCounter);
    void applyHpfReplay (ReplayResult);
    void clearHistories();

    QQSuperCompressionAudioProcessor& processor;
    juce::TextButton scaleButton;
    int lastScaleDb = -1;
    void refreshScale();
    std::array<HistorySet, 2> histories;
    std::array<RenderCache, 2> renderCaches;
    RenderCache stereoLimiterCache; // One ST presentation of independent L/R results.
    std::array<float, dynamicsLutSize> detectorLevelLut {};
    std::unique_ptr<HpfReplayWorker> hpfReplayWorker;
    std::shared_ptr<std::atomic<uint64_t>> replayRequestGeneration;
    int lastMode = -1;
    int lastLimiter = -1;
    int lastKeySource = -1;
    uint64_t lastCaptureGeneration = 0;
    uint64_t lastCapturedHistoryCounter = 0;
    uint64_t historyRevision = 1;
    uint64_t renderedHistoryRevision = 0;
    uint64_t renderedProjectionRevision = 0;
    uint64_t renderReferenceCounter = 0;
    double renderReferenceSampleRate = 44100.0;
    bool geometryDirty = true;
    float lastObservedHpfHz = qqsc::params::keyHpfOffHz;
    float lastObservedLookaheadMs = 26.0f;
    float lastObservedDetectorWindowMs = 26.0f;
    int lastObservedDetectorMode = 0;
    int lastObservedOversampling = -1;
    int hpfStableTimerTicks = 0;
    int hpfRetryTimerTicks = 0;
    int hpfRetryAttempts = 0;
    uint64_t hpfRetryAfterCounter = 0;
    bool keyHpfGestureActive = false;
    bool hpfRefreshPending = false;
    bool hpfReplayRetryPending = false;
    bool hpfReplayBusy = false;
    // Four 60 Hz ticks retain the v1.1.4 two-tick/30 Hz debounce duration.
    static constexpr int hpfAutomationStableTicks = 4;
    static constexpr int hpfRetryDelayTicks = 4;
    static constexpr int hpfMaxRetryAttempts = 3;
    static constexpr uint64_t hpfReplayPreRollSamples = 48000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DynamicDisplay)
};
