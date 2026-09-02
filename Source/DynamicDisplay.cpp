#include "DynamicDisplay.h"
#include "Parameters.h"
#include "UTF8LookAndFeel.h"
#include <algorithm>
#include <optional>

namespace
{
constexpr float minDb = -90.0f;
constexpr float maxDb = 0.0f;
constexpr float silenceDb = -120.0f;

float dbToDetectorLevel (float db) noexcept
{
    return db <= silenceDb + 0.1f ? 0.0f
                                  : juce::Decibels::decibelsToGain (juce::jmin (0.0f, db));
}

float readParameter (QQSuperCompressionAudioProcessor& processor, const char* parameterID,
                     float fallback = 0.0f) noexcept
{
    if (auto* value = processor.getAPVTS().getRawParameterValue (parameterID))
        return value->load();

    return fallback;
}

juce::String grText (float db)
{
    return juce::String (juce::jmax (0.0f, db), 1) + " dB";
}

struct ReplayHighPassCoefficients
{
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
};

struct ReplayHighPassState
{
    float z1 = 0.0f;
    float z2 = 0.0f;

    float process (float input, const ReplayHighPassCoefficients& c) noexcept
    {
        const auto output = c.b0 * input + z1;
        z1 = c.b1 * input - c.a1 * output + z2;
        z2 = c.b2 * input - c.a2 * output;
        return output;
    }
};

class ReplayPeakWindow
{
public:
    explicit ReplayPeakWindow (int lookaheadSamplesIn)
        : lookaheadSamples (juce::jmax (0, lookaheadSamplesIn))
    {
    }

    void process (float sample, int64_t sampleIndex)
    {
        const auto magnitude = std::abs (sample);
        while (! queue.empty() && queue.back().second <= magnitude)
            queue.pop_back();

        queue.emplace_back (sampleIndex, magnitude);
        const auto oldestAllowed = sampleIndex - static_cast<int64_t> (lookaheadSamples);
        while (! queue.empty() && queue.front().first < oldestAllowed)
            queue.pop_front();

        currentLevel = queue.empty() ? magnitude : queue.front().second;
    }

    float getCurrentLevel() const noexcept { return currentLevel; }

private:
    int lookaheadSamples = 0;
    std::deque<std::pair<int64_t, float>> queue;
    float currentLevel = 0.0f;
};

ReplayHighPassCoefficients replayHighPassCoefficients (double sampleRate, float cutoffHz) noexcept
{
    ReplayHighPassCoefficients result;
    const auto safeMaximum = juce::jmax (0.1, sampleRate * 0.45);
    const auto cutoff = juce::jlimit (0.1, safeMaximum,
                                      static_cast<double> (qqsc::params::clampKeyHpfHz (cutoffHz)));
    const auto omega = 2.0 * juce::MathConstants<double>::pi * cutoff / sampleRate;
    const auto sine = std::sin (omega);
    const auto cosine = std::cos (omega);
    constexpr double butterworthQ = 0.70710678118654752440;
    const auto alpha = sine / (2.0 * butterworthQ);
    const auto a0 = 1.0 + alpha;

    result.b0 = static_cast<float> (((1.0 + cosine) * 0.5) / a0);
    result.b1 = static_cast<float> (-(1.0 + cosine) / a0);
    result.b2 = result.b0;
    result.a1 = static_cast<float> ((-2.0 * cosine) / a0);
    result.a2 = static_cast<float> ((1.0 - alpha) / a0);
    return result;
}

float detectorLevelToDb (float level) noexcept
{
    return juce::jlimit (silenceDb, 24.0f,
                         juce::Decibels::gainToDecibels (juce::jmax (0.0f, level), silenceDb));
}
}

class DynamicDisplay::HpfReplayWorker final : public juce::Thread
{
public:
    explicit HpfReplayWorker (DynamicDisplay& ownerIn)
        : juce::Thread ("QQSC Display HPF Replay"), owner (ownerIn)
    {
    }

    void submit (ReplayRequest request)
    {
        {
            const juce::ScopedLock lock (requestLock);
            pendingRequest = std::move (request);
        }
        notify();
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            wait (-1);
            if (threadShouldExit())
                break;

            std::optional<ReplayRequest> request;
            {
                const juce::ScopedLock lock (requestLock);
                request.swap (pendingRequest);
            }

            if (! request.has_value())
                continue;

            ReplayResult result;
            juce::Component::SafePointer<DynamicDisplay> safeOwner (&owner);
            if (! owner.buildHpfReplay (*request, result, *this))
            {
                const auto failedGeneration = request->requestGeneration;
                const auto failedEndCounter = request->markers.empty() ? 0 : request->markers.back();
                juce::MessageManager::callAsync ([safeOwner, failedGeneration, failedEndCounter]
                {
                    if (safeOwner != nullptr)
                        safeOwner->handleHpfReplayFailure (failedGeneration, failedEndCounter);
                });
                continue;
            }

            juce::MessageManager::callAsync ([safeOwner, completed = std::move (result)] () mutable
            {
                if (safeOwner != nullptr)
                    safeOwner->applyHpfReplay (std::move (completed));
            });
        }
    }

