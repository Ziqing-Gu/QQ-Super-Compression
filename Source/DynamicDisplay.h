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
    void beginKeyHpfGesture() noexcept;
    void endKeyHpfGesture();

private:
    class HpfReplayWorker;

    // Sixty display samples per second keeps scrolling and parameter
    // projection fluid while preserving the previous eight-second window.
    static constexpr int displayRefreshHz = 60;
    static constexpr int historyLength = 480;
    static constexpr int gainReductionShadeSegments = 160;

    struct HistoryPoint
    {
        float inputDb = -120.0f;
        float detectorDb = -120.0f;
        float capturedInputGainDb = 0.0f;
        float capturedKeyGainDb = 0.0f;
        float replayedDetectorDb = -120.0f;
        uint64_t captureGeneration = 0;
        uint64_t captureCounter = 0;
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
        std::vector<uint64_t> markers;
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
        size_t size = 0;
    };

    struct RenderCache
    {
        ProjectedHistory projected;
        juce::Path inputPath;
        juce::Path gainReductionPath;
        juce::Path outputPath;
        juce::Path externalKeyPath;
        juce::Path gainReductionShadePath;
        float currentGainReductionDb = 0.0f;
        bool valid = false;
    };

    void timerCallback() override;
    void pushHistory (HistorySet&, HistoryPoint);
    void updatePath (juce::Path&, const std::array<float, historyLength>& values,
                     size_t valueCount, juce::Rectangle<float> plot) const;
    void updateGainReductionShadePath (juce::Path&,
                                       const std::array<float, historyLength>& upper,
                                       const std::array<float, historyLength>& lower,
                                       size_t valueCount, juce::Rectangle<float> plot) const;
    float dbToY (float db, juce::Rectangle<float> plot) const noexcept;
    juce::Rectangle<float> domainPanelBounds (int domainIndex, int mode) const noexcept;
    static juce::Rectangle<float> plotBoundsForPanel (juce::Rectangle<float> panel) noexcept;
    void refreshRenderCaches (int mode);
    void drawDomainPanel (juce::Graphics&, juce::Rectangle<float> panel, int domainIndex,
                          const juce::String& domainName, int mode);
    float thresholdDbForDomain (int domainIndex, int mode) const noexcept;
    float ratioForDomain (int domainIndex, int mode) const noexcept;
    float makeupDbForDomain (int domainIndex, int mode) const noexcept;
    float mixForDomain (int domainIndex, int mode) const noexcept;
    void projectHistory (int domainIndex, int mode, bool externalKey,
                         bool bypassed, ProjectedHistory&) const;
    bool buildHpfReplay (const ReplayRequest&, ReplayResult&, juce::Thread&) const;
    bool requestHpfHistoryRefresh (bool retrying = false);
    void scheduleHpfReplayRetry (uint64_t failedEndCounter);
    void handleHpfReplayFailure (uint64_t requestGeneration, uint64_t failedEndCounter);
    void applyHpfReplay (ReplayResult);
    void clearHistories();

    QQSuperCompressionAudioProcessor& processor;
    std::array<HistorySet, 2> histories;
    std::array<RenderCache, 2> renderCaches;
    std::unique_ptr<HpfReplayWorker> hpfReplayWorker;
    std::shared_ptr<std::atomic<uint64_t>> replayRequestGeneration;
    int lastMode = -1;
    int lastKeySource = -1;
    uint64_t lastCaptureGeneration = 0;
    float lastObservedHpfHz = qqsc::params::keyHpfOffHz;
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
