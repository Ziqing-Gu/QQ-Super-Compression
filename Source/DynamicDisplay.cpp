#include "DynamicDisplay.h"
#include "Parameters.h"
#include "UTF8LookAndFeel.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>

namespace
{
constexpr float minDb = -90.0f;
constexpr float maxDb = 0.0f;
constexpr float silenceDb = -120.0f;

float dbToDetectorLevel (float db) noexcept
{
    return db <= silenceDb + 0.1f ? 0.0f
                                  : juce::Decibels::decibelsToGain (juce::jmin (0.0f, db), silenceDb);
}

float readParameter (QQSuperCompressionAudioProcessor& processor, const char* parameterID,
                     float fallback = 0.0f) noexcept
{
    if (auto* value = processor.getAPVTS().getRawParameterValue (processor.soundParameterID(parameterID)))
        return processor.readSoundParameter(parameterID);

    return fallback;
}

float effectiveDisplayWindow (QQSuperCompressionAudioProcessor& processor, float lookaheadMs) noexcept
{
    return juce::jlimit (0.0f, lookaheadMs,
        readParameter (processor, qqsc::params::detectorWindowMs, 100.0f));
}

int effectiveDisplayDetectorMode (QQSuperCompressionAudioProcessor& processor, float lookaheadMs) noexcept
{
    // At 0 ms both windows contain the same sample. Changing the stored
    // choice must not replace the retained live block peaks with a redundant
    // sample-point replay, or invalidate an otherwise valid HPF replay.
    return effectiveDisplayWindow (processor, lookaheadMs) <= 0.0f ? 0
        : juce::roundToInt (readParameter (processor, qqsc::params::detectorMode));
}

int displayProcessingMode (QQSuperCompressionAudioProcessor& processor) noexcept
{
    return juce::jlimit (static_cast<int> (qqsc::params::stereoLinked),
                         static_cast<int> (qqsc::params::leftRight),
                         juce::roundToInt (readParameter (processor, qqsc::params::processingMode)));
}

juce::String grText (float db)
{
    const auto gainDb = std::abs (db) < 0.05f ? 0.0f : -db;
    return (gainDb > 0.0f ? "+" : "") + juce::String (gainDb, 1) + " dB";
}

juce::Colour gainIncreaseColour() noexcept
{
    if (qqsc::ui::isDarkTheme())
        return juce::Colour (0xff80c897);
    return qqsc::ui::isClassicTheme() ? juce::Colour (0xff80d59c)
                                      : juce::Colour (0xff2c8452);
}

int processorDomain (int domainIndex, int mode) noexcept
{
    if (mode == qqsc::params::leftRight)
        return domainIndex + 1;
    if (mode == qqsc::params::midSide)
        return domainIndex + 3;
    return 0;
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
    ReplayPeakWindow (int lookaheadSamplesIn, bool bilateralIn)
        : lookaheadSamples (juce::jmax (0, lookaheadSamplesIn)), bilateral (bilateralIn)
    {
    }

    void process (float sample, int64_t sampleIndex)
    {
        const auto magnitude = juce::jlimit (0.0f, 1.0f, std::abs (sample));
        while (! queue.empty() && queue.back().second <= magnitude)
            queue.pop_back();

        queue.emplace_back (sampleIndex, magnitude);
        const auto oldestAllowed = sampleIndex - static_cast<int64_t> (lookaheadSamples);
        while (! queue.empty() && queue.front().first < oldestAllowed)
            queue.pop_front();

        const auto futurePeak = queue.empty() ? magnitude : queue.front().second;
        peakHistory.push_back (futurePeak);
        currentLevel = bilateral ? futurePeak : 0.0f;
        if (peakHistory.size() > static_cast<size_t> (lookaheadSamples))
        {
            currentLevel = bilateral ? juce::jmax (futurePeak, peakHistory.front())
                                     : juce::jmin (futurePeak, peakHistory.front());
            peakHistory.pop_front();
        }
    }

    float getCurrentLevel() const noexcept { return currentLevel; }

private:
    int lookaheadSamples = 0;
    bool bilateral = true;
    std::deque<std::pair<int64_t, float>> queue;
    std::deque<float> peakHistory;
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

    // The 4097-point detector grid never changes. Precompute the dB->linear
    // conversion once so parameter drags do not spend message-thread time
    // rebuilding the same exponential mapping every 60 Hz frame.
    for (int i = 0; i < dynamicsLutSize; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (dynamicsLutSize - 1);
        const auto db = juce::jmap (t, silenceDb, maxDb);
        detectorLevelLut[static_cast<size_t> (i)] = dbToDetectorLevel (db);
    }

    for (auto& cache : renderCaches)
    {
        cache.inputPath.preallocateSpace (historyLength * 3);
        cache.gainReductionPath.preallocateSpace (historyLength * 6);
        cache.gainIncreasePath.preallocateSpace (historyLength * 6);
        cache.outputPath.preallocateSpace (historyLength * 3);
        cache.externalKeyPath.preallocateSpace (historyLength * 3);
        cache.gainReductionShadePath.preallocateSpace (gainReductionShadeSegments * 6);
        cache.gainIncreaseShadePath.preallocateSpace (gainReductionShadeSegments * 6);
    }

    processor.setDisplayKeyHistoryCaptureEnabled (true);
    lastObservedHpfHz = readParameter (processor, qqsc::params::keyHpfHz,
                                       qqsc::params::keyHpfOffHz);
    lastObservedLookaheadMs = qqsc::params::snapLookaheadMs (
        readParameter (processor, qqsc::params::lookaheadMs));
    lastObservedDetectorMode = effectiveDisplayDetectorMode (processor, lastObservedLookaheadMs);
    lastObservedDetectorWindowMs = effectiveDisplayWindow (processor, lastObservedLookaheadMs);
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
    geometryDirty = true;
    if (! isShowing())
        return;

    const auto mode = displayProcessingMode (processor);
    refreshRenderCaches (mode);
    renderedHistoryRevision = historyRevision;
    renderedProjectionRevision = processor.getDisplayProjectionRevision();
    geometryDirty = false;
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
    processor.getMeterState().displayTruePeakExcessDb.store (0.0f, std::memory_order_relaxed);
    for (auto& h : histories)
        h.points.clear();

    for (auto& cache : renderCaches)
    {
        cache.projected.size = 0;
        cache.inputPath.clear();
        cache.gainReductionPath.clear();
        cache.gainIncreasePath.clear();
        cache.outputPath.clear();
        cache.externalKeyPath.clear();
        cache.gainReductionShadePath.clear();
        cache.gainIncreaseShadePath.clear();
        cache.currentGainReductionDb = 0.0f;
        cache.valid = false;
    }

    ++historyRevision;
    lastCapturedHistoryCounter = 0;
    hpfReplayRetryPending = false;
    hpfReplayBusy = false;
    hpfRetryTimerTicks = 0;
    hpfRetryAttempts = 0;
}

void DynamicDisplay::timerCallback()
{
    if (processor.isEcoMode() && ! isShowing())
    {
        replayRequestGeneration->fetch_add(1, std::memory_order_relaxed);
        hpfReplayBusy = false;
        hpfRefreshPending = true;
        return;
    }
    auto& m = processor.getMeterState();
    const auto mode = displayProcessingMode (processor);
    const auto position = processor.getDisplayKeyHistoryPosition();
    const auto projectionRevision = processor.getDisplayProjectionRevision();

    // Horizontal motion follows the audio sample counter rather than the number
    // of message-thread Timer callbacks. Mouse drags and busy hosts can delay a
    // JUCE Timer; anchoring x to audio time prevents the Display from visibly
    // entering slow motion when that happens.
    renderReferenceCounter = position.counter;
    // The key-history counter advances at the ring's analysis rate
    // (min(host rate, 48 kHz)), not necessarily at the host sample rate.
    // Use that exact counter rate so 88.2/96/192 kHz sessions keep the same
    // real-time horizontal speed as 44.1/48 kHz sessions.
    if (position.sampleRate > 1.0)
        renderReferenceSampleRate = position.sampleRate;

    const auto keySource = juce::jlimit (
        static_cast<int> (qqsc::params::keyInternal),
        static_cast<int> (qqsc::params::keyExternal),
        juce::roundToInt (readParameter (processor, qqsc::params::keySource)));
    const auto capturedInputGainDb = readParameter (processor, qqsc::params::inputGainDb);
    const auto capturedKeyGainDb = readParameter (processor, qqsc::params::keyGainDb);
    const auto capturedTruePeakExcessDb = juce::jmax (0.0f,
        m.displayTruePeakExcessDb.exchange (0.0f, std::memory_order_acq_rel));

    const int limiter = processor.isLimiterMode() ? 1 : 0;
    // Limiter ON/OFF is a projection choice, not a new history source. Keep
    // the evidence so switching Limiter also reshapes the already-visible past.
    if (mode != lastMode || keySource != lastKeySource
        || position.generation != lastCaptureGeneration)
    {
        clearHistories();
        lastMode = mode;
        lastLimiter = limiter;
        lastKeySource = keySource;
        lastCaptureGeneration = position.generation;
        lastCapturedHistoryCounter = position.counter;
    }

    // Host audio may be stopped when the user switches ST/LR/MS. Geometry
    // follows the current parameter immediately; never relabel old-domain
    // meter data as a new-domain history while waiting for the audio callback.
    const bool hasNewAudioEvidence = position.counter != lastCapturedHistoryCounter;
    if (mode == m.processingMode.load (std::memory_order_relaxed)
        && (hasNewAudioEvidence || histories[0].points.empty()))
    {
        HistoryPoint point0;
        point0.inputDb = m.displayInputDb0.load (std::memory_order_relaxed);
        point0.detectorDb = m.displayDetectorDb0.load (std::memory_order_relaxed);
        point0.truePeakExcessDb = capturedTruePeakExcessDb;
        point0.capturedInputGainDb = capturedInputGainDb;
        point0.capturedKeyGainDb = capturedKeyGainDb;
        point0.captureGeneration = position.generation;
        point0.captureCounter = position.counter;
        point0.carrierDelaySeconds = processor.getDisplayCoreDelaySeconds();
        pushHistory (histories[0], point0);

        if (mode != qqsc::params::stereoLinked)
        {
            HistoryPoint point1;
            point1.inputDb = m.displayInputDb1.load (std::memory_order_relaxed);
            point1.detectorDb = m.displayDetectorDb1.load (std::memory_order_relaxed);
            point1.truePeakExcessDb = capturedTruePeakExcessDb;
            point1.capturedInputGainDb = capturedInputGainDb;
            point1.capturedKeyGainDb = capturedKeyGainDb;
            point1.captureGeneration = position.generation;
            point1.captureCounter = position.counter;
            point1.carrierDelaySeconds = point0.carrierDelaySeconds;
            pushHistory (histories[1], point1);
        }
        lastCapturedHistoryCounter = position.counter;
        trimHistoryToVisibleWindow (histories[0], position.counter);
        if (mode != qqsc::params::stereoLinked)
            trimHistoryToVisibleWindow (histories[1], position.counter);
        ++historyRevision;
    }

    const auto currentHpfHz = readParameter (processor, qqsc::params::keyHpfHz,
                                             qqsc::params::keyHpfOffHz);
    const auto currentLookaheadMs = qqsc::params::snapLookaheadMs (
        readParameter (processor, qqsc::params::lookaheadMs));
    const auto currentDetectorMode = effectiveDisplayDetectorMode (processor, currentLookaheadMs);
    const auto currentWindowMs = effectiveDisplayWindow (processor, currentLookaheadMs);
    const int currentOs=juce::roundToInt(readParameter(processor,qqsc::params::oversampling));
    if (lastObservedOversampling>=0 && currentOs!=lastObservedOversampling)
    { hpfStableTimerTicks=0; hpfRefreshPending=true; }
    lastObservedOversampling=currentOs;
    if (std::abs (currentWindowMs - lastObservedDetectorWindowMs) > 0.001f)
    {
        lastObservedDetectorWindowMs = currentWindowMs;
        hpfStableTimerTicks = 0;
        hpfRefreshPending = true;
    }
    if (std::abs (currentLookaheadMs - lastObservedLookaheadMs) > 0.01f)
    {
        lastObservedLookaheadMs = currentLookaheadMs;
        hpfStableTimerTicks = 0;
        hpfRefreshPending = true;
    }
    if (currentDetectorMode != lastObservedDetectorMode)
    {
        lastObservedDetectorMode = currentDetectorMode;
        hpfStableTimerTicks = 0;
        hpfRefreshPending = true;
    }
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

    // Keep collecting evidence while an editor/component is hidden, but do not
    // spend message-thread time re-projecting paths that cannot be seen. When
    // it becomes visible again the revision mismatch rebuilds everything once.
    if (! isShowing())
        return;

    const bool projectionDirty = projectionRevision != renderedProjectionRevision;
    const bool historyDirty = historyRevision != renderedHistoryRevision;
    if (projectionDirty || historyDirty || geometryDirty)
    {
        refreshRenderCaches (mode);
        renderedProjectionRevision = projectionRevision;
        renderedHistoryRevision = historyRevision;
        geometryDirty = false;
        repaint();
    }
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
    const auto floor=processor.isLimiterMode() ? qqsc::params::limiterThresholdMinimumDb : minDb;
    return juce::jmap (juce::jlimit (floor, maxDb, db), maxDb, floor, plot.getY(), plot.getBottom());
}

float DynamicDisplay::historyXForCounter (uint64_t captureCounter,
                                          juce::Rectangle<float> plot) const noexcept
{
    if (renderReferenceCounter == 0 || renderReferenceSampleRate <= 1.0)
        return plot.getRight();

    const auto ageSamples = renderReferenceCounter > captureCounter
        ? renderReferenceCounter - captureCounter : uint64_t { 0 };
    const auto ageSeconds = static_cast<double> (ageSamples) / renderReferenceSampleRate;
    const auto ageNormalised = juce::jlimit (
        0.0, 1.0, ageSeconds / static_cast<double> (historyWindowSeconds));
    return plot.getRight() - plot.getWidth() * static_cast<float> (ageNormalised);
}

void DynamicDisplay::trimHistoryToVisibleWindow (HistorySet& history,
                                                 uint64_t referenceCounter) const
{
    if (referenceCounter == 0 || renderReferenceSampleRate <= 1.0)
        return;

    const auto maxAgeSamples = static_cast<uint64_t> (
        std::llround (renderReferenceSampleRate * static_cast<double> (historyWindowSeconds)));

    while (! history.points.empty())
    {
        const auto counter = history.points.front().captureCounter;
        if (referenceCounter <= counter || referenceCounter - counter <= maxAgeSamples)
            break;
        history.points.pop_front();
    }

    while (static_cast<int> (history.points.size()) > historyLength)
        history.points.pop_front();
}

juce::Rectangle<float> DynamicDisplay::getBoundaryPlotForDomain (int parameterDomainIndex) const noexcept
{
    const auto domain = juce::jlimit (0, 4, parameterDomainIndex);
    const auto mode = domain == 0 ? qqsc::params::stereoLinked
                                  : (domain <= 2 ? qqsc::params::leftRight : qqsc::params::midSide);
    const auto panelIndex = domain == 0 ? 0 : (domain - 1) % 2;
    return plotBoundsForPanel (domainPanelBounds (panelIndex, mode));
}

float DynamicDisplay::getBoundaryYForDomainDb (int parameterDomainIndex, float detectorDb, bool upper) const noexcept
{
    detectorDb = juce::jmax (processor.boundaryMinimumDb(upper), detectorDb);
    const auto plot = getBoundaryPlotForDomain (parameterDomainIndex);
    // OFF is an unbounded Range, not a finite +1 dB threshold.
    if (! qqsc::params::isRangeEnabled (detectorDb))
        return plot.getY();

    const bool externalKey = juce::roundToInt (readParameter (processor, qqsc::params::keySource))
                             == qqsc::params::keyExternal;
    const auto inputGainDb = readParameter (processor, qqsc::params::inputGainDb);
    return dbToY (externalKey ? detectorDb
                              : qqsc::params::effectiveDisplayThresholdDb (detectorDb, inputGainDb), plot);
}

float DynamicDisplay::getBoundaryDbForY (int parameterDomainIndex, float localY, bool upper) const noexcept
{
    const auto plot = getBoundaryPlotForDomain (parameterDomainIndex);
    if (plot.getHeight() <= 0.0f)
        return qqsc::params::thresholdOffDb;

    const auto displayDb = juce::jmap (juce::jlimit (plot.getY(), plot.getBottom(), localY),
                                      plot.getY(), plot.getBottom(), maxDb,
                                      processor.isLimiterMode() ? qqsc::params::limiterThresholdMinimumDb : minDb);
    const bool externalKey = juce::roundToInt (readParameter (processor, qqsc::params::keySource))
                             == qqsc::params::keyExternal;
    const auto detectorDb = externalKey ? displayDb
                                        : displayDb + readParameter (processor, qqsc::params::inputGainDb);
    // A drag maps finite values only. The fader explicitly chooses the OFF
    // endpoint, keeping it distinct from a finite 0 dB upper boundary.
    return juce::jlimit (processor.boundaryMinimumDb(upper), 0.0f, detectorDb);
}

void DynamicDisplay::updatePath (juce::Path& path,
                                 const std::array<float, historyLength>& values,
                                 const std::array<uint64_t, historyLength>& captureCounters,
                                 size_t valueCount, juce::Rectangle<float> plot) const
{
    path.clear();
    if (valueCount == 0)
        return;

    for (size_t i = 0; i < valueCount; ++i)
    {
        const auto x = historyXForCounter (captureCounters[i], plot);
        const auto y = dbToY (values[i], plot);

        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }
}

void DynamicDisplay::updateGainChangePaths (
    juce::Path& reductionPath, juce::Path& increasePath,
    const ProjectedHistory& projected, juce::Rectangle<float> plot) const
{
    reductionPath.clear();
    increasePath.clear();
    if (projected.size == 0)
        return;

    auto pointAt = [&] (size_t i)
    {
        return juce::Point<float> (
            historyXForCounter (projected.captureCounter[i], plot),
            dbToY (projected.gainReductionBoundary[i], plot));
    };

    auto previous = pointAt (0);
    auto previousGr = projected.effectiveGainReduction[0];
    auto* active = previousGr < 0.0f ? &increasePath : &reductionPath;
    active->startNewSubPath (previous);
    for (size_t i = 1; i < projected.size; ++i)
    {
        const auto next = pointAt (i);
        const auto nextGr = projected.effectiveGainReduction[i];
        auto* nextPath = nextGr < 0.0f ? &increasePath : &reductionPath;
        if (nextPath != active)
        {
            // The sign change crosses unity. Join both colours at that
            // crossing so one history needs only one traversal and no overlay.
            const auto fraction = previousGr / (previousGr - nextGr);
            const auto crossing = previous + (next - previous) * fraction;
            active->lineTo (crossing);
            nextPath->startNewSubPath (crossing);
            active = nextPath;
        }
        active->lineTo (next);
        previous = next;
        previousGr = nextGr;
    }
}

void DynamicDisplay::updateGainReductionShadePath (
    juce::Path& reductionPath, juce::Path& increasePath,
    const std::array<float, historyLength>& upper,
    const std::array<float, historyLength>& lower,
    const std::array<float, historyLength>& gainReduction,
    const std::array<uint64_t, historyLength>& captureCounters,
    size_t valueCount, juce::Rectangle<float> plot) const
{
    reductionPath.clear();
    increasePath.clear();
    if (valueCount == 0)
        return;

    const auto stride = std::max<size_t> (
        1, (valueCount + static_cast<size_t> (gainReductionShadeSegments) - 1)
               / static_cast<size_t> (gainReductionShadeSegments));

    for (size_t i = 0; i < valueCount; i += stride)
    {
        const auto upperY = dbToY (upper[i], plot);
        const auto lowerY = dbToY (lower[i], plot);
        if (std::abs (lowerY - upperY) < 0.35f)
            continue;

        const auto x = historyXForCounter (captureCounters[i], plot);
        auto& path = gainReduction[i] < 0.0f ? increasePath : reductionPath;
        path.startNewSubPath (x, upperY);
        path.lineTo (x, lowerY);
    }
}

juce::Rectangle<float> DynamicDisplay::domainPanelBounds (int domainIndex, int mode) const noexcept
{
    auto content = getLocalBounds().toFloat().reduced (10.0f, 6.0f);
    content.removeFromTop (24.0f);
    content.removeFromBottom (23.0f);

    if (mode == qqsc::params::stereoLinked || processor.isLimiterMode())
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

uint64_t DynamicDisplay::dynamicsLutSignatureForDomain (int domainIndex, int mode) const noexcept
{
    uint64_t hash = 1469598103934665603ull;
    const auto mixWord = [&] (uint64_t word)
    {
        hash ^= word;
        hash *= 1099511628211ull;
    };
    const auto mixFloat = [&] (float value)
    {
        uint32_t bits = 0;
        static_assert (sizeof (bits) == sizeof (value));
        std::memcpy (&bits, &value, sizeof (bits));
        mixWord (bits);
    };

    const auto domain = processorDomain (domainIndex, mode);
    const bool dual = readParameter (processor, qqsc::params::compressionMode) >= 0.5f;
    mixWord (static_cast<uint64_t> (processor.isLimiterMode()));
    mixWord (static_cast<uint64_t> (dual));
    mixWord (static_cast<uint64_t> (juce::roundToInt (readParameter (processor, qqsc::params::keySource))));
    mixFloat (processor.getBoundaryForDomainDb (dual, false, domain));
    mixFloat (processor.getBoundaryForDomainDb (dual, true, domain));

    if (dual)
    {
        mixFloat (readParameter (processor, qqsc::params::upEnabledIds[static_cast<size_t> (domain)]));
        mixFloat (readParameter (processor, qqsc::params::downEnabledIds[static_cast<size_t> (domain)]));
        mixFloat (readParameter (processor, qqsc::params::upRatioIds[static_cast<size_t> (domain)]));
        mixFloat (readParameter (processor, qqsc::params::downRatioIds[static_cast<size_t> (domain)]));
        mixFloat (readParameter (processor, "upAlgorithmMode"));
        mixFloat (readParameter (processor, "downAlgorithmMode"));
    }
    else
    {
        mixFloat (readParameter (processor, qqsc::params::ratioIds[static_cast<size_t> (domain)]));
        mixFloat (readParameter (processor, qqsc::params::algorithmMode));
    }
    return hash;
}

void DynamicDisplay::rebuildDynamicsLut (RenderCache& cache, int domainIndex, int mode,
                                             uint64_t signature)
{
    const auto domain = processorDomain (domainIndex, mode);

    // Snapshot all dynamics parameters once, then fill the complete dense LUT.
    // v1.2.21 called getDynamicsGainForDomain() 4097 times; during a drag that
    // repeated APVTS/bank/algorithm reads thousands of times every display frame.
    // The processor bulk helper uses the identical StaticCompressionEngine law
    // but reads the current parameter state once per LUT rebuild.
    processor.fillDynamicsGainForDomain (
        detectorLevelLut.data(), cache.dynamicsGainLut.data(),
        static_cast<size_t> (dynamicsLutSize), domain);

    cache.dynamicsLutUnity = std::all_of (cache.dynamicsGainLut.begin(),
                                          cache.dynamicsGainLut.end(),
                                          [] (float gain) { return gain == 1.0f; });
    cache.dynamicsLutSignature = signature;
    cache.dynamicsLutValid = true;
}

float DynamicDisplay::dynamicsGainFromLut (const RenderCache& cache, float detectorDb) const noexcept
{
    if (! cache.dynamicsLutValid)
        return 1.0f;

    const auto clamped = juce::jlimit (silenceDb, maxDb, detectorDb);
    const auto position = (clamped - silenceDb) / (maxDb - silenceDb)
                        * static_cast<float> (dynamicsLutSize - 1);
    const auto lo = juce::jlimit (0, dynamicsLutSize - 1, static_cast<int> (std::floor (position)));
    const auto hi = juce::jmin (dynamicsLutSize - 1, lo + 1);
    const auto fraction = position - static_cast<float> (lo);
    return juce::jmap (fraction,
                       cache.dynamicsGainLut[static_cast<size_t> (lo)],
                       cache.dynamicsGainLut[static_cast<size_t> (hi)]);
}

void DynamicDisplay::refreshRenderCaches (int mode)
{
    const auto keySource = juce::roundToInt (readParameter (processor, qqsc::params::keySource));
    const bool externalKey = keySource == qqsc::params::keyExternal;
    const bool bypassed = readParameter (processor, qqsc::params::bypass) >= 0.5f;
    const auto domainCount = mode == qqsc::params::stereoLinked ? 1 : 2;

    // Both channel LUTs must be current before either linked projection uses
    // its partner. Otherwise the first panel could use the previous A/B bank.
    for(int domain=0;domain<domainCount;++domain)
    {
        auto& cache=renderCaches[size_t(domain)];
        const auto signature=dynamicsLutSignatureForDomain(domain,mode);
        if(!cache.dynamicsLutValid || cache.dynamicsLutSignature!=signature)
            rebuildDynamicsLut(cache,domain,mode,signature);
    }
    for (int domain = 0; domain < static_cast<int> (renderCaches.size()); ++domain)
    {
        auto& cache = renderCaches[static_cast<size_t> (domain)];
        if (domain >= domainCount || getWidth() <= 0 || getHeight() <= 0)
        {
            cache.projected.size = 0;
            cache.inputPath.clear();
            cache.gainReductionPath.clear();
            cache.gainIncreasePath.clear();
            cache.outputPath.clear();
            cache.externalKeyPath.clear();
            cache.gainReductionShadePath.clear();
            cache.gainIncreaseShadePath.clear();
            cache.currentGainReductionDb = 0.0f;
            cache.valid = false;
            continue;
        }

        const auto dynamicsSignature = dynamicsLutSignatureForDomain (domain, mode);
        if (! cache.dynamicsLutValid || cache.dynamicsLutSignature != dynamicsSignature)
            rebuildDynamicsLut (cache, domain, mode, dynamicsSignature);

        projectHistory (domain, mode, externalKey, bypassed, cache, cache.projected);
        const auto plot = plotBoundsForPanel (domainPanelBounds (domain, mode));
        updatePath (cache.inputPath, cache.projected.input,
                    cache.projected.captureCounter, cache.projected.size, plot);
        updateGainChangePaths (cache.gainReductionPath, cache.gainIncreasePath,
                               cache.projected, plot);
        updatePath (cache.outputPath, cache.projected.output,
                    cache.projected.captureCounter, cache.projected.size, plot);
        updatePath (cache.externalKeyPath, cache.projected.externalKey,
                    cache.projected.captureCounter, cache.projected.size, plot);
        updateGainReductionShadePath (cache.gainReductionShadePath, cache.gainIncreaseShadePath,
                                      cache.projected.input,
                                      cache.projected.gainReductionBoundary,
                                      cache.projected.effectiveGainReduction,
                                      cache.projected.captureCounter,
                                      cache.projected.size, plot);
        cache.currentGainReductionDb = cache.projected.size == 0
            ? 0.0f : cache.projected.effectiveGainReduction[cache.projected.size - 1];
        cache.valid = true;
    }
    if(processor.isLimiterMode())
    {
        // Project both independent detectors first, including audio Link and
        // Ceiling, then combine equal-time peaks into the single ST view.
        // Taking max(detector) before compression would silently depict 100%
        // linking even when the user selected independent limiting.
        auto& cache=stereoLimiterCache;
        auto& combined=cache.projected;combined.size=0;
        const auto& left=renderCaches[0].projected;
        const auto& right=renderCaches[1].projected;
        size_t l=0,r=0;
        while(l<left.size && r<right.size && combined.size<size_t(historyLength))
        {
            if(left.captureCounter[l]<right.captureCounter[r]) {++l;continue;}
            if(right.captureCounter[r]<left.captureCounter[l]) {++r;continue;}
            const auto i=combined.size++;
            combined.captureCounter[i]=left.captureCounter[l];
            combined.input[i]=juce::jmax(left.input[l],right.input[r]);
            combined.output[i]=juce::jmax(left.output[l],right.output[r]);
            combined.gainReductionBoundary[i]=juce::jmax(left.gainReductionBoundary[l],right.gainReductionBoundary[r]);
            combined.externalKey[i]=juce::jmax(left.externalKey[l],right.externalKey[r]);
            // Reference the same dominant level as the per-channel GR traces.
            const auto reference=juce::jmax(left.gainReductionBoundary[l]+left.effectiveGainReduction[l],
                                           right.gainReductionBoundary[r]+right.effectiveGainReduction[r]);
            combined.effectiveGainReduction[i]=reference-combined.gainReductionBoundary[i];
            ++l;++r;
        }
        const auto plot=plotBoundsForPanel(domainPanelBounds(0,mode));
        updatePath(cache.inputPath,combined.input,combined.captureCounter,combined.size,plot);
        updateGainChangePaths(cache.gainReductionPath,cache.gainIncreasePath,combined,plot);
        updatePath(cache.outputPath,combined.output,combined.captureCounter,combined.size,plot);
        updatePath(cache.externalKeyPath,combined.externalKey,combined.captureCounter,combined.size,plot);
        updateGainReductionShadePath(cache.gainReductionShadePath,cache.gainIncreaseShadePath,
            combined.input,combined.gainReductionBoundary,combined.effectiveGainReduction,combined.captureCounter,combined.size,plot);
        cache.currentGainReductionDb=combined.size==0 ? 0.f : combined.effectiveGainReduction[combined.size-1];
        cache.valid=true;
    }
}

float DynamicDisplay::thresholdDbForDomain (int domainIndex, int mode) const noexcept
{
    const bool dual = readParameter (processor, qqsc::params::compressionMode) >= 0.5f;
    const auto db = processor.getBoundaryForDomainDb (dual, false, processorDomain (domainIndex, mode));
    return juce::jmax (processor.boundaryMinimumDb(false), db);
}

float DynamicDisplay::upperBoundaryDbForDomain (int domainIndex, int mode) const noexcept
{
    const bool dual = readParameter (processor, qqsc::params::compressionMode) >= 0.5f;
    const auto db = processor.getBoundaryForDomainDb (dual, true, processorDomain (domainIndex, mode));
    return juce::jmax (processor.boundaryMinimumDb(true), db);
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
                                     bool bypassed, const RenderCache& cache,
                                     ProjectedHistory& projected) const
{
    projected.size = 0;
    const auto inputGainDb = readParameter (processor, qqsc::params::inputGainDb);
    const auto keyGainDb = readParameter (processor, qqsc::params::keyGainDb);
    const auto inputGain = juce::Decibels::decibelsToGain (inputGainDb);
    const auto wetMix = mixForDomain (domainIndex, mode);
    const bool canLink=processor.isLimiterMode() && mode==qqsc::params::leftRight;
    const float link=canLink ? readParameter(processor,qqsc::params::limiterStereoLink)*.01f : 0.0f;
    const auto makeupGain = juce::Decibels::decibelsToGain (makeupDbForDomain (domainIndex, mode), -180.0f);
    const auto activeOutputGainDb = processor.getActiveOutputGainDb();
    const auto outputGain = juce::Decibels::decibelsToGain (activeOutputGainDb);
    const bool limiter = processor.isLimiterMode();
    const bool truePeak = limiter && processor.isTruePeakSelected();
    const auto ceilingDb = readParameter (processor, qqsc::params::ceilingDb);
    const auto tpRecovery = processor.getTpRecoveryMode();

    // Limiter Ceiling is a post-dynamics stage. History stores only level/shape
    // evidence; the CURRENT Hard/TP choice is replayed on every projection.
    // TP recovery is time-dependent; Hard Clip is instantaneous.
    // This deliberately preserves QQ Super Compression's defining Display rule:
    // changing a current control reshapes the already-visible history.
    const float recoverySeconds = tpRecovery == qqsc::params::tpTight ? .015f
        : tpRecovery == qqsc::params::tpSmooth ? .140f : .050f;
    const float recoveryStep = 1.0f / (1.0f + float (displayRefreshHz) * recoverySeconds);
    float retrospectiveTpGain = 1.0f, peerRetrospectiveTpGain=1.0f;
    const int peerIndex=1-domainIndex;
    const auto& peerPoints=histories[size_t(peerIndex)].points;
    auto peerCursor=peerPoints.begin();
    const auto detectorForPoint=[&](const HistoryPoint& p) {
        float db=p.hasReplayedDetector ? p.replayedDetectorDb : p.detectorDb;
        db+=p.hasReplayedDetector ? (externalKey ? keyGainDb : inputGainDb)
            : externalKey ? keyGainDb-p.capturedKeyGainDb : inputGainDb-p.capturedInputGainDb;
        return juce::jlimit(silenceDb,maxDb,db);
    };

    for (const auto& point : histories[static_cast<size_t> (domainIndex)].points)
    {
        auto detectorDb = point.hasReplayedDetector ? point.replayedDetectorDb
                                                     : point.detectorDb;

        // Replayed evidence is pre-gain. Live fallback points are post-gain and
        // are moved by the delta from their capture setting. No rendered GR is
        // stored, so Input/Key controls remain retrospective as well.
        if (point.hasReplayedDetector)
            detectorDb += externalKey ? keyGainDb : inputGainDb;
        else if (externalKey)
            detectorDb += keyGainDb - point.capturedKeyGainDb;
        else
            detectorDb += inputGainDb - point.capturedInputGainDb;

        detectorDb = juce::jlimit (silenceDb, maxDb, detectorDb);

        // The current transfer law is cached as a dense detector-level LUT.
        // Stable playback therefore reprojects 8 seconds of history with only
        // interpolation; expensive Classic/Super pow/rational evaluation is
        // rebuilt once when a parameter revision changes.
        auto compressedGain = dynamicsGainFromLut (cache, detectorDb);
        const HistoryPoint* peerPoint=nullptr;
        float peerGain=1.0f,peerDetectorDb=silenceDb;
        if(link>0.0f)
        {
            while(peerCursor!=peerPoints.end() && peerCursor->captureCounter<point.captureCounter) ++peerCursor;
            if(peerCursor!=peerPoints.end() && peerCursor->captureCounter==point.captureCounter
                && peerCursor->captureGeneration==point.captureGeneration)
            {
                peerPoint=&*peerCursor;peerDetectorDb=detectorForPoint(*peerPoint);
                peerGain=dynamicsGainFromLut(renderCaches[size_t(peerIndex)],peerDetectorDb);
                qqsc::params::coupleLimiterGains(compressedGain,peerGain,link);
            }
        }
        const bool dynamicsAreNeutral=bypassed || wetMix<=0.0f
            || (cache.dynamicsLutUnity && (peerPoint==nullptr || compressedGain==1.0f));
        const auto dynamicsMixGrDb = bypassed ? 0.0f
            : qqsc::StaticCompressionEngine::effectiveGainReductionDb (compressedGain, wetMix);

        // IMPORTANT DISPLAY CONTRACT
        // --------------------------
        // The blue trace is a calculated transfer/history view, not a measured
        // post-compressor waveform. For INT, the detector peak is the level to
        // which the current static Classic/Super transfer law actually applies.
        // Convert it back to the pre-Input-Gain Display reference. This keeps the
        // curve monotonic, makes finite-Ratio TP-OFF output stay on/above the
        // displayed DOWN threshold, and prevents impossible spikes caused by
        // pairing an unrelated carrier-block peak with a lookahead detector peak.
        // EXT is intentionally different: key and carrier are unrelated, so the
        // captured carrier remains the display reference while the external key
        // supplies only gain control.
        // A neutral transfer must use the captured carrier for every trace:
        // lookahead detector peaks do not change the audible sample at 1:1.
        const auto transferInputDb = (externalKey || dynamicsAreNeutral)
            ? point.inputDb : detectorDb - inputGainDb;
        const auto cutMixDb = bypassed ? transferInputDb
                                       : transferInputDb - dynamicsMixGrDb;

        // Re-project the current audible path from the SAME transfer evidence.
        // Makeup is inside the Wet leg; Input/Output are fixed gains. These do
        // not belong to base Dynamics GR, but they DO affect how hard the later
        // TP stage is driven, so changing them can retrospectively change the TP
        // contribution to the blue trace when TP is enabled.
        auto projectedPreCeilingDb = transferInputDb;
        if (! bypassed)
        {
            const auto mixedGainWithMakeup = (1.0f - wetMix)
                                           + compressedGain * makeupGain * wetMix;
            const auto totalGain = inputGain
                                 * juce::jmax (0.0f, mixedGainWithMakeup)
                                 * outputGain;
            projectedPreCeilingDb += juce::Decibels::gainToDecibels (
                juce::jmax (totalGain, 1.0e-9f), -180.0f);
        }

        // Re-evaluate the current Limiter Ceiling choice from level-independent
        // 8x inter-sample evidence. TP OFF is now an 8x Hard Clipper, so it uses
        // the same reconstructed peak evidence but has no recovery envelope.
        // TP ON retains the established time-dependent protection/recovery. The
        // right-side GAIN +/- meter remains the authority for real-time DSP GR.
        float projectedCeilingGrDb = 0.0f;
        float projectedPostCeilingDb = projectedPreCeilingDb;
        if (limiter && ! bypassed)
        {
            const auto projectedPeakDb = projectedPreCeilingDb
                + juce::jmax (0.0f, point.truePeakExcessDb);
            const auto overshootDb = projectedPeakDb - ceilingDb;
            if (overshootDb > 0.0f)
                projectedCeilingGrDb = overshootDb + (truePeak ? 0.01f : 0.0f);

            if (truePeak)
            {
                const auto targetGain = juce::Decibels::decibelsToGain (-projectedCeilingGrDb);
                if (targetGain <= retrospectiveTpGain)
                    retrospectiveTpGain = targetGain;
                else
                    retrospectiveTpGain += recoveryStep * (targetGain - retrospectiveTpGain);

                projectedCeilingGrDb = -juce::Decibels::gainToDecibels (
                    juce::jmax (retrospectiveTpGain, 1.0e-9f), -180.0f);
            }
            if(peerPoint!=nullptr)
            {
                const auto peerMix=mixForDomain(peerIndex,mode);
                const auto peerMakeup=juce::Decibels::decibelsToGain(makeupDbForDomain(peerIndex,mode),-180.0f);
                const auto peerInput=(externalKey || (renderCaches[size_t(peerIndex)].dynamicsLutUnity && peerGain==1.0f) || peerMix<=0)
                    ? peerPoint->inputDb : peerDetectorDb-inputGainDb;
                const auto peerTotal=inputGain*((1-peerMix)+peerGain*peerMakeup*peerMix)*outputGain;
                const auto peerPre=peerInput+juce::Decibels::gainToDecibels(juce::jmax(peerTotal,1.e-9f),-180.f);
                const auto peerOvershoot=peerPre+juce::jmax(0.0f,peerPoint->truePeakExcessDb)-ceilingDb;
                float peerCeilingGain=juce::Decibels::decibelsToGain(-juce::jmax(0.f,peerOvershoot+(peerOvershoot>0 && truePeak ? .01f : 0.f)));
                if(truePeak)
                {
                    if(peerCeilingGain<=peerRetrospectiveTpGain) peerRetrospectiveTpGain=peerCeilingGain;
                    else peerRetrospectiveTpGain+=recoveryStep*(peerCeilingGain-peerRetrospectiveTpGain);
                    peerCeilingGain=peerRetrospectiveTpGain;
                }
                auto ownCeilingGain=juce::Decibels::decibelsToGain(-projectedCeilingGrDb);
                qqsc::params::coupleLimiterGains(ownCeilingGain,peerCeilingGain,link);
                projectedCeilingGrDb=-juce::Decibels::gainToDecibels(juce::jmax(ownCeilingGain,1.e-9f),-180.f);
            }
            projectedPostCeilingDb -= projectedCeilingGrDb;
        }

        if (projected.size >= static_cast<size_t> (historyLength))
            break;

        const auto index = projected.size++;
        projected.input[index] = point.inputDb;

        // Blue = current QQ compression transfer + current Mix + CURRENT Limiter
        // Ceiling attenuation. TP OFF therefore shows the 8x Hard Clipper's
        // instantaneous clip depth; TP ON shows the replayed TP envelope.
        const auto ceilingGrForBlue = (limiter && ! bypassed) ? projectedCeilingGrDb : 0.0f;
        projected.gainReductionBoundary[index] = cutMixDb - ceilingGrForBlue;
        projected.effectiveGainReduction[index] = dynamicsMixGrDb + ceilingGrForBlue;

        // Orange remains the current projected final output, including Makeup,
        // Output Gain and the active Hard-Clip/True-Peak Ceiling stage.
        projected.output[index] = limiter && ! bypassed
            ? projectedPostCeilingDb
            : (bypassed ? point.inputDb : projectedPreCeilingDb);
        projected.externalKey[index] = detectorDb;
        projected.captureCounter[index] = point.captureCounter;
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
    if (processor.isEcoMode() && ! isShowing())
    {
        hpfRefreshPending = true;
        return false;
    }
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
    request.detectorMode = effectiveDisplayDetectorMode (processor, request.lookaheadMs);
    request.detectorWindowMs = effectiveDisplayWindow (processor, request.lookaheadMs);
    request.oversampling=juce::roundToInt(readParameter(processor,qqsc::params::oversampling));
    request.markers.reserve (histories[0].points.size());

    for (const auto& point : histories[0].points)
    {
        if (point.captureGeneration != request.captureGeneration)
        {
            scheduleHpfReplayRetry (point.captureCounter);
            return false;
        }
        request.markers.push_back (point.captureCounter);
        request.carrierDelays.push_back(point.carrierDelaySeconds);
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
    if (! processor.shouldRunUiAnalysis()) return false;
    QQSuperCompressionAudioProcessor::DisplayKeyHistorySnapshot snapshot;
    if (request.markers.empty()
        || ! processor.copyDisplayKeyHistory (request.captureGeneration,
                                              request.requestedStartCounter,
                                              request.markers.back()+9600, snapshot)
        || snapshot.keySource != request.keySource)
        return false;

    const auto currentRequest = replayRequestGeneration->load (std::memory_order_relaxed);
    if (worker.threadShouldExit() || currentRequest != request.requestGeneration)
        return false;

    const auto maxLookaheadSamples = juce::jmax (0, static_cast<int> (
        std::ceil (snapshot.sampleRate * 0.100)));
    const auto lookaheadSamples = juce::jlimit (0, maxLookaheadSamples, static_cast<int> (
        std::round (snapshot.sampleRate * static_cast<double> (request.lookaheadMs) * 0.001)));

    const int factor=qqsc::params::oversamplingFactorForChoiceIndex(request.oversampling);
    const int stages=qqsc::params::oversamplingStageCountForChoiceIndex(request.oversampling);
    const int windowSamples=juce::jlimit(0,lookaheadSamples*factor,
        int(std::round(snapshot.sampleRate*factor*double(request.detectorWindowMs)*.001)));
    ReplayPeakWindow primaryEngine(windowSamples,request.detectorMode!=0);
    ReplayPeakWindow secondaryEngine(windowSamples,request.detectorMode!=0);
    const bool needsSecondaryEngine=request.mode!=qqsc::params::stereoLinked || snapshot.stereoKey;
    const bool hpfEnabled=qqsc::params::isKeyHpfEnabled(request.hpfHz);
    const auto coefficients=replayHighPassCoefficients(snapshot.sampleRate,request.hpfHz);
    ReplayHighPassState filterLeft,filterRight;

    // Replay on the same oversampled detector grid. Only this worker allocates.
    juce::dsp::Oversampling<float> os(2,size_t(stages),
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true);
    os.initProcessing(256);
    juce::AudioBuffer<float> replayInput(2,256);replayInput.clear();
    replayInput.setSample(0,0,1.0f);
    const auto impulse=os.processSamplesUp(juce::dsp::AudioBlock<float>(replayInput));
    int peakIndex=0;
    for(int i=1;i<int(impulse.getNumSamples());++i)
        if(std::abs(impulse.getSample(0,i))>std::abs(impulse.getSample(0,peakIndex)))peakIndex=i;
    const double detectorDelay=double(windowSamples+peakIndex)/factor;
    os.reset();
    std::vector<std::array<float,2>> levels(snapshot.left.size());
    for(size_t offset=0;offset<snapshot.left.size();offset+=256)
    {
        if(worker.threadShouldExit() || !processor.shouldRunUiAnalysis()
            || replayRequestGeneration->load()!=request.requestGeneration)return false;
        const int count=int(std::min(size_t(256),snapshot.left.size()-offset));
        for(int i=0;i<count;++i)
        {
            float l=snapshot.left[offset+size_t(i)],r=snapshot.stereoKey?snapshot.right[offset+size_t(i)]:l;
            if(hpfEnabled){l=filterLeft.process(l,coefficients);r=snapshot.stereoKey?filterRight.process(r,coefficients):l;}
            if(request.mode==qqsc::params::midSide)
            {const float m=snapshot.stereoKey?.5f*(l+r):l;
             r=snapshot.stereoKey?.5f*(l-r):(request.keySource==qqsc::params::keyExternal?l:0.f);l=m;}
            replayInput.setSample(0,i,l);replayInput.setSample(1,i,r);
        }
        const auto up=os.processSamplesUp(juce::dsp::AudioBlock<float>(replayInput).getSubBlock(0,size_t(count)));
        for(int i=0;i<count;++i)
        {
            std::array<float,2> peak{};
            for(int j=0;j<factor;++j)
            {
                const int k=i*factor+j;const auto index=int64_t((offset+size_t(i))*size_t(factor)+size_t(j));
                primaryEngine.process(up.getSample(0,k),index);
                if(needsSecondaryEngine)secondaryEngine.process(up.getSample(1,k),index);
                peak[0]=juce::jmax(peak[0],primaryEngine.getCurrentLevel());
                if(needsSecondaryEngine)peak[1]=juce::jmax(peak[1],secondaryEngine.getCurrentLevel());
            }
            levels[offset+size_t(i)]=peak;
        }
    }
    result.request=request;
    result.firstValidMarkerIndex=0;
    result.detectorDb0.assign(request.markers.size(),std::numeric_limits<float>::quiet_NaN());
    result.detectorDb1=result.detectorDb0;
    for(size_t marker=0;marker<request.markers.size();++marker)
    {
        // A retained input point belongs to the carrier delay at CAPTURE time.
        // Replaying it with today's Lookahead was the horizontal displacement.
        const double capturedDelay=marker<request.carrierDelays.size() && request.carrierDelays[marker]>=0
            ? request.carrierDelays[marker]*snapshot.sampleRate : double(lookaheadSamples);
        const auto span=marker>0 ? request.markers[marker]-request.markers[marker-1]
            : request.markers.size()>1 ? request.markers[1]-request.markers[0]
            : uint64_t(snapshot.sampleRate/displayRefreshHz);
        const int64_t end=int64_t(request.markers[marker])-int64_t(snapshot.firstCounter)
            +int64_t(std::llround(detectorDelay-capturedDelay));
        const int64_t start=end-int64_t(span);
        if(end>int64_t(levels.size()))continue; // No fabricated future audio at the live edge.
        std::array<float,2> peak{};
        for(int64_t i=std::max(int64_t(0),start);i<end;++i)
        {peak[0]=juce::jmax(peak[0],levels[size_t(i)][0]);peak[1]=juce::jmax(peak[1],levels[size_t(i)][1]);}
        if(request.mode==qqsc::params::stereoLinked && snapshot.stereoKey)peak[0]=juce::jmax(peak[0],peak[1]);
        result.detectorDb0[marker]=detectorLevelToDb(peak[0]);
        result.detectorDb1[marker]=detectorLevelToDb(peak[1]);
    }
    return true;
}

void DynamicDisplay::applyHpfReplay (ReplayResult result)
{
    const auto currentRequest = replayRequestGeneration->load (std::memory_order_relaxed);
    if (result.request.requestGeneration != currentRequest)
        return;

    const auto currentLookaheadMs = qqsc::params::snapLookaheadMs (
        readParameter (processor, qqsc::params::lookaheadMs));
    if (result.request.captureGeneration != lastCaptureGeneration
        || result.request.mode != lastMode
        || result.request.keySource != lastKeySource
        || result.request.oversampling != juce::roundToInt(readParameter(processor,qqsc::params::oversampling))
        || result.request.detectorMode != effectiveDisplayDetectorMode (processor, currentLookaheadMs)
        || std::abs (result.request.detectorWindowMs - effectiveDisplayWindow (processor, currentLookaheadMs)) > 0.001f
        || std::abs (result.request.hpfHz
                     - readParameter (processor, qqsc::params::keyHpfHz,
                                      qqsc::params::keyHpfOffHz)) > 0.01f
        || std::abs (result.request.lookaheadMs - currentLookaheadMs) > 0.01f)
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
            if (index >= result.firstValidMarkerIndex && index < values.size() && std::isfinite(values[index]))
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
    ++historyRevision;
    if (isShowing())
    {
        refreshRenderCaches (lastMode);
        renderedHistoryRevision = historyRevision;
        renderedProjectionRevision = processor.getDisplayProjectionRevision();
        repaint();
    }
}

void DynamicDisplay::drawDomainPanel (juce::Graphics& g, juce::Rectangle<float> panel, int domainIndex,
                                      const juce::String& domainName, int mode)
{
    const auto keySource = juce::roundToInt (readParameter (processor, qqsc::params::keySource));
    const bool externalKey = keySource == qqsc::params::keyExternal;
    const bool externalAvailable = processor.isExternalSidechainBusAvailable();
    const auto& cache = processor.isLimiterMode() ? stereoLimiterCache : renderCaches[static_cast<size_t> (domainIndex)];
    const auto currentGr = cache.currentGainReductionDb;

    if (qqsc::ui::isDarkTheme())
        g.setColour (juce::Colour (0xff1c1d1f));
    else if (qqsc::ui::isClassicTheme())
        g.setColour (qqsc::ui::panelAlt().withAlpha (0.34f));
    else
        g.setGradientFill (juce::ColourGradient (
            qqsc::ui::panel(), panel.getCentreX(), panel.getY(),
            qqsc::ui::panel().interpolatedWith (qqsc::ui::panelAlt(), 0.60f),
            panel.getCentreX(), panel.getBottom(), false));
    g.fillRoundedRectangle (panel, 7.0f);
    g.setColour (qqsc::ui::border().withAlpha (0.45f));
    g.drawRoundedRectangle (panel.reduced (0.5f), 7.0f, 0.8f);

    auto header = panel.reduced (8.0f, 4.0f).removeFromTop (19.0f);
    auto domainArea = header.removeFromLeft (42.0f);
    auto readoutArea = header;

    g.setColour (qqsc::ui::text().withAlpha (0.88f));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawFittedText (domainName, domainArea.toNearestInt(), juce::Justification::centredLeft, 1);

    auto readout = "GAIN (MIX)  " + grText (currentGr);
    if (externalKey && ! externalAvailable)
        readout += "   EXT N/A";

    g.setColour (currentGr < -0.05f ? gainIncreaseColour() : qqsc::ui::grAccent());
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawFittedText (readout, readoutArea.toNearestInt(),
                      juce::Justification::centredRight, 1);

    const auto plot = plotBoundsForPanel (panel);

    const auto gridFloor=processor.isLimiterMode() ? qqsc::params::limiterThresholdMinimumDb : minDb;
    const auto gridStep=processor.isLimiterMode() ? 5.0f : 15.0f;
    for (float db=0.0f; db>=gridFloor; db-=gridStep)
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

    const bool dual = readParameter (processor, qqsc::params::compressionMode) >= 0.5f;
    const auto lowerDb = thresholdDbForDomain (domainIndex, mode);
    const auto upperDb = upperBoundaryDbForDomain (domainIndex, mode);
    const auto parameterDomainIndex = processorDomain (domainIndex, mode);
    const auto boundaryY = [&] (float db,bool upper)
    {
        return getBoundaryYForDomainDb (parameterDomainIndex, db,upper);
    };
    const auto tagY = [&] (float db,bool upper)
    {
        return juce::jlimit (plot.getY(), plot.getBottom() - 16.0f, boundaryY (db,upper) - 8.0f);
    };
    const bool tagsOverlap = std::abs (tagY (lowerDb,false) - tagY (upperDb,true)) < 18.0f;
    const auto drawBoundary = [&] (float db, bool upper)
    {
        const bool rangeOff = ! dual && upper && ! qqsc::params::isRangeEnabled (db);
        const auto y = boundaryY (db,upper);
        const auto colour = dual ? (upper ? qqsc::ui::grAccent() : gainIncreaseColour())
                                  : (upper ? qqsc::ui::grAccent() : qqsc::ui::warmAccent());
        if (! rangeOff)
        {
            const float dashPattern[] { 5.0f, 4.0f };
            g.setColour (colour.withAlpha (0.82f));
            g.drawDashedLine ({ plot.getX(), y, plot.getRight(), y }, dashPattern, 2, 1.0f);
        }

        const juce::String name = dual ? (upper ? "DOWN " : "UP ") : (upper ? "RANGE " : "THR ");
        const auto value = rangeOff ? juce::String ("OFF")
                                    : (processor.isClassicBoundary(upper) || qqsc::params::isThresholdEnabled (db) ? juce::String (db, 1) : juce::String ("-inf"));
        constexpr float tagWidth = 88.0f;
        const auto offset = upper && tagsOverlap ? tagWidth + 3.0f : 0.0f;
        auto tag = juce::Rectangle<float> (plot.getRight() - tagWidth - 2.0f - offset,
                                            tagY (db,upper), tagWidth, 16.0f);
        g.setColour (qqsc::ui::panel().withAlpha (0.90f));
        g.fillRoundedRectangle (tag, 4.0f);
        g.setColour (colour);
        g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
        g.drawFittedText (name + value, tag.toNearestInt(), juce::Justification::centred, 1);
    };
    g.saveState();
    g.reduceClipRegion (plot.toNearestInt());

    // Paint retained paths with level-aligned tones, not rebuilt polygons.
    // Fixed dB anchors avoid colour pumping when history peaks change.
    const auto tracePaint = [&] (juce::Colour colour)
    {
        if (qqsc::ui::isClassicTheme() || qqsc::ui::isDarkTheme())
        {
            g.setColour (colour);
            return;
        }
        const auto alpha = colour.getFloatAlpha();
        juce::ColourGradient tones (
            colour.interpolatedWith (qqsc::ui::panel(), 0.46f).withAlpha (alpha),
            plot.getCentreX(), dbToY (0.0f, plot),
            colour.darker (0.24f).withAlpha (alpha),
            plot.getCentreX(), dbToY (-45.0f, plot), false);
        tones.addColour (0.50, colour.withAlpha (alpha));
        g.setGradientFill (tones);
    };

    if (externalKey && externalAvailable && cache.valid)
    {
        tracePaint (qqsc::ui::cyanAccent().withAlpha (0.10f));
        g.strokePath (cache.externalKeyPath, juce::PathStrokeType (4.0f));
        tracePaint (qqsc::ui::cyanAccent().withAlpha (0.34f));
        g.strokePath (cache.externalKeyPath, juce::PathStrokeType (1.0f));
    }

    // Both signs share the same sparse-shade budget. There is no extra
    // full-area translucent fill when upward and downward processing coexist.
    tracePaint (qqsc::ui::grAccent().withAlpha (0.24f));
    g.strokePath (cache.gainReductionShadePath, juce::PathStrokeType (1.15f));
    tracePaint (gainIncreaseColour().withAlpha (0.24f));
    g.strokePath (cache.gainIncreaseShadePath, juce::PathStrokeType (1.15f));

    tracePaint (qqsc::ui::dryTrace().withAlpha (0.82f));
    g.strokePath (cache.inputPath, juce::PathStrokeType (1.15f));

    tracePaint (qqsc::ui::grAccent().withAlpha (0.90f));
    g.strokePath (cache.gainReductionPath, juce::PathStrokeType (1.25f));
    tracePaint (gainIncreaseColour().withAlpha (0.90f));
    g.strokePath (cache.gainIncreasePath, juce::PathStrokeType (1.25f));

    tracePaint (qqsc::ui::outputAccent().withAlpha (0.96f));
    g.strokePath (cache.outputPath, juce::PathStrokeType (1.5f));
    g.restoreState();
    drawBoundary (lowerDb, false);
    if (dual || !processor.isLimiterMode()) drawBoundary (upperDb, true);
}

juce::Rectangle<float> DynamicDisplay::getLoudnessReadoutBounds() const noexcept
{
    if (!processor.isLimiterMode()) return {};
    const auto mode = displayProcessingMode(processor);
    const bool split = mode != qqsc::params::stereoLinked && !processor.isLimiterMode();
    const auto plot = plotBoundsForPanel(domainPanelBounds(split ? 1 : 0, mode));
    const auto width = juce::jmin(380.0f, plot.getWidth() * .48f);
    const auto height = juce::jmin(split ? 103.0f : 150.0f, plot.getHeight() * .68f);
    return {plot.getX() + plot.getWidth() * .13f,
            plot.getY() + plot.getHeight() * (split ? .27f : .49f), width, height};
}

void DynamicDisplay::drawLoudnessReadout(juce::Graphics& g)
{
    const auto box = getLoudnessReadoutBounds();
    if (box.isEmpty()) return;
    const auto& meters = processor.getMeterState();
    const auto lufs = meters.outputIntegratedLufs.load(std::memory_order_relaxed);
    const auto seconds = meters.outputLoudnessSeconds.load(std::memory_order_relaxed);
    const bool running = meters.outputLoudnessMeasuring.load(std::memory_order_relaxed);
    const bool valid = lufs > -100.0f && std::isfinite(lufs);
    g.setColour(qqsc::ui::panel().withAlpha(.94f));
    g.fillRoundedRectangle(box, 9.0f);
    g.setColour(qqsc::ui::border().withAlpha(.80f));
    g.drawRoundedRectangle(box.reduced(.5f), 9.0f, 1.0f);
    auto content = box.reduced(18.0f, 12.0f);
    auto heading = content.removeFromTop(15.0f);
    g.setColour(qqsc::ui::textMuted());
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText("OUTPUT  /  LUFS-I", heading, juce::Justification::centredLeft);
    auto footer = content.removeFromBottom(14.0f);
    g.setFont(9.0f);
    g.setColour(qqsc::ui::textMuted());
    g.drawText(running ? "MEASURING" : seconds > 0 ? "HOLD" : "READY", footer, juce::Justification::centredLeft);
    const int elapsed = int(seconds);
    const auto duration = juce::String(elapsed / 60) + ":" + juce::String(elapsed % 60).paddedLeft('0', 2);
    g.drawText(duration, footer, juce::Justification::centredRight);
    g.setColour(qqsc::ui::outputAccent());
    g.setFont(juce::Font(juce::FontOptions(juce::jmin(46.0f, content.getHeight() * .86f), juce::Font::bold)));
    g.drawText(valid ? juce::String(lufs, 1) : "--.-", content.withTrimmedRight(62.0f), juce::Justification::centredLeft);
    g.setColour(qqsc::ui::textMuted());
    g.setFont(12.0f);
    g.drawText("LUFS", content.removeFromRight(62.0f), juce::Justification::centredRight);
}

void DynamicDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll (qqsc::ui::canvas());
    g.setColour (qqsc::ui::panel().withAlpha (0.97f));
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (qqsc::ui::border().withAlpha (0.72f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

    const auto mode = displayProcessingMode (processor);

    auto header = bounds.reduced (14.0f, 5.0f).removeFromTop (22.0f);
    auto titleArea = header.removeFromLeft (juce::jmin (290.0f, header.getWidth() * 0.62f));
    g.setColour (qqsc::ui::text().withAlpha (0.84f));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawFittedText ("DYNAMIC LEVEL / GAIN HISTORY", titleArea.toNearestInt(),
                      juce::Justification::centredLeft, 1);
    g.setColour ((hpfReplayBusy ? qqsc::ui::cyanAccent() : qqsc::ui::textMuted())
                    .withAlpha (0.82f));
    g.setFont (9.5f);
    const auto headerStatus = (hpfReplayBusy ? juce::String ("HPF UPDATING   ") : juce::String())
                            + (readParameter (processor, qqsc::params::compressionMode) >= 0.5f ? "DUAL  " : "SINGLE  ")
                            + qqsc::params::modeName (processor.isLimiterMode() ? qqsc::params::stereoLinked : mode);
    g.drawFittedText (headerStatus, header.toNearestInt(),
                      juce::Justification::centredRight, 1);

    if (mode == qqsc::params::stereoLinked || processor.isLimiterMode())
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

    if (processor.isLimiterMode()) drawLoudnessReadout(g);

    const bool externalKey = juce::roundToInt (
        readParameter (processor, qqsc::params::keySource)) == qqsc::params::keyExternal;
    auto legendArea = juce::Rectangle<int> (54, getHeight() - 22, juce::jmax (1, getWidth() - 64), 17);
    const int itemCount = externalKey ? 5 : 4;
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
    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::grAccent(), "Cut / Mix");
    drawLegend (legendArea.removeFromLeft (itemW), gainIncreaseColour(), "Boost / Mix");
    drawLegend (legendArea.removeFromLeft (itemW), qqsc::ui::outputAccent(), "Output");

    if (externalKey)
        drawLegend (legendArea, qqsc::ui::cyanAccent().withAlpha (0.45f), "External key");
}