private:
    DynamicDisplay& owner;
    juce::CriticalSection requestLock;
    std::optional<ReplayRequest> pendingRequest;
};

DynamicDisplay::DynamicDisplay (QQSuperCompressionAudioProcessor& p)
    : processor (p),
      hpfReplayWorker (std::make_unique<HpfReplayWorker> (*this)),
      replayRequestGeneration (std::make_shared<std::atomic<uint64_t>> (0))
{
    setOpaque (true);
    for (auto& cache : renderCaches)
    {
        cache.inputPath.preallocateSpace (historyLength * 3);
        cache.gainReductionPath.preallocateSpace (historyLength * 3);
        cache.outputPath.preallocateSpace (historyLength * 3);
        cache.externalKeyPath.preallocateSpace (historyLength * 3);
        cache.gainReductionShadePath.preallocateSpace (gainReductionShadeSegments * 6);
    }

    processor.setDisplayKeyHistoryCaptureEnabled (true);
    lastObservedHpfHz = readParameter (processor, qqsc::params::keyHpfHz,
                                       qqsc::params::keyHpfOffHz);
    hpfReplayWorker->startThread (juce::Thread::Priority::low);
    startTimerHz (displayRefreshHz);
}

DynamicDisplay::~DynamicDisplay()
{
    stopTimer();
    replayRequestGeneration->fetch_add (1, std::memory_order_relaxed);
    if (hpfReplayWorker != nullptr)
    {
        hpfReplayWorker->signalThreadShouldExit();
        hpfReplayWorker->notify();
        hpfReplayWorker->stopThread (2000);
        hpfReplayWorker.reset();
    }
    processor.setDisplayKeyHistoryCaptureEnabled (false);
}

void DynamicDisplay::resized()
{
    const auto mode = processor.getMeterState().processingMode.load (std::memory_order_relaxed);
    refreshRenderCaches (mode);
}

void DynamicDisplay::beginKeyHpfGesture() noexcept
{
    keyHpfGestureActive = true;
}

void DynamicDisplay::endKeyHpfGesture()
{
    keyHpfGestureActive = false;
    hpfRefreshPending = false;
    hpfStableTimerTicks = 0;
    lastObservedHpfHz = readParameter (processor, qqsc::params::keyHpfHz,
                                       qqsc::params::keyHpfOffHz);
    requestHpfHistoryRefresh();
}

void DynamicDisplay::clearHistories()
{
    replayRequestGeneration->fetch_add (1, std::memory_order_relaxed);
    for (auto& h : histories)
        h.points.clear();

    for (auto& cache : renderCaches)
    {
        cache.projected.size = 0;
        cache.inputPath.clear();
        cache.gainReductionPath.clear();
        cache.outputPath.clear();
        cache.externalKeyPath.clear();
        cache.gainReductionShadePath.clear();
        cache.currentGainReductionDb = 0.0f;
        cache.valid = false;
    }

    hpfReplayRetryPending = false;
    hpfReplayBusy = false;
    hpfRetryTimerTicks = 0;
    hpfRetryAttempts = 0;
}

void DynamicDisplay::timerCallback()
{
    auto& m = processor.getMeterState();
    const auto mode = m.processingMode.load (std::memory_order_relaxed);
    const auto position = processor.getDisplayKeyHistoryPosition();
    const auto keySource = juce::jlimit (
        static_cast<int> (qqsc::params::keyInternal),
        static_cast<int> (qqsc::params::keyExternal),
        juce::roundToInt (readParameter (processor, qqsc::params::keySource)));
    const auto capturedInputGainDb = readParameter (processor, qqsc::params::inputGainDb);
    const auto capturedKeyGainDb = readParameter (processor, qqsc::params::keyGainDb);

    if (mode != lastMode || keySource != lastKeySource
        || position.generation != lastCaptureGeneration)
    {
        clearHistories();
        lastMode = mode;
        lastKeySource = keySource;
        lastCaptureGeneration = position.generation;
    }

    HistoryPoint point0;
    point0.inputDb = m.displayInputDb0.load (std::memory_order_relaxed);
    point0.detectorDb = m.displayDetectorDb0.load (std::memory_order_relaxed);
    point0.capturedInputGainDb = capturedInputGainDb;
    point0.capturedKeyGainDb = capturedKeyGainDb;
    point0.captureGeneration = position.generation;
    point0.captureCounter = position.counter;
    pushHistory (histories[0], point0);

    if (mode != qqsc::params::stereoLinked)
    {
        HistoryPoint point1;
        point1.inputDb = m.displayInputDb1.load (std::memory_order_relaxed);
        point1.detectorDb = m.displayDetectorDb1.load (std::memory_order_relaxed);
        point1.capturedInputGainDb = capturedInputGainDb;
        point1.capturedKeyGainDb = capturedKeyGainDb;
        point1.captureGeneration = position.generation;
        point1.captureCounter = position.counter;
        pushHistory (histories[1], point1);
    }

    const auto currentHpfHz = readParameter (processor, qqsc::params::keyHpfHz,
                                             qqsc::params::keyHpfOffHz);
    if (std::abs (currentHpfHz - lastObservedHpfHz) > 0.01f)
    {
        lastObservedHpfHz = currentHpfHz;
        hpfStableTimerTicks = 0;
        hpfRefreshPending = true;
    }
    else if (hpfRefreshPending && ! keyHpfGestureActive)
    {
        if (++hpfStableTimerTicks >= hpfAutomationStableTicks)
        {
            hpfRefreshPending = false;
            hpfStableTimerTicks = 0;
            requestHpfHistoryRefresh();
        }
    }

    if (hpfReplayRetryPending && ! keyHpfGestureActive && ! hpfRefreshPending)
    {
        if (position.counter > hpfRetryAfterCounter
            || ++hpfRetryTimerTicks >= hpfRetryDelayTicks)
        {
            hpfReplayRetryPending = false;
            hpfRetryTimerTicks = 0;
            requestHpfHistoryRefresh (true);
        }
    }

    refreshRenderCaches (mode);
    repaint();
}

void DynamicDisplay::pushHistory (HistorySet& history, HistoryPoint point)
{
    point.inputDb = juce::jlimit (silenceDb, 24.0f, point.inputDb);
    point.detectorDb = juce::jlimit (silenceDb, maxDb, point.detectorDb);
    history.points.push_back (point);

    while (static_cast<int> (history.points.size()) > historyLength)
        history.points.pop_front();
}

float DynamicDisplay::dbToY (float db, juce::Rectangle<float> plot) const noexcept
{
    return juce::jmap (juce::jlimit (minDb, maxDb, db), maxDb, minDb, plot.getY(), plot.getBottom());
}

void DynamicDisplay::updatePath (juce::Path& path,
                                 const std::array<float, historyLength>& values,
                                 size_t valueCount, juce::Rectangle<float> plot) const
{
    path.clear();
    if (valueCount == 0)
        return;

    const auto denom = static_cast<float> (juce::jmax (1, historyLength - 1));
    const auto startOffset = historyLength - static_cast<int> (valueCount);

    for (size_t i = 0; i < valueCount; ++i)
    {
        const auto xIndex = static_cast<float> (startOffset + static_cast<int> (i));
        const auto x = plot.getX() + plot.getWidth() * xIndex / denom;
        const auto y = dbToY (values[i], plot);

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }
}

void DynamicDisplay::updateGainReductionShadePath (
    juce::Path& path, const std::array<float, historyLength>& upper,
    const std::array<float, historyLength>& lower, size_t valueCount,
    juce::Rectangle<float> plot) const
{
    path.clear();
    if (valueCount == 0)
        return;

    const auto denom = static_cast<float> (juce::jmax (1, historyLength - 1));
    const auto startOffset = historyLength - static_cast<int> (valueCount);
    const auto stride = std::max<size_t> (
        1, (valueCount + static_cast<size_t> (gainReductionShadeSegments) - 1)
               / static_cast<size_t> (gainReductionShadeSegments));

    for (size_t i = 0; i < valueCount; i += stride)
    {
        const auto upperY = dbToY (upper[i], plot);
        const auto lowerY = dbToY (lower[i], plot);
        if (std::abs (lowerY - upperY) < 0.35f)
            continue;

        const auto xIndex = static_cast<float> (startOffset + static_cast<int> (i));
        const auto x = plot.getX() + plot.getWidth() * xIndex / denom;
        path.startNewSubPath (x, upperY);
        path.lineTo (x, lowerY);
    }
}

juce::Rectangle<float> DynamicDisplay::domainPanelBounds (int domainIndex, int mode) const noexcept
{
    auto content = getLocalBounds().toFloat().reduced (10.0f, 6.0f);
    content.removeFromTop (24.0f);
    content.removeFromBottom (23.0f);

    if (mode == qqsc::params::stereoLinked)
        return content;

    constexpr float gap = 6.0f;
    auto top = content.removeFromTop ((content.getHeight() - gap) * 0.5f);
    content.removeFromTop (gap);
    return domainIndex == 0 ? top : content;
}

juce::Rectangle<float> DynamicDisplay::plotBoundsForPanel (juce::Rectangle<float> panel) noexcept
{
    auto plot = panel.reduced (8.0f);
    plot.removeFromTop (22.0f);
    plot.removeFromBottom (3.0f);
    plot.removeFromLeft (44.0f);
    plot.removeFromRight (4.0f);
    return plot;
}

void DynamicDisplay::refreshRenderCaches (int mode)
{
    const auto keySource = juce::roundToInt (readParameter (processor, qqsc::params::keySource));
    const bool externalKey = keySource == qqsc::params::keyExternal;
    const bool bypassed = readParameter (processor, qqsc::params::bypass) >= 0.5f;
    const auto domainCount = mode == qqsc::params::stereoLinked ? 1 : 2;

    for (int domain = 0; domain < static_cast<int> (renderCaches.size()); ++domain)
    {
        auto& cache = renderCaches[static_cast<size_t> (domain)];
        if (domain >= domainCount || getWidth() <= 0 || getHeight() <= 0)
        {
            cache.projected.size = 0;
            cache.inputPath.clear();
            cache.gainReductionPath.clear();
            cache.outputPath.clear();
            cache.externalKeyPath.clear();
            cache.gainReductionShadePath.clear();
            cache.currentGainReductionDb = 0.0f;
            cache.valid = false;
            continue;
        }

        projectHistory (domain, mode, externalKey, bypassed, cache.projected);
        const auto plot = plotBoundsForPanel (domainPanelBounds (domain, mode));
        updatePath (cache.inputPath, cache.projected.input, cache.projected.size, plot);
        updatePath (cache.gainReductionPath, cache.projected.gainReductionBoundary,
                    cache.projected.size, plot);
        updatePath (cache.outputPath, cache.projected.output, cache.projected.size, plot);
        updatePath (cache.externalKeyPath, cache.projected.externalKey, cache.projected.size, plot);
        updateGainReductionShadePath (cache.gainReductionShadePath, cache.projected.input,
                                      cache.projected.gainReductionBoundary,
                                      cache.projected.size, plot);
        cache.currentGainReductionDb = cache.projected.size == 0
            ? 0.0f : cache.projected.effectiveGainReduction[cache.projected.size - 1];
        cache.valid = true;
    }
}

float DynamicDisplay::thresholdDbForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::thresholdDb;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::thresholdLDb : qqsc::params::thresholdRDb;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::thresholdMDb : qqsc::params::thresholdSDb;

    return readParameter (processor, parameterID, qqsc::params::thresholdOffDb);
}

float DynamicDisplay::ratioForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::ratio;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::ratioL : qqsc::params::ratioR;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::ratioM : qqsc::params::ratioS;

    return readParameter (processor, parameterID, 1.0f);
}

float DynamicDisplay::makeupDbForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::makeupGainDb;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::makeupGainLDb : qqsc::params::makeupGainRDb;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::makeupGainMDb : qqsc::params::makeupGainSDb;

    return readParameter (processor, parameterID);
}

float DynamicDisplay::mixForDomain (int domainIndex, int mode) const noexcept
{
    const char* parameterID = qqsc::params::mix;

    if (mode == qqsc::params::leftRight)
        parameterID = domainIndex == 0 ? qqsc::params::mixL : qqsc::params::mixR;
    else if (mode == qqsc::params::midSide)
        parameterID = domainIndex == 0 ? qqsc::params::mixM : qqsc::params::mixS;

    return juce::jlimit (0.0f, 1.0f, readParameter (processor, parameterID, 100.0f) * 0.01f);
}

void DynamicDisplay::projectHistory (int domainIndex, int mode, bool externalKey,
                                     bool bypassed, ProjectedHistory& projected) const
{
    projected.size = 0;
    const auto inputGainDb = readParameter (processor, qqsc::params::inputGainDb);
    const auto keyGainDb = readParameter (processor, qqsc::params::keyGainDb);
    const auto inputGain = juce::Decibels::decibelsToGain (inputGainDb);
    const auto ratio = ratioForDomain (domainIndex, mode);
    const auto thresholdLinear = qqsc::params::thresholdLinear (thresholdDbForDomain (domainIndex, mode));
    const auto wetMix = mixForDomain (domainIndex, mode);
    const auto makeupGain = juce::Decibels::decibelsToGain (makeupDbForDomain (domainIndex, mode));
    const auto outputGain = juce::Decibels::decibelsToGain (
        readParameter (processor, qqsc::params::outputGainDb));

    for (const auto& point : histories[static_cast<size_t> (domainIndex)].points)
    {
        auto detectorDb = point.hasReplayedDetector ? point.replayedDetectorDb
                                                     : point.detectorDb;

        // A replayed point is stored before detector gain, so current Input
        // (INT) or Key Gain (EXT) can move the complete history immediately.
        // Live fallback points remain post-gain and use the capture-time delta.
        if (point.hasReplayedDetector)
            detectorDb += externalKey ? keyGainDb : inputGainDb;
        else if (externalKey)
            detectorDb += keyGainDb - point.capturedKeyGainDb;
        else
            detectorDb += inputGainDb - point.capturedInputGainDb;

        detectorDb = juce::jlimit (silenceDb, maxDb, detectorDb);

        const auto compressedGain = qqsc::StaticCompressionEngine::gainForLevel (
            dbToDetectorLevel (detectorDb), ratio, thresholdLinear);
        const auto effectiveGr = bypassed ? 0.0f
                                          : qqsc::StaticCompressionEngine::effectiveGainReductionDb (
                                                compressedGain, wetMix);

        auto projectedOutputDb = point.inputDb;
        if (! bypassed)
        {
            // Makeup affects only the Wet leg, exactly as in processBlock.
            // It changes Output, but it is intentionally not part of GR.
            const auto mixedGain = 1.0f + (compressedGain * makeupGain - 1.0f) * wetMix;
            const auto totalGain = inputGain * juce::jmax (0.0f, mixedGain) * outputGain;
            projectedOutputDb += juce::Decibels::gainToDecibels (
                juce::jmax (totalGain, 1.0e-9f), -180.0f);
        }

        if (projected.size >= static_cast<size_t> (historyLength))
            break;

        const auto index = projected.size++;
        projected.input[index] = point.inputDb;
        projected.gainReductionBoundary[index] = point.inputDb - effectiveGr;
        projected.output[index] = projectedOutputDb;
        projected.externalKey[index] = detectorDb;
        projected.effectiveGainReduction[index] = effectiveGr;
    }
}

void DynamicDisplay::scheduleHpfReplayRetry (uint64_t failedEndCounter)
{
    if (++hpfRetryAttempts > hpfMaxRetryAttempts)
    {
        hpfReplayRetryPending = false;
        hpfReplayBusy = false;
        repaint();
        return;
    }

    hpfRetryAfterCounter = failedEndCounter;
    hpfRetryTimerTicks = 0;
    hpfReplayRetryPending = true;
    hpfReplayBusy = true;
    repaint();
}

void DynamicDisplay::handleHpfReplayFailure (uint64_t requestGeneration,
                                             uint64_t failedEndCounter)
{
    if (requestGeneration != replayRequestGeneration->load (std::memory_order_relaxed))
        return;

    scheduleHpfReplayRetry (failedEndCounter);
}

bool DynamicDisplay::requestHpfHistoryRefresh (bool retrying)
{
    if (! retrying)
        hpfRetryAttempts = 0;

    if (histories[0].points.empty() || hpfReplayWorker == nullptr)
    {
        hpfReplayBusy = false;
        return false;
    }

    ReplayRequest request;
    request.captureGeneration = histories[0].points.front().captureGeneration;
    request.mode = lastMode;
    request.keySource = lastKeySource;
    request.hpfHz = readParameter (processor, qqsc::params::keyHpfHz,
                                   qqsc::params::keyHpfOffHz);
    request.lookaheadMs = qqsc::params::snapLookaheadMs (
        readParameter (processor, qqsc::params::lookaheadMs));
    request.markers.reserve (histories[0].points.size());

    for (const auto& point : histories[0].points)
    {
        if (point.captureGeneration != request.captureGeneration)
        {
            scheduleHpfReplayRetry (point.captureCounter);
            return false;
        }
        request.markers.push_back (point.captureCounter);
    }

    if (request.captureGeneration == 0 || request.markers.empty())
    {
        hpfReplayBusy = false;
        return false;
    }

    const auto firstMarker = request.markers.front();
    request.requestedStartCounter = firstMarker > hpfReplayPreRollSamples
        ? firstMarker - hpfReplayPreRollSamples : 0;
    request.requestGeneration = replayRequestGeneration->fetch_add (1, std::memory_order_relaxed) + 1;
    hpfReplayRetryPending = false;
    hpfRetryTimerTicks = 0;
    hpfReplayBusy = true;
    repaint();
    hpfReplayWorker->submit (std::move (request));
    return true;
}

bool DynamicDisplay::buildHpfReplay (const ReplayRequest& request, ReplayResult& result,
                                     juce::Thread& worker) const
{
    QQSuperCompressionAudioProcessor::DisplayKeyHistorySnapshot snapshot;
    if (request.markers.empty()
        || ! processor.copyDisplayKeyHistory (request.captureGeneration,
                                              request.requestedStartCounter,
                                              request.markers.back(), snapshot)
        || snapshot.keySource != request.keySource)
        return false;

    const auto currentRequest = replayRequestGeneration->load (std::memory_order_relaxed);
    if (worker.threadShouldExit() || currentRequest != request.requestGeneration)
        return false;

    const auto maxLookaheadSamples = juce::jmax (0, static_cast<int> (
        std::ceil (snapshot.sampleRate * 0.100)));
    const auto lookaheadSamples = juce::jlimit (0, maxLookaheadSamples, static_cast<int> (
        std::round (snapshot.sampleRate * static_cast<double> (request.lookaheadMs) * 0.001)));

    ReplayPeakWindow primaryEngine (lookaheadSamples);
    ReplayPeakWindow secondaryEngine (lookaheadSamples);
    const bool needsSecondaryEngine = request.mode != qqsc::params::stereoLinked
                                      || snapshot.stereoKey;

    const bool hpfEnabled = qqsc::params::isKeyHpfEnabled (request.hpfHz);
    const auto coefficients = replayHighPassCoefficients (snapshot.sampleRate, request.hpfHz);
    ReplayHighPassState filterLeft;
    ReplayHighPassState filterRight;

    result.request = request;
    result.detectorDb0.assign (request.markers.size(), silenceDb);
    result.detectorDb1.assign (request.markers.size(), silenceDb);
    size_t markerIndex = 0;

    while (markerIndex < request.markers.size()
           && request.markers[markerIndex] <= snapshot.firstCounter)
        ++markerIndex;
    result.firstValidMarkerIndex = markerIndex;

    for (size_t i = 0; i < snapshot.left.size(); ++i)
    {
        if ((i & 4095u) == 0u
            && (worker.threadShouldExit()
                || replayRequestGeneration->load (std::memory_order_relaxed) != request.requestGeneration))
            return false;

        auto left = snapshot.left[i];
        auto right = snapshot.stereoKey ? snapshot.right[i] : left;
        if (hpfEnabled)
        {
            left = filterLeft.process (left, coefficients);
            right = snapshot.stereoKey ? filterRight.process (right, coefficients) : left;
        }

        auto analysis0 = left;
        auto analysis1 = right;
        if (request.mode == qqsc::params::midSide)
        {
            analysis0 = snapshot.stereoKey ? 0.5f * (left + right) : left;
            analysis1 = snapshot.stereoKey ? 0.5f * (left - right)
                                           : (request.keySource == qqsc::params::keyExternal ? left : 0.0f);
        }

        const auto sampleIndex = static_cast<int64_t> (i);
        primaryEngine.process (analysis0, sampleIndex);
        if (needsSecondaryEngine)
            secondaryEngine.process (analysis1, sampleIndex);

        const auto completedCounter = snapshot.firstCounter + static_cast<uint64_t> (i) + 1;
        while (markerIndex < request.markers.size()
               && request.markers[markerIndex] <= completedCounter)
        {
            auto level0 = primaryEngine.getCurrentLevel();
            auto level1 = needsSecondaryEngine ? secondaryEngine.getCurrentLevel() : 0.0f;
            if (request.mode == qqsc::params::stereoLinked && snapshot.stereoKey)
                level0 = juce::jmax (level0, level1);

            result.detectorDb0[markerIndex] = detectorLevelToDb (level0);
            result.detectorDb1[markerIndex] = detectorLevelToDb (level1);
            ++markerIndex;
        }
    }

    return markerIndex == request.markers.size();
}

void DynamicDisplay::applyHpfReplay (ReplayResult result)
{
    const auto currentRequest = replayRequestGeneration->load (std::memory_order_relaxed);
    if (result.request.requestGeneration != currentRequest)
        return;

    if (result.request.captureGeneration != lastCaptureGeneration
        || result.request.mode != lastMode
        || result.request.keySource != lastKeySource
        || std::abs (result.request.hpfHz
                     - readParameter (processor, qqsc::params::keyHpfHz,
                                      qqsc::params::keyHpfOffHz)) > 0.01f
        || std::abs (result.request.lookaheadMs
                     - qqsc::params::snapLookaheadMs (
                         readParameter (processor, qqsc::params::lookaheadMs))) > 0.01f)
    {
        scheduleHpfReplayRetry (result.request.markers.empty()
                                    ? 0 : result.request.markers.back());
        return;
    }

    for (size_t domain = 0; domain < histories.size(); ++domain)
    {
        auto& points = histories[domain].points;
        const auto& values = domain == 0 ? result.detectorDb0 : result.detectorDb1;
        for (auto& point : points)
        {
            const auto marker = std::lower_bound (result.request.markers.begin(),
                                                  result.request.markers.end(),
                                                  point.captureCounter);
            if (marker == result.request.markers.end() || *marker != point.captureCounter)
                continue;

            const auto index = static_cast<size_t> (std::distance (result.request.markers.begin(), marker));
            if (index >= result.firstValidMarkerIndex && index < values.size())
            {
                point.replayedDetectorDb = values[index];
                point.hasReplayedDetector = true;
            }
        }
    }

    hpfReplayRetryPending = false;
    hpfReplayBusy = false;
    hpfRetryAttempts = 0;
    hpfRetryTimerTicks = 0;
    refreshRenderCaches (lastMode);
    repaint();
}

void DynamicDisplay::drawDomainPanel (juce::Graphics& g, juce::Rectangle<float> panel, int domainIndex,
                                      const juce::String& domainName, int mode)
{
    const auto keySource = juce::roundToInt (readParameter (processor, qqsc::params::keySource));
    const bool externalKey = keySource == qqsc::params::keyExternal;
    const bool externalAvailable = processor.isExternalSidechainBusAvailable();
    const auto& cache = renderCaches[static_cast<size_t> (domainIndex)];
    const auto currentGr = cache.currentGainReductionDb;

    g.setColour (qqsc::ui::panelAlt().withAlpha (0.34f));
    g.fillRoundedRectangle (panel, 7.0f);
    g.setColour (qqsc::ui::border().withAlpha (0.45f));
    g.drawRoundedRectangle (panel.reduced (0.5f), 7.0f, 0.8f);

    auto header = panel.reduced (8.0f, 4.0f).removeFromTop (19.0f);
    auto domainArea = header.removeFromLeft (42.0f);
    auto readoutArea = header;

    g.setColour (qqsc::ui::text().withAlpha (0.88f));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawFittedText (domainName, domainArea.toNearestInt(), juce::Justification::centredLeft, 1);

    auto readout = "GR (MIX)  " + grText (currentGr);
    if (externalKey && ! externalAvailable)
        readout += "   EXT N/A";

    g.setColour (qqsc::ui::grAccent());
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawFittedText (readout, readoutArea.toNearestInt(),
                      juce::Justification::centredRight, 1);

    const auto plot = plotBoundsForPanel (panel);

    for (float db : { 0.0f, -15.0f, -30.0f, -45.0f, -60.0f, -75.0f, -90.0f })
    {
        const auto y = dbToY (db, plot);
        g.setColour (qqsc::ui::text().withAlpha (0.065f));
        g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        g.setColour (qqsc::ui::textMuted().withAlpha (0.64f));
        g.setFont (8.0f);
        g.drawText (juce::String (static_cast<int> (db)),
                    juce::roundToInt (panel.getX() + 4.0f), juce::roundToInt (y - 6.0f), 37, 12,
                    juce::Justification::right);
    }

    const auto thresholdDb = thresholdDbForDomain (domainIndex, mode);
    if (qqsc::params::isThresholdEnabled (thresholdDb))
    {
        const auto inputGainDb = readParameter (processor, qqsc::params::inputGainDb);
        const auto effectiveThresholdDb = externalKey
            ? thresholdDb : qqsc::params::effectiveDisplayThresholdDb (thresholdDb, inputGainDb);
        const auto thresholdY = dbToY (effectiveThresholdDb, plot);
        const float dashPattern[] { 5.0f, 4.0f };
        g.setColour (qqsc::ui::warmAccent().withAlpha (0.82f));
        g.drawDashedLine ({ plot.getX(), thresholdY, plot.getRight(), thresholdY }, dashPattern, 2, 1.2f);

        const auto thresholdText = juce::String (thresholdDb, 2) + " dB";
        const auto tagWidth = 70.0f;
        auto tag = juce::Rectangle<float> (plot.getRight() - tagWidth - 2.0f,
                                            thresholdY - 8.0f, tagWidth, 16.0f);
        tag.setY (juce::jlimit (plot.getY(), plot.getBottom() - tag.getHeight(), tag.getY()));
        g.setColour (qqsc::ui::panel().withAlpha (0.90f));
        g.fillRoundedRectangle (tag, 4.0f);
        g.setColour (qqsc::ui::warmAccent().darker (0.12f));
        g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
        g.drawFittedText (thresholdText, tag.toNearestInt(), juce::Justification::centred, 1);
    }

    g.saveState();
    g.reduceClipRegion (plot.toNearestInt());

    if (externalKey && externalAvailable && cache.valid)
    {
        g.setColour (qqsc::ui::cyanAccent().withAlpha (0.10f));
        g.strokePath (cache.externalKeyPath, juce::PathStrokeType (4.0f));
        g.setColour (qqsc::ui::cyanAccent().withAlpha (0.34f));
        g.strokePath (cache.externalKeyPath, juce::PathStrokeType (1.0f));
    }

    // The former full-area translucent polygon made paint cost grow
    // with GR depth. A single cached sparse shade path preserves the
    // visual band without blending every pixel in the compressed area.
    g.setColour (qqsc::ui::grAccent().withAlpha (0.24f));
    g.strokePath (cache.gainReductionShadePath, juce::PathStrokeType (1.15f));

    g.setColour (qqsc::ui::dryTrace().withAlpha (0.82f));
    g.strokePath (cache.inputPath, juce::PathStrokeType (1.15f));

    g.setColour (qqsc::ui::grAccent().withAlpha (0.90f));
    g.strokePath (cache.gainReductionPath, juce::PathStrokeType (1.25f));

    g.setColour (qqsc::ui::outputAccent().withAlpha (0.96f));
    g.strokePath (cache.outputPath, juce::PathStrokeType (1.5f));
    g.restoreState();
}

void DynamicDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll (qqsc::ui::canvas());
    g.setColour (qqsc::ui::panel().withAlpha (0.97f));
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (qqsc::ui::border().withAlpha (0.72f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

    auto& m = processor.getMeterState();
    const auto mode = m.processingMode.load (std::memory_order_relaxed);

    auto header = bounds.reduced (14.0f, 5.0f).removeFromTop (22.0f);
    auto titleArea = header.removeFromLeft (juce::jmin (290.0f, header.getWidth() * 0.62f));
    g.setColour (qqsc::ui::text().withAlpha (0.84f));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawFittedText ("DYNAMIC LEVEL / GAIN REDUCTION HISTORY", titleArea.toNearestInt(),
                      juce::Justification::centredLeft, 1);
    g.setColour ((hpfReplayBusy ? qqsc::ui::cyanAccent() : qqsc::ui::textMuted())
                    .withAlpha (0.82f));
    g.setFont (9.5f);
    const auto headerStatus = (hpfReplayBusy ? juce::String ("HPF UPDATING   ") : juce::String())
                            + "MODE  " + qqsc::params::modeName (mode);
    g.drawFittedText (headerStatus, header.toNearestInt(),
                      juce::Justification::centredRight, 1);

    if (mode == qqsc::params::stereoLinked)
    {
        drawDomainPanel (g, domainPanelBounds (0, mode), 0, "ST", mode);
    }
    else if (mode == qqsc::params::leftRight)
    {
        drawDomainPanel (g, domainPanelBounds (0, mode), 0, "L", mode);
        drawDomainPanel (g, domainPanelBounds (1, mode), 1, "R", mode);
    }
    else
    {
        drawDomainPanel (g, domainPanelBounds (0, mode), 0, "M", mode);
        drawDomainPanel (g, domainPanelBounds (1, mode), 1, "S", mode);
    }

    const bool externalKey = juce::roundToInt (
        readParameter (processor, qqsc::params::keySource)) == qqsc::params::keyExternal;
    auto legendArea = juce::Rectangle<int> (54, getHeight() - 22, juce::jmax (1, getWidth() - 64), 17);
    const int itemCount = externalKey ? 4 : 3;
    const auto itemW = legendArea.getWidth() / itemCount;

    auto drawLegend = [&] (juce::Rectangle<int> item, juce::Colour colour, const juce::String& text)
    {
        const int lineY = item.getCentreY();
        g.setColour (colour);
        g.drawLine (static_cast<float> (item.getX() + 4), static_cast<float> (lineY),
                    static_cast<float> (item.getX() + 18), static_cast<float> (lineY), 1.8f);
        g.setFont (8.2f);
        g.drawFittedText (text, item.withTrimmedLeft (23), juce::Justification::centredLeft, 1);
    };

    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::dryTrace(), "Dry / Input");
    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::grAccent(), "GR incl. Mix");
    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::outputAccent(), "Output post-mix");

    if (externalKey)
        drawLegend (legendArea, qqsc::ui::cyanAccent().withAlpha (0.45f), "External key");
}
