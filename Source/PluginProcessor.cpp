#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LimiterModeContinuity.h"
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

void atomicMaxFloat (std::atomic<float>& target, float value) noexcept
{
    auto current = target.load (std::memory_order_relaxed);
    while (value > current
           && ! target.compare_exchange_weak (current, value,
                                              std::memory_order_release,
                                              std::memory_order_relaxed)) {}
}

thread_local const QQSuperCompressionAudioProcessor* boundaryWriteSource = nullptr;

constexpr auto abPrefix = "qqscAB_";
constexpr auto stateSchemaProperty = "qqscStateSchemaVersion";
constexpr auto monitorLRProperty = "qqscMonitorLRSelection";
constexpr auto monitorMSProperty = "qqscMonitorMSSelection";
constexpr auto performanceEcoProperty = "qqscPerformanceEco";
constexpr int oversamplingSchemaVersion = 2; // v0.1.10: 0 ms-only 1x/8x/16x Oversampling schema
constexpr int currentStateSchemaVersion = 30; // Core/Ceiling add 4x; migrate old 1x/8x/16x indices
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

bool loadLastPerformanceEco()
{
    // Construction only, never the audio callback or state restore path.
    juce::PropertiesFile properties(userPreferencesOptions());
    return properties.getBoolValue("lastPerformanceEco",false);
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
    return oldChoice == 0 ? qqsc::params::osNative : qqsc::params::os8x;
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

QQSuperCompressionAudioProcessor::QQSuperCompressionAudioProcessor(std::unique_ptr<juce::PropertiesFile> initialPreferencesOverride)
    : juce::AudioProcessor (BusesProperties()
                               .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                               .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
                               .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "QQSuperCompressionState", createParameterLayout (&classicAlgorithmForText))
{
    ecoMode.store(initialPreferencesOverride != nullptr
        ? initialPreferencesOverride->getBoolValue("lastPerformanceEco",false)
        : loadLastPerformanceEco(),std::memory_order_relaxed);
    // Preallocate every supported Core factor. Switching quality never creates
    // an oversampler in processBlock; 4x adds a lower-cost option in 1.2.36.
    oversamplers[0] = std::make_unique<juce::dsp::Oversampling<float>> (8u);
    for (size_t i = 1; i < oversamplers.size(); ++i)
    {
        const auto stageCount = static_cast<size_t> (qqsc::params::oversamplingStageCountForChoiceIndex (static_cast<int> (i)));
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            8u, stageCount, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    }

    rebuildBoundaryPairs();
    for(auto* parameter:getParameters())
        if(auto* identified=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
            apvts.addParameterListener(identified->paramID,this);
    startTimerHz (30);
    snapshotA = captureCurrentSnapshot();
    snapshotB = snapshotA;
}

QQSuperCompressionAudioProcessor::~QQSuperCompressionAudioProcessor()
{
    stopTimer();
    for(auto* parameter:getParameters())
        if(auto* identified=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
            apvts.removeParameterListener(identified->paramID,this);
}

void QQSuperCompressionAudioProcessor::setEcoMode(bool eco)
{
    if (ecoMode.exchange (eco, std::memory_order_acq_rel) == eco) return;
    updateHostDisplay (juce::AudioProcessor::ChangeDetails().withNonParameterStateChanged (true));
}

void QQSuperCompressionAudioProcessor::rebuildBoundaryPairs() noexcept
{
    static_assert (std::atomic<uint64_t>::is_always_lock_free, "Boundary pairs must be lock-free");
    for (int bank = 0; bank < 4; ++bank)
        for (size_t d = 0; d < 5; ++d)
        {
            const auto& lowerIds = qqsc::params::boundaryBankIds(bank,false);
            const auto& upperIds = qqsc::params::boundaryBankIds(bank,true);
            const auto lower = juce::jlimit (bank>=2 ? qqsc::params::limiterThresholdMinimumDb : -120.0f,
                                           0.0f, apvts.getRawParameterValue (lowerIds[d])->load());
            const auto rawUpper = apvts.getRawParameterValue (upperIds[d])->load();
            const auto upper = bank == 2 ? qqsc::params::rangeOffDb
                : juce::jmax (lower, bank == 0 ? qqsc::params::clampRangeDb (rawUpper)
                                             : juce::jlimit (bank >= 2 ? qqsc::params::limiterThresholdMinimumDb : -120.0f, 0.0f, rawUpper));
            boundaryPairs[static_cast<size_t> (bank) * 5 + d].store (packDisplayStereoSample (lower, upper), std::memory_order_release);
        }
    // Initialisation, A/B recall and project restore preserve stored banks;
    // they are not fresh user/automation Single/Dual switches.
    limiterCompressionModeForAudio.store (
        apvts.getRawParameterValue ("limiterCompressionMode")->load() >= 0.5f ? 1 : 0,
        std::memory_order_release);
}

void QQSuperCompressionAudioProcessor::continueLimiterCompressionMode (bool destinationIsDual) noexcept
{
    const size_t sourceBank = destinationIsDual ? 2u : 3u;
    const size_t destinationBank = destinationIsDual ? 3u : 2u;
    for (size_t d = 0; d < 5; ++d)
    {
        qqsc::LimiterModeBoundaryPair source;
        unpackDisplayStereoSample (boundaryPairs[sourceBank * 5 + d].load (std::memory_order_acquire),
                                   source.lowerDb, source.upperDb);
        auto& destination = boundaryPairs[destinationBank * 5 + d];
        auto old = destination.load (std::memory_order_acquire);
        uint64_t next;
        do
        {
            qqsc::LimiterModeBoundaryPair previous;
            unpackDisplayStereoSample (old, previous.lowerDb, previous.upperDb);
            const auto continued = qqsc::continueLimiterModeBoundaries (destinationIsDual, source, previous);
            const auto floor=qqsc::params::limiterThresholdMinimumDb;
            const auto lower=juce::jlimit(floor,0.0f,continued.lowerDb);
            const auto upper=destinationIsDual ? juce::jlimit(lower,0.0f,continued.upperDb) : qqsc::params::rangeOffDb;
            next = packDisplayStereoSample (lower,upper);
        }
        while (! destination.compare_exchange_weak (old, next, std::memory_order_release, std::memory_order_acquire));
    }
    // No APVTS/host calls here: a mode automation callback may be on the audio
    // thread. The existing timer publishes canonical boundaries to parameters.
    // Makeup, Mix, Output and calibration are already shared by Single/Dual
    // in this Limiter bank. Leave them untouched, regardless of every Link flag.
}

void QQSuperCompressionAudioProcessor::parameterChanged (const juce::String& id, float value)
{
    displayProjectionRevision.fetch_add (1, std::memory_order_relaxed);
    if (id == qqsc::params::distort && boundaryWriteSource != this)
        distortForAudio.store(juce::jlimit(0.0f,100.0f,value),std::memory_order_release);
    if (id == qqsc::params::detectorWindowMs && boundaryWriteSource != this
        && !restoringDynamicsState.load(std::memory_order_acquire))
        distortForAudio.store(qqsc::params::distortForWindowMs(
            readSoundParameter(qqsc::params::lookaheadMs),value),std::memory_order_release);
    if (id == qqsc::params::lookaheadMs || id == "limiterLookaheadMs")
    {
        const bool limiterBank = id == "limiterLookaheadMs";
        const auto next = qqsc::params::snapLookaheadMs(value);
        const auto previous = observedLookahead[limiterBank ? 1u : 0u].exchange(next);
        const auto* messages=juce::MessageManager::getInstanceWithoutCreating();
        const bool undoing=messages!=nullptr && messages->isThisTheMessageThread()
            && undoManager.isPerformingUndoRedo();
        if (next != previous && limiterBank == isLimiterMode() && !undoing
            && !restoringDynamicsState.load(std::memory_order_acquire))
        {
            distortForAudio.store(0.0f,std::memory_order_release);
            if(messages!=nullptr && messages->isThisTheMessageThread())
                setActualParameterValue(qqsc::params::distort,0.0f);
        }
        latencyRefreshPending.store(true,std::memory_order_release);
    }
    if (id == qqsc::params::oversampling || id == "limiterOversampling")
        latencyRefreshPending.store(true,std::memory_order_release);
    if(!restoringDynamicsState.load(std::memory_order_acquire))
    {
        if(id==qqsc::params::limiterMode && value>=0.5f)
            limiterBankInitialised.store(true,std::memory_order_release);
        for(const auto* bankID : qqsc::params::limiterModeIds)
            if(id==bankID) limiterBankInitialised.store(true,std::memory_order_release);
        for(const auto* bankID : qqsc::params::limiterSoundIds)
            if(id==bankID) limiterBankInitialised.store(true,std::memory_order_release);
    }
    if(!applyingCumulativeMatch.load(std::memory_order_acquire)
        && id==qqsc::params::limiterMode)
    {
        matchReady.store(false,std::memory_order_relaxed);
        resetMatchOnNextPlaybackBlock.store(true,std::memory_order_relaxed);
    }
    if(id==qqsc::params::ceilingOversampling || id==qqsc::params::truePeakLimiting)
        latencyRefreshPending.store(true,std::memory_order_release);
    if (id == "limiterCompressionMode")
    {
        // Use callback value, not APVTS's possibly not-yet-updated raw value.
        const int nextMode = value >= 0.5f ? 1 : 0;
        const int previousMode = limiterCompressionModeForAudio.load (std::memory_order_acquire);
        if (nextMode != previousMode && ! restoringDynamicsState.load (std::memory_order_acquire))
        {
            // Undo/redo replays the recorded mode AND companion parameters.
            // Copying again during replay would overwrite the saved values.
            // UndoManager is inspected only on its owning message thread.
            const auto* messages = juce::MessageManager::getInstanceWithoutCreating();
            const bool replaying = messages != nullptr && messages->isThisTheMessageThread()
                                && undoManager.isPerformingUndoRedo();
            if (! replaying)
                continueLimiterCompressionMode (nextMode == 1);
        }
        limiterCompressionModeForAudio.store (nextMode, std::memory_order_release);
        return;
    }
    if (id == qqsc::params::algorithmMode)
    {
        algorithmPreferenceInitialised.store (true);
        classicAlgorithmForText.store (value < 0.5f, std::memory_order_relaxed);
        return;
    }
    if(id=="upAlgorithmMode" || id=="downAlgorithmMode")
        dualAlgorithmPreferenceInitialised[id=="upAlgorithmMode" ? 0 : 1].store(true);
    if (id == qqsc::params::inputOutputLink)
    {
        inputOutputLinkPreferenceInitialised.store (true);
        return;
    }
    if (boundaryWriteSource == this || restoringDynamicsState.load (std::memory_order_acquire))
        return;
    for (int bank = 0; bank < 4; ++bank)
        for (size_t d = 0; d < 5; ++d)
        {
            const auto& lowerIds = qqsc::params::boundaryBankIds(bank,false);
            const auto& upperIds = qqsc::params::boundaryBankIds(bank,true);
            const bool isLower = id == lowerIds[d];
            if (! isLower && id != upperIds[d]) continue;
            value = bank == 2 && !isLower ? qqsc::params::rangeOffDb
                  : bank % 2 == 0 && ! isLower ? qqsc::params::clampRangeDb (value)
                                          : juce::jlimit (bank>=2 ? qqsc::params::limiterThresholdMinimumDb : -120.0f, 0.0f, value);
            auto& pair = boundaryPairs[static_cast<size_t> (bank) * 5 + d];
            auto old = pair.load (std::memory_order_acquire);
            uint64_t next;
            do
            {
                float lower, upper;
                unpackDisplayStereoSample (old, lower, upper);
                if (isLower) { lower = value; upper = juce::jmax (upper, lower); }
                else         { upper = value; lower = juce::jmin (lower, upper); }
                if (bank == 2) upper = qqsc::params::rangeOffDb;
                next = packDisplayStereoSample (lower, upper);
            }
            while (! pair.compare_exchange_weak (old, next, std::memory_order_release, std::memory_order_acquire));
            return;
        }
}

float QQSuperCompressionAudioProcessor::getBoundaryForDomainDb (bool dual, bool upper, int domain) const noexcept
{
    return getBoundaryForBankDb ((isLimiterMode() ? 2 : 0)+(dual ? 1 : 0),upper,domain);
}
float QQSuperCompressionAudioProcessor::getBoundaryForBankDb (int bank, bool upper, int domain) const noexcept
{
    if(bank>=2 && domain==2) domain=1; // Common stereo Limiter boundary.
    float lowerValue,upperValue;
    unpackDisplayStereoSample(boundaryPairs[size_t(bank*5+juce::jlimit(0,4,domain))].load(std::memory_order_acquire),lowerValue,upperValue);
    return upper ? upperValue : lowerValue;
}

float QQSuperCompressionAudioProcessor::effectiveDualRatio (size_t domain, bool upward) const noexcept
{
    const auto* enabled = (upward ? qqsc::params::upEnabledIds : qqsc::params::downEnabledIds)[domain];
    const auto* ratio = (upward ? qqsc::params::upRatioIds : qqsc::params::downRatioIds)[domain];
    return readSoundParameter(enabled) >= 0.5f
        ? (upward ? qqsc::params::upwardRatio (readSoundParameter (ratio), isLimiterMode())
                  : qqsc::params::limiterRatio (readSoundParameter (ratio), isLimiterMode(), true)) : 1.0f;
}

float QQSuperCompressionAudioProcessor::getDynamicsGainForDomain (float level, int domain) const noexcept
{
    if (level <= 1.0e-9f && readSoundParameter(qqsc::params::keySource) >= 0.5f)
        return 1.0f; // Missing/silent external key leaves the carrier unchanged.
    const auto d = static_cast<size_t> (isLimiterMode() && domain==2 ? 1 : juce::jlimit (0, 4, domain));
    const bool dual = readSoundParameter(qqsc::params::compressionMode) >= 0.5f;
    float lower, upper;
    unpackDisplayStereoSample (boundaryPairs[(isLimiterMode() ? 10u : 0u) + (dual ? 5u : 0u) + d].load (std::memory_order_acquire), lower, upper);
    lower = qqsc::params::thresholdLinear (lower);
    upper = dual ? qqsc::params::thresholdLinear (upper) : qqsc::params::rangeLinear (upper);
    const auto algorithm = readSoundParameter(qqsc::params::algorithmMode) >= 0.5f
        ? qqsc::CompressionAlgorithm::super : qqsc::CompressionAlgorithm::classic;
    if (dual)
        return qqsc::StaticCompressionEngine::dualGainForLevel (level,
            effectiveDualRatio (d, true), effectiveDualRatio (d, false), lower, upper,
            readSoundParameter("upAlgorithmMode")<.5f ? qqsc::CompressionAlgorithm::classic : qqsc::CompressionAlgorithm::super,
            readSoundParameter("downAlgorithmMode")<.5f ? qqsc::CompressionAlgorithm::classic : qqsc::CompressionAlgorithm::super);
    return qqsc::StaticCompressionEngine::singleGainForLevel (level,
        effectiveSingleRatio (d), lower, upper, algorithm);
}


void QQSuperCompressionAudioProcessor::fillDynamicsGainForDomain (
    const float* detectorLevels, float* gains, size_t count, int domain) const noexcept
{
    if (detectorLevels == nullptr || gains == nullptr || count == 0)
        return;

    const auto d = static_cast<size_t> (isLimiterMode() && domain==2 ? 1 : juce::jlimit (0, 4, domain));
    const bool limiter = isLimiterMode();
    const bool externalKey = readSoundParameter (qqsc::params::keySource) >= 0.5f;
    const bool dual = readSoundParameter (qqsc::params::compressionMode) >= 0.5f;

    float lowerDb = 0.0f;
    float upperDb = 0.0f;
    unpackDisplayStereoSample (
        boundaryPairs[(limiter ? 10u : 0u) + (dual ? 5u : 0u) + d].load (std::memory_order_acquire),
        lowerDb, upperDb);

    const auto lower = qqsc::params::thresholdLinear (lowerDb);
    const auto upper = dual ? qqsc::params::thresholdLinear (upperDb)
                            : qqsc::params::rangeLinear (upperDb);

    const auto algorithm = readSoundParameter (qqsc::params::algorithmMode) >= 0.5f
        ? qqsc::CompressionAlgorithm::super : qqsc::CompressionAlgorithm::classic;

    float singleRatio = 1.0f;
    float upRatio = 1.0f;
    float downRatio = 1.0f;
    auto upAlgorithm = qqsc::CompressionAlgorithm::classic;
    auto downAlgorithm = qqsc::CompressionAlgorithm::classic;

    if (dual)
    {
        const auto* upEnabled = qqsc::params::upEnabledIds[d];
        const auto* downEnabled = qqsc::params::downEnabledIds[d];
        upRatio = readSoundParameter (upEnabled) >= 0.5f
            ? qqsc::params::upwardRatio (readSoundParameter (qqsc::params::upRatioIds[d]), limiter)
            : 1.0f;
        downRatio = readSoundParameter (downEnabled) >= 0.5f
            ? qqsc::params::limiterRatio (readSoundParameter (qqsc::params::downRatioIds[d]), limiter, true)
            : 1.0f;
        upAlgorithm = readSoundParameter ("upAlgorithmMode") < 0.5f
            ? qqsc::CompressionAlgorithm::classic : qqsc::CompressionAlgorithm::super;
        downAlgorithm = readSoundParameter ("downAlgorithmMode") < 0.5f
            ? qqsc::CompressionAlgorithm::classic : qqsc::CompressionAlgorithm::super;
    }
    else
    {
        singleRatio = qqsc::params::limiterRatio (
            readSoundParameter (qqsc::params::ratioIds[d]), limiter);
    }

    for (size_t i = 0; i < count; ++i)
    {
        const auto level = detectorLevels[i];
        if (level <= 1.0e-9f && externalKey)
        {
            gains[i] = 1.0f; // Missing/silent external key leaves the carrier unchanged.
            continue;
        }

        gains[i] = dual
            ? qqsc::StaticCompressionEngine::dualGainForLevel (
                level, upRatio, downRatio, lower, upper, upAlgorithm, downAlgorithm)
            : qqsc::StaticCompressionEngine::singleGainForLevel (
                level, singleRatio, lower, upper, algorithm);
    }
}

void QQSuperCompressionAudioProcessor::setBoundaryForDomainDb (bool dual, bool upper, int domain, float value)
{
    const auto d = static_cast<size_t> (isLimiterMode() && domain==2 ? 1 : juce::jlimit (0, 4, domain));
    const auto& ids = dual ? (upper ? qqsc::params::downThresholdIds : qqsc::params::upThresholdIds)
                          : (upper ? qqsc::params::rangeIds : qqsc::params::thresholdIds);
    const auto* mappedID=soundParameterID(ids[d]);
    auto* parameter = apvts.getParameter (mappedID);
    const auto normalised = parameter->convertTo0to1 (value);
    parameterChanged (mappedID, parameter->convertFrom0to1 (normalised));
    parameter->setValueNotifyingHost (normalised);
    timerCallback(); // UI edits publish the pushed companion in the same turn.
}

void QQSuperCompressionAudioProcessor::timerCallback()
{
    if (restoringDynamicsState.load (std::memory_order_acquire)) return;
    // Only this non-audio callback publishes collision pushes back to the host.
    // Its own listener callbacks are suppressed on this thread; concurrent host
    // edits still update the canonical pair and are reconciled by the next tick.
    const juce::ScopedValueSetter<const QQSuperCompressionAudioProcessor*> suppress (boundaryWriteSource, this);
    for (int bank = 0; bank < 4; ++bank)
        for (size_t d = 0; d < 5; ++d)
        {
            const auto& lowerIds = qqsc::params::boundaryBankIds(bank,false);
            const auto& upperIds = qqsc::params::boundaryBankIds(bank,true);
            for (bool isUpper : { false, true })
            {
                const auto* id = isUpper ? upperIds[d] : lowerIds[d];
                // Re-read immediately before each companion write, so a host
                // edit between the two writes is incorporated in this tick.
                const auto value = getBoundaryForBankDb (bank, isUpper, static_cast<int> (d));
                if (std::abs (apvts.getRawParameterValue (id)->load() - value) > 0.0001f)
                {
                    auto* parameter = apvts.getParameter (id);
                    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
                }
            }
        }
    const auto distortValue=distortForAudio.load(std::memory_order_acquire);
    if (apvts.getRawParameterValue(qqsc::params::distort)->load()!=distortValue)
        setActualParameterValue(qqsc::params::distort,distortValue);
    refreshMatchResults();
    if(latencyRefreshPending.exchange(false,std::memory_order_acq_rel))
        notifyHostProcessingLatency();
}

void QQSuperCompressionAudioProcessor::writeCanonicalBoundariesTo (juce::ValueTree& state) const
{
    for (auto child : state)
        for (int bank = 0; bank < 4; ++bank)
            for (size_t d = 0; d < 5; ++d)
            {
                const auto id = child.getProperty ("id").toString();
                const auto& lowerIds = qqsc::params::boundaryBankIds(bank,false);
                const auto& upperIds = qqsc::params::boundaryBankIds(bank,true);
                if (id == lowerIds[d] || id == upperIds[d])
                    child.setProperty ("value", getBoundaryForBankDb (bank, id == upperIds[d], static_cast<int> (d)), nullptr);
            }
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
        meterState.displayTruePeakExcessDb.store (0.0f, std::memory_order_relaxed);

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

    return { storage->generation,
             storage->writeCounter.load (std::memory_order_acquire),
             storage->analysisSampleRate };
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

juce::AudioProcessorValueTreeState::ParameterLayout QQSuperCompressionAudioProcessor::createParameterLayout (const std::atomic<bool>* classicForText)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addRatio = [&] (const char* id, const juce::String& name)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            qqsc::params::dynamicsRatioRange(), 1.0f,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
            {
                return qqsc::params::dynamicsRatioText (v);
            }).withValueFromStringFunction (qqsc::params::dynamicsRatioFromText)));
    };

    // Keep the legacy Ratio ID as ST. v1.0.0 appends independent LR/MS Ratio
    // parameters later so older projects keep their established parameter ID.
    addRatio (qqsc::params::ratio, "Ratio ST");

    auto addMakeup = [&] (const char* id, const juce::String& name, float maximum = qqsc::normalMaximumMakeupDb)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { -maximum, maximum, 0.01f }, 0.0f,
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
    // but UI/DSP snap it to the five approved presets. A future new
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

    // Keep Oversampling appended after the 0.1.8 parameter sequence. v1.2.36
    // adds 4x to 1x/8x/16x and retains the 8x default. The candidate applies
    // the selected overall audio rate at every Lookahead.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { qqsc::params::processingMode, 1 }, "Processing Mode",
        qqsc::params::modeChoices(), qqsc::params::stereoLinked));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { qqsc::params::bypass, 1 }, "Bypass", false));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { qqsc::params::oversampling, 1 }, "Oversampling",
        qqsc::params::oversamplingChoices(), qqsc::params::os8x));

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
                .withStringFromValueFunction ([classicForText] (float v, int)
                {
                    return qqsc::params::boundaryText (v, classicForText == nullptr || classicForText->load (std::memory_order_relaxed));
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

    // v1.2.0 append-only host parameter extension. Each mode retains its own
    // boundaries and ratios when switched; ST/LR/MS retain independent banks.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { qqsc::params::compressionMode, 1 }, "Compression Mode",
        juce::StringArray { "Single", "Dual" }, qqsc::params::singleCompression));
    const std::array<juce::String, 5> domainNames { "ST", "L", "R", "M", "S" };
    auto addBoundary = [&] (const char* id, const juce::String& name, float initial)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> { qqsc::params::thresholdOffDb, 0.0f, 0.01f }, initial,
            juce::AudioParameterFloatAttributes().withLabel ("dB")
                .withStringFromValueFunction ([classicForText] (float db, int)
                {
                    return qqsc::params::boundaryText (db, classicForText == nullptr || classicForText->load (std::memory_order_relaxed));
                })
                .withValueFromStringFunction ([] (const juce::String& text)
                {
                    return text.containsIgnoreCase ("-inf") ? qqsc::params::thresholdOffDb
                        : juce::jlimit (qqsc::params::thresholdOffDb, 0.0f, text.getFloatValue());
                })));
    };
    for (size_t d = 0; d < 5; ++d)
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { qqsc::params::rangeIds[d], 1 }, "Range " + domainNames[d],
            qqsc::params::rangeParameterRange(), qqsc::params::rangeOffDb,
            juce::AudioParameterFloatAttributes().withLabel ("dB")
                .withStringFromValueFunction ([classicForText] (float db, int) { return qqsc::params::rangeText (db, classicForText == nullptr || classicForText->load (std::memory_order_relaxed)); })
                .withValueFromStringFunction (qqsc::params::rangeFromText)));
    for (size_t d = 0; d < 5; ++d) addBoundary (qqsc::params::upThresholdIds[d], "Up Threshold " + domainNames[d], qqsc::params::thresholdOffDb);
    for (size_t d = 0; d < 5; ++d) addBoundary (qqsc::params::downThresholdIds[d], "Down Threshold " + domainNames[d], 0.0f);
    for (bool upward : { true, false })
        for (size_t d = 0; d < 5; ++d)
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { upward ? qqsc::params::upRatioIds[d] : qqsc::params::downRatioIds[d], 1 },
                (upward ? "Up Ratio " : "Down Ratio ") + domainNames[d],
                upward ? qqsc::params::dynamicsRatioRange (qqsc::minimumUpRatio, 1.0f)
                       : qqsc::params::dynamicsRatioRange (1.0f, qqsc::normalMaximumDownRatio),
                1.0f,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                { return qqsc::params::dynamicsRatioText (v); })
                .withValueFromStringFunction (qqsc::params::dynamicsRatioFromText)));

    // Workflow preference: saved in the project, independent of A/B sound banks.
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { qqsc::params::dualRatioLink, 1 }, "Up Down Ratio Link", true));
    for (size_t d = 0; d < 5; ++d)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { qqsc::params::upEnabledIds[d], 1 }, "Up Enabled " + domainNames[d], true));
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { qqsc::params::downEnabledIds[d], 1 }, "Down Enabled " + domainNames[d], true));
    }
    // Append after all existing parameters; this workflow preference is shared
    // by A/B sound banks. First use defaults ON; the editor restores the
    // last explicit user choice once unless a project/host edit came first.
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { qqsc::params::inputOutputLink, 1 }, "Input Output Gain Link", true));
    // Preserve every existing host ID/index. This is a sound parameter,
    // saved in both project state and A/B banks, not a global preference.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { qqsc::params::algorithmMode, 1 }, "Compression Algorithm",
        juce::StringArray { "Classic", "Super" }, qqsc::params::classicAlgorithm));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { qqsc::params::limiterMode, 1 }, "Limiter Mode", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { qqsc::params::limiterLink, 1 }, "Limiter Output Link", true));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { qqsc::params::ceilingDb, 1 }, "Ceiling",
        juce::NormalisableRange<float> { -24.0f, 0.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { qqsc::params::limiterOutputDb, 1 }, "Limiter Output Gain",
        juce::NormalisableRange<float> { -120.0f, 120.0f, 0.01f }, 0.0f, juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{qqsc::params::limiterCalibrationDb,1},"Limiter Reference Calibration",
        juce::NormalisableRange<float>{-180.0f,180.0f,0.00001f},0.0f));
    for (size_t i=0; i<35; ++i)
    {
        const int group=int(i/5);
        const auto range=group==0 ? qqsc::params::limiterSingleRange<float>()
            : group==1 ? qqsc::params::dynamicsRatioRange(qqsc::limiterDualMinimumUpRatio,1.0f)
            : group==2 ? qqsc::params::dynamicsRatioRange(qqsc::limiterMinimumDownRatio,qqsc::maximumDownRatio)
            : group==4 ? qqsc::params::rangeParameterRange() : juce::NormalisableRange<float>{-120.0f,0.0f,0.01f};
        // Preserve legacy host normalisation/IDs; canonical Limiter bounds are -45..0.
        const float initial=(group<=2) ? 1.0f : group==4 ? 1.0f : (group==3||group==6) ? 0.0f : qqsc::params::limiterThresholdMinimumDb;
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{qqsc::params::limiterSoundIds[i],1},
            "Limiter "+juce::String(qqsc::params::normalSoundIds[i]),range,initial,
            group<3 ? juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction([](float v,int){return qqsc::params::dynamicsRatioText(v);})
                .withValueFromStringFunction(qqsc::params::dynamicsRatioFromText)
                : (group==3 || group==5 || group==6) ? juce::AudioParameterFloatAttributes().withLabel("dB")
                    .withStringFromValueFunction([](float v,int){return juce::String(juce::jlimit(qqsc::params::limiterThresholdMinimumDb,0.0f,v),2);})
                    .withValueFromStringFunction([](const juce::String& s){return s.containsIgnoreCase("-inf")
                        ? qqsc::params::limiterThresholdMinimumDb : juce::jlimit(qqsc::params::limiterThresholdMinimumDb,0.0f,s.getFloatValue());})
                : juce::AudioParameterFloatAttributes()));
    }
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { qqsc::params::truePeakLimiting, 1 }, "True Peak Limiting", true));
    // Separate mode settings. All old IDs and indices above are unchanged.
    for(size_t i=0;i<31;++i)
    {
        const auto* id=qqsc::params::limiterModeIds[i];
        const auto name="Limiter "+juce::String(qqsc::params::normalModeIds[i]);
        if(i>=1 && i<=5) { addMakeup(id,name,qqsc::maximumMakeupDb); continue; }
        if(i>=6 && i<=10) { addMix(id,name); continue; }
        if(i>=14 && i<=25)
            layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},name,true));
        else if(i==11 || i==12 || i==13 || i==26 || i==29)
        {
            const auto choices=i==11 ? juce::StringArray{"Classic","Super"}
                : i==12 ? juce::StringArray{"Single","Dual"}
                : i==13 ? qqsc::params::modeChoices()
                : i==26 ? qqsc::params::keySourceChoices() : qqsc::params::oversamplingChoices();
            layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,choices,i==29?int(qqsc::params::os8x):i==13?int(qqsc::params::leftRight):0));
        }
        else if(i==30)
            layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,
                juce::NormalisableRange<float>{0.f,100.f,.1f},defaultLookaheadMs,
                juce::AudioParameterFloatAttributes().withLabel("ms")
                    .withStringFromValueFunction([](float v,int){return juce::String(qqsc::params::snapLookaheadMs(v),0)+" ms";})
                    .withValueFromStringFunction([](const juce::String& s){return qqsc::params::snapLookaheadMs(s.getFloatValue());})));
        else
        {
            const auto range=i==28 ? qqsc::params::keyHpfRange() : juce::NormalisableRange<float>{-24.f,24.f,.01f};
            layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,range,
                i==28 ? qqsc::params::keyHpfOffHz : 0.f,
                juce::AudioParameterFloatAttributes().withLabel(i==28 ? "Hz" : "dB")));
        }
    }
    // Append only: all pre-existing host IDs and indices remain stable.
    for(const auto* id:{"upAlgorithmMode","downAlgorithmMode","limiterUpAlgorithmMode","limiterDownAlgorithmMode"})
        layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},juce::String(id),
            juce::StringArray{"Classic","Super"},0));
    // Append-only sound parameter: TP release character. AUTO preserves the
    // verified 1.2.17 timing exactly; old sessions migrate to AUTO.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{qqsc::params::tpRecoveryMode,1},"TP Recovery",
        juce::StringArray{"Tight","Auto","Smooth"},qqsc::params::tpAuto));
    // 1.2.34 append-only Limiter Ceiling quality selector. 8x is the
    // compatibility default; TP promotes 1x to 8x.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{qqsc::params::ceilingOversampling,2},"Ceiling Oversampling",
        qqsc::params::ceilingOversamplingChoices(),qqsc::params::ceiling8x));
    // Append-only: both modes use the same static transfer law. MIN retains
    // the min-of-windows behaviour for saved projects.
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{qqsc::params::detectorMode,1},"Detector",
        juce::StringArray{"Normal","Safe"},0));
    // Append after Detector; existing host parameter indices are unchanged.
    // 100 is capped to the active Lookahead, preserving the complete legacy
    // window on fresh instances and when loading pre-WINDOW sessions.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{qqsc::params::detectorWindowMs,1},"Detector Window",
        juce::NormalisableRange<float>{0.0f,100.0f,0.01f},100.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{qqsc::params::distort,1},"Distort",
        juce::NormalisableRange<float>{0.0f,100.0f,0.01f},0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{qqsc::params::limiterStereoLink,1},"Limiter L/R Link",
        juce::NormalisableRange<float>{0.0f,100.0f,0.01f},0.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    return layout;
}

void QQSuperCompressionAudioProcessor::initialiseInputOutputLinkPreference (bool enabled)
{
    if (! inputOutputLinkPreferenceInitialised.exchange (true))
        setActualParameterValue (qqsc::params::inputOutputLink, enabled ? 1.0f : 0.0f);
}

void QQSuperCompressionAudioProcessor::initialiseAlgorithmPreference (int algorithm)
{
    if (! algorithmPreferenceInitialised.exchange (true))
    {
        algorithm = juce::jlimit (0, 1, algorithm);
        setActualParameterValue (qqsc::params::algorithmMode, float(algorithm));
        const juce::ScopedLock lock (abLock);
        snapshotA.algorithmMode = snapshotB.algorithmMode = algorithm;
    }
}

void QQSuperCompressionAudioProcessor::initialiseDualRatioLinkPreference (bool enabled)
{
    // A restored host state or a previously opened instance takes precedence
    // over the global last-click preference used for genuinely new instances.
    if (! dualRatioLinkPreferenceInitialised.exchange (true))
        setActualParameterValue (qqsc::params::dualRatioLink, enabled ? 1.0f : 0.0f);
}

void QQSuperCompressionAudioProcessor::initialiseDualAlgorithmPreferences (int up, int down)
{
    for(size_t i=0;i<2;++i)
        if(!dualAlgorithmPreferenceInitialised[i].exchange(true))
        {
            const auto value=juce::jlimit(0,1,i==0 ? up : down);
            setActualParameterValue(i==0 ? "upAlgorithmMode" : "downAlgorithmMode",float(value));
            const juce::ScopedLock lock(abLock);
            if(i==0) snapshotA.upAlgorithmMode=snapshotB.upAlgorithmMode=value;
            else snapshotA.downAlgorithmMode=snapshotB.downAlgorithmMode=value;
        }
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

    wetBaseBuffer.setSize (8, configuredMaximumBlockSize, false, true, true);
    wetBaseBuffer.clear();
    oversamplingInputBuffer.setSize (8, configuredMaximumBlockSize, false, true, true);
    mixControlBuffer.setSize (16, configuredMaximumBlockSize, false, true, true);
    abActive = false;
    abTransferPending.store (false);
    oversamplingInputBuffer.clear();
    keyInputBuffer.setSize (2, configuredMaximumBlockSize, false, true, true);
    keyInputBuffer.clear();
    originalInputBuffer.setSize (2, configuredMaximumBlockSize, false, true, true);
    originalInputBuffer.clear();

    // Reserve the maximum 100 ms window at 16x; processBlock never allocates.
    maxLookaheadSamplesBase = juce::jmax (0, static_cast<int> (std::ceil (currentSampleRate * 0.100)));
    maxLookaheadSamplesInternal = maxLookaheadSamplesBase * qqsc::params::oversamplingFactors.back();
    // Rebuilding the two-sided detector needs 2N input history. This buffer
    // capacity does not change the N-sample audio read position or host latency.
    oversampledDelayCapacity = juce::jmax (2, 2 * maxLookaheadSamplesInternal + 2);
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
    truePeakOversampler.initProcessing(size_t(configuredMaximumBlockSize));
    truePeakOversampler.reset();
    truePeakBuffer.setSize(2,configuredMaximumBlockSize);
    const auto initialCeilingChoice=getEffectiveCeilingOversamplingChoice();
    outputCeiling.setStereoLink(readSoundParameter(qqsc::params::limiterStereoLink)*.01f);
    outputCeiling.prepare(currentSampleRate,isTruePeakSelected(),readSoundParameter(qqsc::params::ceilingDb),
                          getTpRecoveryMode(),initialCeilingChoice,configuredMaximumBlockSize);
    // Fixed-capacity reference ring prevents quality changes from allocating on
    // the audio thread. 16x full latency is the maximum possible requirement.
    const auto maxCeilingLatency=outputCeiling.latencySamplesForChoice(qqsc::params::ceiling16x);
    ceilingReferenceDelay.assign(size_t(maxCeilingLatency+2),{});
    ceilingReferenceIndex=0;
    ceilingReferenceDelaySamples=0;
    currentCeilingOversamplingChoice=initialCeilingChoice;
    currentCeilingSharesCoreOversampling=false;
    previousUiAnalysisEnabled=shouldRunUiAnalysis();
    outputLoudness.prepare(currentSampleRate);
    lufsWasMeasuring=false;
    meterState.outputIntegratedLufs.store(-120.0f,std::memory_order_relaxed);
    meterState.outputLoudnessSeconds.store(0.0f,std::memory_order_relaxed);
    meterState.outputLoudnessMeasuring.store(false,std::memory_order_relaxed);
    truePeakHold=-120.0f; truePeakRemaining=0;
    truePeakResetRequested.store(false,std::memory_order_relaxed);
    meterState.truePeakHoldDb.store(-120.0f,std::memory_order_relaxed);
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
    unityOutputSmoother.reset(currentSampleRate,.010);
    unityOutputSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        isLimiterMode() ? readSoundParameter(qqsc::params::limiterOutputDb) : 0.f));
    unityMonitorFade.reset(currentSampleRate,.020);
    unityMonitorFade.setCurrentAndTargetValue(isUnityMonitorActive() ? 1.f : 0.f);
    bypassFade.reset(currentSampleRate,.020);bypassInitialised=false;
    abLastUnityOutput=abFrozenUnityOutput=unityOutputSmoother.getCurrentValue();

    inputGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        readSoundParameter(qqsc::params::inputGainDb)));
    keyGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        readSoundParameter(qqsc::params::keyGainDb)));
    const auto initialKeyHpfHz = readSoundParameter(qqsc::params::keyHpfHz);
    keyHpfCutoffSmoother.setCurrentAndTargetValue (
        qqsc::params::isKeyHpfEnabled (initialKeyHpfHz)
            ? qqsc::params::clampKeyHpfHz (initialKeyHpfHz)
            : qqsc::params::keyHpfMinHz);
    keyHpfWetSmoother.setCurrentAndTargetValue (
        qqsc::params::isKeyHpfEnabled (initialKeyHpfHz) ? 1.0f : 0.0f);
    updateKeyHighPassCoefficients (keyHpfCutoffSmoother.getCurrentValue());
    resetKeyHighPassState();
    ratioSmoother.setCurrentAndTargetValue (effectiveSingleRatio (0));
    ratioLSmoother.setCurrentAndTargetValue (effectiveSingleRatio (1));
    ratioRSmoother.setCurrentAndTargetValue (effectiveSingleRatio (2));
    ratioMSmoother.setCurrentAndTargetValue (effectiveSingleRatio (3));
    ratioSSmoother.setCurrentAndTargetValue (effectiveSingleRatio (4));
    for (size_t d = 0; d < 5; ++d)
    {
        upRatioSmoothers[d].reset (currentSampleRate, 0.010);
        downRatioSmoothers[d].reset (currentSampleRate, 0.010);
        upRatioSmoothers[d].setCurrentAndTargetValue (qqsc::params::upwardRatio (readSoundParameter (qqsc::params::upRatioIds[d]), isLimiterMode()));
        upEnableFades[d].reset (currentSampleRate, 0.010);
        upEnableFades[d].setCurrentAndTargetValue (readSoundParameter(qqsc::params::upEnabledIds[d]));
        downRatioSmoothers[d].setCurrentAndTargetValue (qqsc::params::limiterRatio (readSoundParameter (qqsc::params::downRatioIds[d]), isLimiterMode(), true));
        downEnableFades[d].reset (currentSampleRate, 0.010);
        downEnableFades[d].setCurrentAndTargetValue (readSoundParameter(qqsc::params::downEnabledIds[d]));
    }
    makeupSTSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::makeupGainDb));
    makeupLSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::makeupGainLDb));
    makeupRSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::makeupGainRDb));
    makeupMSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::makeupGainMDb));
    makeupSSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::makeupGainSDb));
    mixSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::mix) * 0.01f);
    mixLSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::mixL) * 0.01f);
    mixRSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::mixR) * 0.01f);
    mixMSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::mixM) * 0.01f);
    mixSSmoother.setCurrentAndTargetValue (readSoundParameter(qqsc::params::mixS) * 0.01f);
    outputGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (
        getActiveOutputGainDb()));

    gainReductionHoldDurationSamples = juce::jmax<int64_t> (1, static_cast<int64_t> (std::llround (currentSampleRate * 2.0)));
    displayCoreDelaySeconds.store(float((currentTotalLatencySamples+getCurrentCeilingAdditionalLatency())/currentSampleRate),std::memory_order_release);
    resetGainReductionHold();

    currentLookaheadSamplesBase = -1;
    currentLookaheadSamplesInternal = -1;
    currentOversamplingIndex = -1;
    currentKeySource = -1;
    currentOversamplingFactor = 1;
    currentTotalLatencySamples = 0;
    currentLimiterCeilingActive = false;
    stoppedQuietOutputSamples=0;stoppedSilentSamples=0;stoppedDspSleeping=false;stoppedFastPathBlocks=0;ecoTransportSuspended=false;
    performancePhase=-1;
    for(auto& w:performanceWindows)
    {
        w.blocks.store(0,std::memory_order_relaxed);w.ticks.store(0,std::memory_order_relaxed);
        w.sleepBlocks.store(0,std::memory_order_relaxed);w.zeroBlocks.store(0,std::memory_order_relaxed);
        w.playingBlocks.store(0,std::memory_order_relaxed);w.unknownBlocks.store(0,std::memory_order_relaxed);
    }
    updateProcessingConfiguration (true);

    loudnessMatch.prepare (currentSampleRate);
    unityMixMatch.prepare(currentSampleRate);
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

int QQSuperCompressionAudioProcessor::getEffectiveCeilingOversamplingChoice() const noexcept
{
    return juce::jlimit(0,3,juce::roundToInt(readSoundParameter(qqsc::params::oversampling)));
}

bool QQSuperCompressionAudioProcessor::shouldShareCeilingOversampling (int coreFactor,int ceilingChoice,int lookaheadSamplesBase) const noexcept
{
    juce::ignoreUnused(lookaheadSamplesBase);
    if(!isLimiterMode() || coreFactor<=1) return false;
    return qqsc::params::ceilingOversamplingFactorForChoiceIndex(ceilingChoice)==coreFactor;
}

int QQSuperCompressionAudioProcessor::getCurrentCeilingAdditionalLatency() const noexcept
{
    if(!currentLimiterCeilingActive) return 0;
    return currentCeilingSharesCoreOversampling ? outputCeiling.sharedLatencySamples() : outputCeiling.latencySamples();
}

int QQSuperCompressionAudioProcessor::getCombinedLatencySamples (float requestedLookaheadMs,
                                                                  int oversamplingIndex) const noexcept
{
    const auto presetMs = qqsc::params::snapLookaheadMs (requestedLookaheadMs);
    const auto lookaheadSamples = juce::jmax (0, static_cast<int> (
        std::round (currentSampleRate * static_cast<double> (presetMs) * 0.001)));
    const auto effectiveOversamplingIndex = qqsc::params::effectiveOversamplingChoiceIndex (presetMs, oversamplingIndex);
    const auto coreFactor=qqsc::params::oversamplingFactorForChoiceIndex(effectiveOversamplingIndex);
    int ceilingLatency=0;
    if(isLimiterMode())
    {
        const auto ceilingChoice=getEffectiveCeilingOversamplingChoice();
        ceilingLatency=shouldShareCeilingOversampling(coreFactor,ceilingChoice,lookaheadSamples)
            ? outputCeiling.sharedLatencySamplesForChoice(ceilingChoice)
            : outputCeiling.latencySamplesForChoice(ceilingChoice);
    }
    return lookaheadSamples + getOversamplingLatencySamples (effectiveOversamplingIndex) + ceilingLatency;
}

void QQSuperCompressionAudioProcessor::setLookaheadFromEditor(float ms)
{
    timerCallback(); // Finish older pending companion edits before this transaction.
    undoManager.beginNewTransaction("Lookahead / Distort");
    auto* look=apvts.getParameter(soundParameterID(qqsc::params::lookaheadMs));
    auto* colour=apvts.getParameter(qqsc::params::distort);
    look->beginChangeGesture(); colour->beginChangeGesture();
    look->setValueNotifyingHost(look->convertTo0to1(qqsc::params::snapLookaheadMs(ms)));
    timerCallback(); // Record the reset in the same undo transaction.
    colour->endChangeGesture(); look->endChangeGesture();
    notifyHostProcessingLatency();
}

void QQSuperCompressionAudioProcessor::notifyHostProcessingLatency()
{
    const auto currentLookaheadMs = qqsc::params::snapLookaheadMs (
        readSoundParameter(qqsc::params::lookaheadMs));
    const auto storedOversamplingChoice = juce::jlimit (0, 3,
        juce::roundToInt (readSoundParameter(qqsc::params::oversampling)));
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

// Conservative amplitude bound for stopped FULL residuals, including upward
// compression and gains before/after Ceiling. Never used to gate active audio.
double QQSuperCompressionAudioProcessor::fullIdleGainBound() const noexcept
{
    const auto gain=[](float db){return std::pow(10.0,double(db)*.05);};
    const auto input=gain(readSoundParameter(qqsc::params::inputGainDb));
    const auto output=gain(getActiveOutputGainDb());
    const auto unity=isLimiterMode() ? juce::jmax(1.0,gain(-readSoundParameter(qqsc::params::limiterOutputDb))) : 1.0;
    const bool dual=readSoundParameter(qqsc::params::compressionMode)>=.5f;
    const bool super=readSoundParameter(dual ? "upAlgorithmMode" : qqsc::params::algorithmMode)>=.5f;
    const int mode=juce::roundToInt(readSoundParameter(qqsc::params::processingMode));
    const int first=mode==qqsc::params::leftRight ? 1 : mode==qqsc::params::midSide ? 3 : 0;
    const int last=first==0 ? 0 : first+1;
    const char* makeupIds[]={"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"};
    double domainBound=1.0;
    for(int d=first;d<=last;++d)
    {
        const float ratio=dual ? qqsc::params::upwardRatio(readSoundParameter(qqsc::params::upRatioIds[size_t(d)]),isLimiterMode())
                               : effectiveSingleRatio(size_t(d));
        const bool upward=ratio<1.f && (!dual || readSoundParameter(qqsc::params::upEnabledIds[size_t(d)])>.5f);
        const double boost=!upward ? 1.0 : super ? 1.0/double(juce::jmax(qqsc::minimumUpRatio,ratio))
                                                : double(qqsc::maximumUpwardGain);
        domainBound=juce::jmax(domainBound,boost*gain(readSoundParameter(makeupIds[d])));
    }
    // Reserve 12 dB for M/S summing and filter excursions, and include bypass.
    return 4.0*juce::jmax(1.0,input*domainBound*output*unity);
}

// A transport stop is a deliberate discontinuity in ECO. Discard the old
// carrier/ceiling history before processing the first resumed block; never emit
// cached pre-stop audio. Reuse prepared storage and the existing PDC.
void QQSuperCompressionAudioProcessor::resumeFromEcoTransportStop(bool forceBypass) noexcept
{
    abTransferPending.store(false,std::memory_order_release);
    updateProcessingConfiguration(true);
    inputGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(readSoundParameter(qqsc::params::inputGainDb)));
    keyGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(readSoundParameter(qqsc::params::keyGainDb)));
    const auto hz=readSoundParameter(qqsc::params::keyHpfHz);
    keyHpfCutoffSmoother.setCurrentAndTargetValue(qqsc::params::isKeyHpfEnabled(hz) ? qqsc::params::clampKeyHpfHz(hz) : qqsc::params::keyHpfMinHz);
    keyHpfWetSmoother.setCurrentAndTargetValue(qqsc::params::isKeyHpfEnabled(hz) ? 1.f : 0.f);
    updateKeyHighPassCoefficients(keyHpfCutoffSmoother.getCurrentValue());
    const std::array<juce::SmoothedValue<float>*,5> makeups{&makeupSTSmoother,&makeupLSmoother,&makeupRSmoother,&makeupMSmoother,&makeupSSmoother};
    const std::array<juce::SmoothedValue<float>*,5> mixes{&mixSmoother,&mixLSmoother,&mixRSmoother,&mixMSmoother,&mixSSmoother};
    const char* makeupIds[]={"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"};
    const char* mixIds[]={"mix","mixL","mixR","mixM","mixS"};
    for(size_t d=0;d<5;++d)
    {
        makeups[d]->setCurrentAndTargetValue(readSoundParameter(makeupIds[d]));
        mixes[d]->setCurrentAndTargetValue(readSoundParameter(mixIds[d])*.01f);
    }
    outputGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(getActiveOutputGainDb()));
    unityOutputSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(isLimiterMode() ? readSoundParameter(qqsc::params::limiterOutputDb) : 0.f));
    unityMonitorFade.setCurrentAndTargetValue(isUnityMonitorActive() ? 1.f : 0.f);
    bypassFade.setCurrentAndTargetValue(forceBypass ? 1.f : 0.f);bypassInitialised=true;
    abLastUnityOutput=abFrozenUnityOutput=unityOutputSmoother.getCurrentValue();
    truePeakOversampler.reset();truePeakBuffer.clear();
    truePeakHold=-120.f;truePeakRemaining=0;
    resetMatchAccumulator();matchReady.store(false,std::memory_order_relaxed);
    outputLoudness.reset();
    stoppedQuietOutputSamples=0;stoppedSilentSamples=0;stoppedDspSleeping=false;ecoTransportSuspended=false;
}

void QQSuperCompressionAudioProcessor::updateProcessingConfiguration (bool force)
{
    const auto requestedMs = qqsc::params::snapLookaheadMs (
        readSoundParameter(qqsc::params::lookaheadMs));
    const auto requestedLookaheadBase = juce::jlimit (0, maxLookaheadSamplesBase,
        static_cast<int> (std::round (currentSampleRate * static_cast<double> (requestedMs) * 0.001)));
    const auto storedOversamplingChoice = juce::jlimit (0, 3,
        juce::roundToInt (readSoundParameter(qqsc::params::oversampling)));
    const auto requestedOversamplingIndex = qqsc::params::effectiveOversamplingChoiceIndex (requestedMs, storedOversamplingChoice);
    const auto requestedKeySource = juce::jlimit (static_cast<int> (qqsc::params::keyInternal),
                                                  static_cast<int> (qqsc::params::keyExternal),
                                                  juce::roundToInt (readSoundParameter(qqsc::params::keySource)));
    const auto requestedFactor = qqsc::params::oversamplingFactorForChoiceIndex (requestedOversamplingIndex);
    const auto requestedLookaheadInternal = requestedLookaheadBase * requestedFactor;
    const auto requestedWindowMs = juce::jlimit (0.0f, requestedMs,
        readSoundParameter (qqsc::params::detectorWindowMs));
    const auto requestedWindowInternal = juce::jlimit (0, requestedLookaheadInternal,
        static_cast<int> (std::round (currentSampleRate * requestedFactor * double(requestedWindowMs) * 0.001)));
    const auto requestedOversamplingLatency = getOversamplingLatencySamples (requestedOversamplingIndex);
    const auto requestedTotalLatency = requestedLookaheadBase + requestedOversamplingLatency;
    const bool requestedLimiterCeilingActive = isLimiterMode();
    const auto requestedCeilingChoice = getEffectiveCeilingOversamplingChoice();
    const bool requestedCeilingShare = requestedLimiterCeilingActive
        && shouldShareCeilingOversampling(requestedFactor,requestedCeilingChoice,requestedLookaheadBase);

    const bool oversamplingChanged = requestedOversamplingIndex != currentOversamplingIndex;
    const bool lookaheadChanged = requestedLookaheadBase != currentLookaheadSamplesBase;
    const bool windowChanged = requestedWindowInternal != currentDetectorWindowSamplesInternal;
    const bool keySourceChanged = requestedKeySource != currentKeySource;
    const bool limiterCeilingChanged = requestedLimiterCeilingActive != currentLimiterCeilingActive;
    const bool ceilingOversamplingChanged = requestedLimiterCeilingActive
        && requestedCeilingChoice != currentCeilingOversamplingChoice;
    const bool ceilingShareChanged = requestedCeilingShare != currentCeilingSharesCoreOversampling;

    if (! force && ! oversamplingChanged && ! lookaheadChanged && ! windowChanged && ! keySourceChanged
        && ! limiterCeilingChanged && ! ceilingOversamplingChanged && ! ceilingShareChanged)
        return;

    const auto ratioCurrent = ratioSmoother.getCurrentValue();
    const auto ratioLCurrent = ratioLSmoother.getCurrentValue();
    const auto ratioRCurrent = ratioRSmoother.getCurrentValue();
    const auto ratioMCurrent = ratioMSmoother.getCurrentValue();
    const auto ratioSCurrent = ratioSSmoother.getCurrentValue();
    const auto ratioTarget = qqsc::params::limiterRatio (readSoundParameter (qqsc::params::ratio), isLimiterMode());
    const auto ratioLTarget = qqsc::params::limiterRatio (readSoundParameter (qqsc::params::ratioL), isLimiterMode());
    const auto ratioRTarget = qqsc::params::limiterRatio (readSoundParameter (qqsc::params::ratioR), isLimiterMode());
    const auto ratioMTarget = qqsc::params::limiterRatio (readSoundParameter (qqsc::params::ratioM), isLimiterMode());
    const auto ratioSTarget = qqsc::params::limiterRatio (readSoundParameter (qqsc::params::ratioS), isLimiterMode());

    currentOversamplingIndex = requestedOversamplingIndex;
    currentKeySource = requestedKeySource;
    currentOversamplingFactor = requestedFactor;
    currentLookaheadSamplesBase = requestedLookaheadBase;
    currentLookaheadSamplesInternal = requestedLookaheadInternal;
    currentDetectorWindowSamplesInternal = requestedWindowInternal;
    currentTotalLatencySamples = requestedTotalLatency;
    displayCoreDelaySeconds.store(float(requestedTotalLatency/currentSampleRate),std::memory_order_release);
    currentLimiterCeilingActive = requestedLimiterCeilingActive;
    currentCeilingOversamplingChoice = requestedCeilingChoice;
    currentCeilingSharesCoreOversampling = requestedCeilingShare;

    if (force || oversamplingChanged || lookaheadChanged || windowChanged || keySourceChanged)
    {
        leftEngine.setLookaheadSamples (requestedWindowInternal);
        rightEngine.setLookaheadSamples (requestedWindowInternal);
        midEngine.setLookaheadSamples (requestedWindowInternal);
        sideEngine.setLookaheadSamples (requestedWindowInternal);
    }

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

    abFade.reset (currentSampleRate * currentOversamplingFactor, 0.020);
    abFade.setCurrentAndTargetValue (1.0f);
    abActive = false;
    resetRatioSmoother(limiterStereoLinkSmoother,limiterStereoLinkSmoother.getCurrentValue(),
                       readSoundParameter(qqsc::params::limiterStereoLink)*.01f);
    resetRatioSmoother (algorithmFade, algorithmFade.getCurrentValue(),
                       readSoundParameter(qqsc::params::algorithmMode));
    resetRatioSmoother (detectorModeFade, detectorModeFade.getCurrentValue(),
                       readSoundParameter(qqsc::params::detectorMode));
    resetRatioSmoother (upAlgorithmFade, upAlgorithmFade.getCurrentValue(),readSoundParameter("upAlgorithmMode"));
    resetRatioSmoother (downAlgorithmFade, downAlgorithmFade.getCurrentValue(),readSoundParameter("downAlgorithmMode"));
    resetRatioSmoother (ratioSmoother,  ratioCurrent,  ratioTarget);
    resetRatioSmoother (ratioLSmoother, ratioLCurrent, ratioLTarget);
    resetRatioSmoother (ratioRSmoother, ratioRCurrent, ratioRTarget);
    resetRatioSmoother (ratioMSmoother, ratioMCurrent, ratioMTarget);
    resetRatioSmoother (ratioSSmoother, ratioSCurrent, ratioSTarget);
    for (size_t d = 0; d < 5; ++d)
    {
        resetRatioSmoother (upRatioSmoothers[d], upRatioSmoothers[d].getCurrentValue(),
                           qqsc::params::upwardRatio (readSoundParameter (qqsc::params::upRatioIds[d]), isLimiterMode()));
        resetRatioSmoother (upEnableFades[d], upEnableFades[d].getCurrentValue(),
                           readSoundParameter(qqsc::params::upEnabledIds[d]));
        resetRatioSmoother (downRatioSmoothers[d], downRatioSmoothers[d].getCurrentValue(),
                           qqsc::params::limiterRatio (readSoundParameter (qqsc::params::downRatioIds[d]), isLimiterMode(), true));
        resetRatioSmoother (downEnableFades[d], downEnableFades[d].getCurrentValue(),
                           readSoundParameter(qqsc::params::downEnabledIds[d]));
    }


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
    else if (lookaheadChanged || windowChanged)
    {
        // Same internal rate/source, different future-window length: rebuild all
        // four queues from the exact stored detector-source domains.
        resetDetectorCoreState();
        const auto thresholdL = qqsc::params::thresholdLinear (
            readSoundParameter(qqsc::params::thresholdLDb));
        const auto thresholdR = qqsc::params::thresholdLinear (
            readSoundParameter(qqsc::params::thresholdRDb));
        const auto thresholdM = qqsc::params::thresholdLinear (
            readSoundParameter(qqsc::params::thresholdMDb));
        const auto thresholdS = qqsc::params::thresholdLinear (
            readSoundParameter(qqsc::params::thresholdSDb));

        const int detectorOffset = requestedLookaheadInternal - requestedWindowInternal;
        for (int age = detectorOffset + 2 * requestedWindowInternal; age > detectorOffset; --age)
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

    if (force || limiterCeilingChanged || ceilingOversamplingChanged || ceilingShareChanged || lookaheadChanged)
    {
        // 1.2.34: quality changes may reset/reconfigure the Ceiling and PDC,
        // but the 4x/8x/16x objects are already prepared. TP toggling within one
        // quality remains target-only and does not rebuild filters.
        outputCeiling.selectOversamplingChoice(currentCeilingOversamplingChoice,isTruePeakSelected(),
            readSoundParameter(qqsc::params::ceilingDb),getTpRecoveryMode());
        if (! ceilingOversamplingChanged)
            outputCeiling.resetForLimiter(isTruePeakSelected(),readSoundParameter(qqsc::params::ceilingDb),getTpRecoveryMode());
        ceilingReferenceDelaySamples=getCurrentCeilingAdditionalLatency();
        std::fill (ceilingReferenceDelay.begin(), ceilingReferenceDelay.end(), std::array<float,10>{});
        ceilingReferenceIndex = 0;
    }

    displayCoreDelaySeconds.store(float((currentTotalLatencySamples+getCurrentCeilingAdditionalLatency())/currentSampleRate),std::memory_order_release);
    resetGainReductionHold();
    setLatencySamples (currentTotalLatencySamples + getCurrentCeilingAdditionalLatency());
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
    const auto bypassed = readSoundParameter(qqsc::params::bypass) >= 0.5f;
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

    const auto performanceStart = juce::Time::getHighResolutionTicks();
    const bool ecoForBlock=isEcoMode();
    const int performancePhaseForBlock = (ecoForBlock ? 2 : 0) + (isEditorOpen() ? 1 : 0);
    updateProcessingConfiguration();

    const bool uiAnalysisEnabled=shouldRunUiAnalysis();
    outputCeiling.setAnalysisEnabled(uiAnalysisEnabled);
    if(uiAnalysisEnabled!=previousUiAnalysisEnabled)
    {
        if(uiAnalysisEnabled)
        {
            truePeakOversampler.reset();truePeakBuffer.clear();
            truePeakHold=-120.0f;truePeakRemaining=0;
            displayCoreDelaySeconds.store(float((currentTotalLatencySamples+getCurrentCeilingAdditionalLatency())/currentSampleRate),std::memory_order_release);
    resetGainReductionHold();
            resetMatchAccumulator();matchReady.store(false,std::memory_order_relaxed);outputLoudness.reset();
            meterState.displayTruePeakExcessDb.store(0.0f,std::memory_order_relaxed);
        }
        else
        {
            matchReady.store(false,std::memory_order_relaxed);
            meterState.inputDb0.store(-120.0f,std::memory_order_relaxed);
            meterState.inputDb1.store(-120.0f,std::memory_order_relaxed);
            meterState.outputDb0.store(-120.0f,std::memory_order_relaxed);
            meterState.outputDb1.store(-120.0f,std::memory_order_relaxed);
            meterState.gainReductionDb0.store(0.0f,std::memory_order_relaxed);
            meterState.gainReductionDb1.store(0.0f,std::memory_order_relaxed);
            meterState.gainReductionHoldDb0.store(0.0f,std::memory_order_relaxed);
            meterState.gainReductionHoldDb1.store(0.0f,std::memory_order_relaxed);
            meterState.truePeakHoldDb.store(-120.0f,std::memory_order_relaxed);
            meterState.keyInputDb.store(-120.0f,std::memory_order_relaxed);
            meterState.outputIntegratedLufs.store(-120.0f,std::memory_order_relaxed);
            meterState.outputLoudnessSeconds.store(0.0f,std::memory_order_relaxed);
            meterState.outputLoudnessMeasuring.store(false,std::memory_order_relaxed);
        }
        previousUiAnalysisEnabled=uiAnalysisEnabled;
    }

    bool transportAvailable = false;
    bool transportPlaying = false;
    bool transportRecording = false;
    const bool offlineRendering=isNonRealtime();
    int64_t transportSample = -1;
    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            transportAvailable = true;
            transportPlaying = position->getIsPlaying();
            transportRecording = position->getIsRecording();
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

    // Output LUFS retains its seek/loop reset. MATCH below instead keeps
    // the complete uninterrupted playback capture. Hosts with no transport
    // information are metered continuously while Limiter is on.
    const bool measureOutputLoudness = uiAnalysisEnabled && isLimiterMode() && (!transportAvailable || transportPlaying);
    if (measureOutputLoudness && (!lufsWasMeasuring || discontinuity))
        outputLoudness.reset();
    lufsWasMeasuring = measureOutputLoudness;
    meterState.outputLoudnessMeasuring.store(measureOutputLoudness,std::memory_order_relaxed);

    // A Match click never starts a new measurement pass. Seeking/looping
    // inside uninterrupted playback remains part of that playback capture.
    if(transportPlaying && !lastTransportPlaying)
        matchPassComplete.store(uiAnalysisEnabled,std::memory_order_release);
    else if(transportPlaying && !uiAnalysisEnabled)
        matchPassComplete.store(false,std::memory_order_release);
    const bool matchResetRequested=uiAnalysisEnabled && transportPlaying
        && resetMatchOnNextPlaybackBlock.exchange(false,std::memory_order_relaxed);
    if (uiAnalysisEnabled && transportPlaying && (!lastTransportPlaying || matchResetRequested))
    {
        resetMatchAccumulator();
        matchReady.store (false, std::memory_order_relaxed);
    }

    // FULL retains live monitoring. ECO deliberately mutes/suspends whenever
    // the host is stopped, even with hardware noise or an active sidechain.
    // Recording/offline rendering always process, including hosts reporting
    // isPlaying=false during those operations. Unknown transport stays active.
    const bool ecoTransportStop=ecoForBlock && transportAvailable
        && !transportPlaying && !transportRecording && !offlineRendering;
    const auto silenceRevision=displayProjectionRevision.load(std::memory_order_acquire);
    const bool silenceRevisionStable=silenceRevision==stoppedSilenceRevision;
    const bool stoppedFullEligible=!ecoForBlock && transportAvailable && !transportPlaying && !transportRecording && !offlineRendering && silenceRevisionStable;
    stoppedSilenceRevision=silenceRevision;
    const auto keySource = juce::jlimit (static_cast<int> (qqsc::params::keyInternal),
                                         static_cast<int> (qqsc::params::keyExternal),
                                         juce::roundToInt (readSoundParameter(qqsc::params::keySource)));
    const bool useExternalKey = keySource == qqsc::params::keyExternal;
    // Only inputs selected by the processing graph may prevent sleep. An
    // enabled but unused sidechain bus must not keep an INT instance awake.
    const auto isExactlyZero = [numSamples] (const juce::AudioBuffer<float>& input, int channels)
    {
        for(int ch=0;ch<channels;++ch)
            if(!std::all_of(input.getReadPointer(ch),input.getReadPointer(ch)+numSamples,
                           [](float sample){return sample==0.0f;})) return false;
        return true;
    };
    const bool selectedInputIsZero = isExactlyZero(buffer,numInputChannels)
        && (!useExternalKey || isExactlyZero(externalKeyBuffer,externalKeyChannels));
    // Measure raw input only during stopped callbacks. Do not replace or gate
    // samples: these values distinguish main-input noise, selected sidechain
    // activity and invalid samples independently of the transport-stop policy.
    IdleInputDiagnostics idleInput;
    idleInput.externalSelected=useExternalKey;
    idleInput.externalChannels=externalKeyChannels;
    if(transportAvailable && !transportPlaying && !selectedInputIsZero)
    {
        idleInput.mainPeak=measureInputPeak(buffer,numInputChannels,idleInput.nonFiniteSamples);
        if(useExternalKey)
            idleInput.externalPeak=measureInputPeak(externalKeyBuffer,externalKeyChannels,idleInput.nonFiniteSamples);
    }
    const bool rawResidualSmall=idleInput.nonFiniteSamples==0
        && idleInput.mainPeak<=fullIdleRawFloor
        && (!useExternalKey || idleInput.externalPeak<=fullIdleRawFloor);
    bool stoppedInputIsQuiet=stoppedFullEligible && rawResidualSmall
        && (selectedInputIsZero || (double(idleInput.mainPeak)*fullIdleGainBound()<=double(fullIdleOutputFloor)
        && double(useExternalKey ? idleInput.externalPeak : idleInput.mainPeak)
            *std::pow(10.0,.05*double(readSoundParameter(useExternalKey ? qqsc::params::keyGainDb : qqsc::params::inputGainDb)))
            *4.0<=double(fullIdleOutputFloor)));
    int idleGate = !transportAvailable ? 1 : offlineRendering ? 12 : transportRecording ? 11 : transportPlaying ? 2
                 : !silenceRevisionStable ? 3 : stoppedInputIsQuiet ? 0 : !selectedInputIsZero ? (rawResidualSmall ? 15 : 4) : 0;

    const bool stereoBus = numInputChannels >= 2;
    if(ecoTransportStop || (!ecoForBlock && stoppedDspSleeping && stoppedSilentSamples>0 && stoppedInputIsQuiet
        && !ecoTransportSuspended && !abActive && !abTransferPending.load(std::memory_order_acquire)))
    {
        if(ecoTransportStop)
        {
            ecoTransportSuspended=true;
            stoppedDspSleeping=true;
            stoppedSilentSamples=0;
        }
        // FULL reaches here only after zero input and settled tails. ECO uses
        // the host stop directly; its cached audio is reset on the next active
        // block. Neither path runs the compressor or oversampling filters.
        for(int ch=0;ch<numOutputChannels;++ch) buffer.clear(ch,0,numSamples);
        bypassFade.setTargetValue(forceBypass ? 1.f : 0.f);
        bypassFade.skip(numSamples);
        unityMonitorFade.setTargetValue(isUnityMonitorActive() ? 1.f : 0.f);
        unityMonitorFade.skip(numSamples);
        unityMatchSettling=juce::jmax(0,unityMatchSettling-numSamples);
        if(uiAnalysisEnabled)
        {
            // Preserve FULL's history and meter hold timing. Only the cheap
            // zero history writes remain; no gain conversion, delay mixing,
            // HPF, compressor, Ceiling or true-peak oversampling is run here.
            if(auto history=std::atomic_load_explicit(&displayKeyHistoryStorage,std::memory_order_acquire))
                for(int i=0;i<numSamples;++i)
                    history->push(0.f,0.f,keySource,useExternalKey ? externalKeyChannels>=2 : stereoBus);
            meterState.inputDb0.store(-120.f,std::memory_order_relaxed);
            meterState.inputDb1.store(-120.f,std::memory_order_relaxed);
            meterState.outputDb0.store(-120.f,std::memory_order_relaxed);
            meterState.outputDb1.store(-120.f,std::memory_order_relaxed);
            meterState.displayInputDb0.store(-120.f,std::memory_order_relaxed);
            meterState.displayInputDb1.store(-120.f,std::memory_order_relaxed);
            meterState.displayDetectorDb0.store(-120.f,std::memory_order_relaxed);
            meterState.displayDetectorDb1.store(-120.f,std::memory_order_relaxed);
            meterState.keyInputDb.store(-120.f,std::memory_order_relaxed);
            meterState.gainReductionDb0.store(0.f,std::memory_order_relaxed);
            meterState.gainReductionDb1.store(0.f,std::memory_order_relaxed);
            meterState.displayCeilingGainReductionDb.store(0.f,std::memory_order_relaxed);
            updateTruePeakHold(-120.f,numSamples);
            for(int ch=0;ch<2;++ch) updateGainReductionHoldChannel(ch,0.f,numSamples);
            meterState.gainReductionHoldDb0.store(gainReductionHoldDb[0],std::memory_order_relaxed);
            meterState.gainReductionHoldDb1.store(gainReductionHoldDb[1],std::memory_order_relaxed);
        }
        lastTransportPlaying=transportPlaying;
        lastTransportSample=transportSample;
        lastTransportBlockSize=numSamples;
        ++stoppedFastPathBlocks;
        recordPerformanceBlock(performancePhaseForBlock, performanceStart, numSamples,
                               selectedInputIsZero, transportPlaying, transportAvailable, true, ecoTransportStop ? 10 : !selectedInputIsZero ? 13 : 9, idleInput);
        return;
    }
    if(ecoTransportSuspended)
        resumeFromEcoTransportStop(forceBypass);
    const int mode = juce::jlimit (static_cast<int> (qqsc::params::stereoLinked),
                                   static_cast<int> (qqsc::params::leftRight),
                                   static_cast<int> (readSoundParameter(qqsc::params::processingMode)));

    meterState.processingMode.store (mode, std::memory_order_relaxed);

    // One atomic boundary-pair load per domain per block; audio and Display
    // evaluate the same law without APVTS lookups per audio sample.
    algorithmFade.setTargetValue (readSoundParameter(qqsc::params::algorithmMode));
    detectorModeFade.setTargetValue (readSoundParameter(qqsc::params::detectorMode));
    upAlgorithmFade.setTargetValue(readSoundParameter("upAlgorithmMode"));
    downAlgorithmFade.setTargetValue(readSoundParameter("downAlgorithmMode"));
    const bool dualCompression = readSoundParameter(qqsc::params::compressionMode) >= 0.5f;
    std::array<float, 5> lowerBoundaries, upperBoundaries;
    for (size_t d = 0; d < 5; ++d)
    {
        float lower, upper;
        unpackDisplayStereoSample (boundaryPairs[(isLimiterMode() ? 10u : 0u) + (dualCompression ? 5u : 0u) + (isLimiterMode() && d==2 ? 1u : d)].load (std::memory_order_acquire), lower, upper);
        lowerBoundaries[d] = qqsc::params::thresholdLinear (lower);
        upperBoundaries[d] = dualCompression ? qqsc::params::thresholdLinear (upper)
                                             : qqsc::params::rangeLinear (upper);
        upRatioSmoothers[d].setTargetValue (qqsc::params::upwardRatio (readSoundParameter (qqsc::params::upRatioIds[d]), isLimiterMode()));
        upEnableFades[d].setTargetValue (readSoundParameter(qqsc::params::upEnabledIds[d]));
        downRatioSmoothers[d].setTargetValue (qqsc::params::limiterRatio (readSoundParameter (qqsc::params::downRatioIds[d]), isLimiterMode(), true));
        downEnableFades[d].setTargetValue (readSoundParameter(qqsc::params::downEnabledIds[d]));
    }

    if (uiAnalysisEnabled && mode != gainReductionHoldMode)
        resetGainReductionHold (mode);

    inputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (
        readSoundParameter(qqsc::params::inputGainDb)));
    ratioSmoother.setTargetValue (effectiveSingleRatio (0));
    ratioLSmoother.setTargetValue (effectiveSingleRatio (1));
    ratioRSmoother.setTargetValue (effectiveSingleRatio (2));
    ratioMSmoother.setTargetValue (effectiveSingleRatio (3));
    ratioSSmoother.setTargetValue (effectiveSingleRatio (4));
    makeupSTSmoother.setTargetValue (readSoundParameter(qqsc::params::makeupGainDb));
    makeupLSmoother.setTargetValue (readSoundParameter(qqsc::params::makeupGainLDb));
    makeupRSmoother.setTargetValue (readSoundParameter(qqsc::params::makeupGainRDb));
    makeupMSmoother.setTargetValue (readSoundParameter(qqsc::params::makeupGainMDb));
    makeupSSmoother.setTargetValue (readSoundParameter(qqsc::params::makeupGainSDb));
    mixSmoother.setTargetValue (readSoundParameter(qqsc::params::mix) * 0.01f);
    mixLSmoother.setTargetValue (readSoundParameter(qqsc::params::mixL) * 0.01f);
    mixRSmoother.setTargetValue (readSoundParameter(qqsc::params::mixR) * 0.01f);
    mixMSmoother.setTargetValue (readSoundParameter(qqsc::params::mixM) * 0.01f);
    mixSSmoother.setTargetValue (readSoundParameter(qqsc::params::mixS) * 0.01f);
    outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (
        getActiveOutputGainDb()));
    unityOutputSmoother.setTargetValue(juce::Decibels::decibelsToGain(
        isLimiterMode() ? readSoundParameter(qqsc::params::limiterOutputDb) : 0.f));
    unityMonitorFade.setTargetValue(isUnityMonitorActive() ? 1.f : 0.f);
    if(!bypassInitialised)
    { bypassFade.setCurrentAndTargetValue(forceBypass ? 1.f : 0.f);bypassInitialised=true; }
    bypassFade.setTargetValue(forceBypass ? 1.f : 0.f);
    keyGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (
        readSoundParameter(qqsc::params::keyGainDb)));
    const auto keyHpfHz = readSoundParameter(qqsc::params::keyHpfHz);
    const auto keyHpfEnabled = qqsc::params::isKeyHpfEnabled (keyHpfHz);
    keyHpfCutoffSmoother.setTargetValue (
        keyHpfEnabled ? qqsc::params::clampKeyHpfHz (keyHpfHz) : qqsc::params::keyHpfMinHz);
    keyHpfWetSmoother.setTargetValue (keyHpfEnabled ? 1.0f : 0.0f);

    const bool detectorKeyIsStereo = useExternalKey ? externalKeyChannels >= 2 : stereoBus;
    const auto displayHistoryForBlock = uiAnalysisEnabled
        ? std::atomic_load_explicit (&displayKeyHistoryStorage, std::memory_order_acquire)
        : std::shared_ptr<DisplayKeyHistoryStorage> {};

    float meterMaxGrDb[2] { 0.0f, 0.0f };
    float meterMinGrDb[2] { 0.0f, 0.0f };
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

        if (uiAnalysisEnabled && displayHistoryForBlock != nullptr)
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
        if (uiAnalysisEnabled)
            keyInputPeak = juce::jmax (keyInputPeak, maxAbs (selectedKeyL, selectedKeyR));

        oversamplingInputBuffer.setSample (0, i, mainL);
        oversamplingInputBuffer.setSample (1, i, mainR);
        oversamplingInputBuffer.setSample (2, i, selectedKeyL);
        oversamplingInputBuffer.setSample (3, i, selectedKeyR);
        oversamplingInputBuffer.setSample (4, i, 0.0f);
        oversamplingInputBuffer.setSample (5, i, 0.0f);
        oversamplingInputBuffer.setSample (6, i, 0.0f);
        oversamplingInputBuffer.setSample (7, i, 0.0f);
    }

    // Cache the existing base-rate controls once. The original six paths
    // still use these exact values after downsampling. Two extra aligned
    // channels carry the complete A/B result, including Makeup and Mix.
    const std::array<juce::SmoothedValue<float>*,5> makeups { &makeupSTSmoother, &makeupLSmoother, &makeupRSmoother, &makeupMSmoother, &makeupSSmoother };
    const std::array<juce::SmoothedValue<float>*,5> mixes { &mixSmoother, &mixLSmoother, &mixRSmoother, &mixMSmoother, &mixSSmoother };
    for (int i=0;i<numSamples;++i)
    {
        for (size_t d=0;d<5;++d)
        {
            mixControlBuffer.setSample (int(d),i,juce::Decibels::decibelsToGain (makeups[d]->getNextValue(), -180.0f));
            mixControlBuffer.setSample (int(d)+5,i,mixes[d]->getNextValue());
        }
        mixControlBuffer.setSample (10,i,outputGainSmoother.getNextValue());
        mixControlBuffer.setSample(15,i,unityOutputSmoother.getNextValue());
    }
    if (abTransferPending.load (std::memory_order_acquire))
    {
        const juce::SpinLock::ScopedTryLockType lock (abTransferLock);
        if (lock.isLocked())
        {
            if (! requestedABCompatible) abActive = false;
            else if (abActive && requestedABTo == abFrom && ! abFromFrozen)
                abFade.setTargetValue (0.0f);
            else if (abActive && requestedABTo == abTo)
                abFade.setTargetValue (1.0f);
            else
            {
                abFromFrozen = abActive;
                abFrozen = abLastMatrix;
                abFrozenDry = abLastDryMatrix;
                abFrozenUnityOutput=abLastUnityOutput;
                abFrom = requestedABFrom;
                abTo = requestedABTo;
                abFade.setCurrentAndTargetValue (0.0f);
                abFade.setTargetValue (1.0f);
                abActive = true;
            }
            abTailSamples = (currentTotalLatencySamples + 64) * currentOversamplingFactor;
            abTransferPending.store (false,std::memory_order_release);
        }
    }
    const bool abOutputForBlock = abActive;
    if (currentLimiterCeilingActive)
    {
        outputCeiling.setStereoLink(readSoundParameter(qqsc::params::limiterStereoLink)*.01f);
        outputCeiling.set(isTruePeakSelected(),readSoundParameter(qqsc::params::ceilingDb),getTpRecoveryMode());
    }

    // Keep the HPF and output tail advancing until both have remained quiet
    // for a full second. Zero and sub-floor blocks share the same settling
    // window so a host alternating between them cannot defeat sleep.
    const bool idleABBusy=abActive || abTransferPending.load(std::memory_order_acquire);
    const bool idleKeySettled=!(stoppedInputIsQuiet && !idleABBusy)
        || keyInputBuffer.getMagnitude(0,numSamples)<=fullIdleOutputFloor;
    stoppedInputIsQuiet=stoppedInputIsQuiet && !idleABBusy && idleKeySettled;
    if(idleGate==0) idleGate=idleABBusy ? 5 : !idleKeySettled ? 6 : 0;
    const auto silenceWait=juce::jmax(int64_t(std::ceil(currentSampleRate)),
                                    int64_t(2)*currentTotalLatencySamples+configuredMaximumBlockSize);
    stoppedSilentSamples=stoppedInputIsQuiet
        ? juce::jmin(silenceWait,stoppedSilentSamples+numSamples):0;
    const bool residualOutputSettled=stoppedQuietOutputSamples>=silenceWait;
    // Quiet output alone is insufficient: Ceiling's release can still affect
    // the next hit. Keep the original recovery tolerance, while permitting a
    // sub-floor carrier instead of requiring a permanently exact-zero input.
    const bool ceilingTailSettled=!currentLimiterCeilingActive
        || outputCeiling.isQuietAndSettled(fullIdleOutputFloor);
    stoppedDspSleeping=stoppedSilentSamples>=silenceWait && residualOutputSettled && ceilingTailSettled;
    if(idleGate==0) idleGate=stoppedDspSleeping ? (!selectedInputIsZero ? 13 : 9)
        : stoppedSilentSamples<silenceWait ? 7 : !residualOutputSettled ? 14 : 8;

    if(stoppedDspSleeping)
        wetBaseBuffer.clear(0,numSamples);
    else
    {
    // Main and selected Key enter the same effective 1x/4x/8x/16x internal domain.
    // Channels 2/3 carry the Key only until the detector consumes them; all six
    // channels are then overwritten with the established wet variants.
    const juce::dsp::AudioBlock<const float> fullHostInputBlock (oversamplingInputBuffer);
    const auto hostInputBlock = fullHostInputBlock.getSubBlock (0, static_cast<size_t> (numSamples));
    auto& oversampler = getCurrentOversampler();
    auto oversampledBlock = oversampler.processSamplesUp (hostInputBlock);

    const auto internalNumSamples = static_cast<int> (oversampledBlock.getNumSamples());
    const auto expectedInternalSamples = numSamples * currentOversamplingFactor;
    jassert (internalNumSamples == expectedInternalSamples);
    jassert (oversampledBlock.getNumChannels() >= 8u);
    if (internalNumSamples != expectedInternalSamples || oversampledBlock.getNumChannels() < 8u)
    {
        buffer.clear();
        return;
    }

    // Hidden ECO only needs the audible mode's gain curves. Preserve every
    // detector/smoother and the end of all inactive downsampler histories so
    // reopening the editor or selecting another mode is ready next block.
    // Twice the complete FIR latency bounds its finite support; 64 additional
    // host samples settle JUCE's fractional Thiran delay below float precision.
    const int warmHostSamples = currentOversamplingFactor > 1
        ? 2 * getOversamplingLatencySamples(currentOversamplingIndex) + 64 : 0;
    const int fullDomainStart = uiAnalysisEnabled ? 0
        : juce::jmax(0, numSamples - warmHostSamples) * currentOversamplingFactor;
    limiterStereoLinkSmoother.setTargetValue(readSoundParameter(qqsc::params::limiterStereoLink)*.01f);
    const size_t audibleFirst = mode == qqsc::params::midSide ? 3u
                             : mode == qqsc::params::leftRight ? 1u : 0u;
    const size_t audibleEnd = audibleFirst + (mode == qqsc::params::stereoLinked ? 1u : 2u);
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

        oversampledKeyHistoryBuffer.setSample (0, oversampledDelayWriteIndex, keyL);
        oversampledKeyHistoryBuffer.setSample (1, oversampledDelayWriteIndex, keyR);
        oversampledKeyHistoryBuffer.setSample (2, oversampledDelayWriteIndex, keyM);
        oversampledKeyHistoryBuffer.setSample (3, oversampledDelayWriteIndex, keyS);
        // Audio remains delayed by N. Feed the W-radius detectors the key at
        // n-(N-W), so both peaks still belong to the audible sample n-N.
        // W=N is the original path, W=0 is sample detection at the SAME PDC.
        // No gain envelope, release memory, or interpolation is added.
        int detectorReadIndex = oversampledDelayWriteIndex
            - (currentLookaheadSamplesInternal - currentDetectorWindowSamplesInternal);
        if (detectorReadIndex < 0) detectorReadIndex += oversampledDelayCapacity;
        leftEngine.processLevelSample (oversampledKeyHistoryBuffer.getSample (0, detectorReadIndex), detectorSampleCounter);
        rightEngine.processLevelSample (oversampledKeyHistoryBuffer.getSample (1, detectorReadIndex), detectorSampleCounter);
        midEngine.processLevelSample (oversampledKeyHistoryBuffer.getSample (2, detectorReadIndex), detectorSampleCounter);
        sideEngine.processLevelSample (oversampledKeyHistoryBuffer.getSample (3, detectorReadIndex), detectorSampleCounter);
        // Interpolate only when the user explicitly changes detector mode.
        // No music-dependent gain envelope is introduced: steady choices use
        // the exact Original or Bilateral peak, including in the Display path.
        const auto bilateralAmount = detectorModeFade.getNextValue();
        const auto selectPeak = [bilateralAmount] (const qqsc::StaticCompressionEngine& engine)
        {
            const auto original = engine.getCurrentLevel();
            return original + bilateralAmount * (engine.getCurrentBilateralPeak() - original);
        };
        const auto linkedOriginal = stereoKey
            ? juce::jmax (leftEngine.getCurrentLevel(), rightEngine.getCurrentLevel())
            : leftEngine.getCurrentLevel();
        const auto linkedBilateral = stereoKey
            ? juce::jmax (leftEngine.getCurrentBilateralPeak(), rightEngine.getCurrentBilateralPeak())
            : leftEngine.getCurrentBilateralPeak();
        const auto linkedLevel = linkedOriginal + bilateralAmount * (linkedBilateral - linkedOriginal);
        const std::array<float, 5> levels { linkedLevel, selectPeak (leftEngine), selectPeak (rightEngine),
                                            selectPeak (midEngine), selectPeak (sideEngine) };
        const std::array<float, 5> singleRatios { ratioSTNow, ratioLNow, ratioRNow, ratioMNow, ratioSNow };
        // One shared weight per INTERNAL sample keeps all domains aligned.
        // Both laws see the same detector and delayed carrier. A linear
        // gain blend is exactly a crossfade between their audio outputs;
        // unity stays unity. Reversing mid-fade starts at the current weight.
        const auto superAmount = algorithmFade.getNextValue();
        const auto upSuperAmount=upAlgorithmFade.getNextValue(),downSuperAmount=downAlgorithmFade.getNextValue();
        std::array<float, 5> gains;
        for (size_t d = 0; d < 5; ++d)
        {
            const auto upRatio = upRatioSmoothers[d].getNextValue();
            const auto downRatio = downRatioSmoothers[d].getNextValue();
            const auto upAmount = upEnableFades[d].getNextValue();
            const auto downAmount = downEnableFades[d].getNextValue();
            if (i < fullDomainStart && (d < audibleFirst || d >= audibleEnd))
            {
                gains[d] = 1.0f;
                continue;
            }
            const auto evaluateSingle = [&] (qqsc::CompressionAlgorithm algorithm)
            {
                return qqsc::StaticCompressionEngine::singleGainForLevel (levels[d], singleRatios[d], lowerBoundaries[d], upperBoundaries[d], algorithm);
            };
            if(dualCompression)
            {
                // Independent 10 ms gain crossfades. Steady choices evaluate
                // exactly one curve pair; switching both stays continuous.
                gains[d]=0;
                for(int up=0;up<2;++up)for(int down=0;down<2;++down)
                {
                    const auto weight=(up ? upSuperAmount : 1-upSuperAmount)*(down ? downSuperAmount : 1-downSuperAmount);
                    if(weight<=0)continue;
                    gains[d]+=weight*qqsc::StaticCompressionEngine::dualGainForLevel(levels[d],upRatio,downRatio,
                        lowerBoundaries[d],upperBoundaries[d],up ? qqsc::CompressionAlgorithm::super : qqsc::CompressionAlgorithm::classic,
                        down ? qqsc::CompressionAlgorithm::super : qqsc::CompressionAlgorithm::classic);
                }
            }
            else if (superAmount <= 0.0f) gains[d] = evaluateSingle (qqsc::CompressionAlgorithm::classic);
            else if (superAmount >= 1.0f) gains[d] = evaluateSingle (qqsc::CompressionAlgorithm::super);
            else gains[d] = (1.0f - superAmount) * evaluateSingle (qqsc::CompressionAlgorithm::classic)
                          + superAmount * evaluateSingle (qqsc::CompressionAlgorithm::super);
            // Ten millisecond crossfade of this branch's processed signal with
            // its unprocessed signal; stored Ratio and detector remain untouched.
            if (dualCompression)
            {
                const auto amount = levels[d] < upperBoundaries[d] ? upAmount : downAmount;
                gains[d] = (1.0f - amount) + gains[d] * amount;
            }
            if (useExternalKey && levels[d] <= 1.0e-9f) gains[d] = 1.0f;
        }
        const float limiterStereoLinkAmount=limiterStereoLinkSmoother.getNextValue();
        if (currentLimiterCeilingActive && stereoBus)
            qqsc::params::coupleLimiterGains(gains[1],gains[2],limiterStereoLinkAmount);
        const auto linkedGain = gains[0];
        const auto gainL = gains[1], gainR = gains[2], gainM = gains[3], gainS = gains[4];

        if (uiAnalysisEnabled)
        {
            if (mode == qqsc::params::midSide)
            {
                displayDetectorPeak[0] = juce::jmax (displayDetectorPeak[0], levels[3]);
                displayDetectorPeak[1] = juce::jmax (displayDetectorPeak[1], levels[4]);
            }
            else if (mode == qqsc::params::leftRight)
            {
                displayDetectorPeak[0] = juce::jmax (displayDetectorPeak[0], levels[1]);
                displayDetectorPeak[1] = juce::jmax (displayDetectorPeak[1], levels[2]);
            }
            else
                displayDetectorPeak[0] = juce::jmax (displayDetectorPeak[0], linkedLevel);
        }
        oversampledLookaheadDelayBuffer.setSample (0, oversampledDelayWriteIndex, inputL);
        oversampledLookaheadDelayBuffer.setSample (1, oversampledDelayWriteIndex, inputR);

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

        const int baseSample = i / currentOversamplingFactor;
        std::array<float,5> totalGains, dryGains;
        for (size_t d=0;d<5;++d)
        {
            const auto amount=mixControlBuffer.getSample (int(d)+5,baseSample);
            totalGains[d]=(gains[d]*mixControlBuffer.getSample (int(d),baseSample)*amount)
                       * mixControlBuffer.getSample (10,baseSample);
            dryGains[d]=(1.0f-amount)*mixControlBuffer.getSample (10,baseSample);
        }
        auto complete = qqsc::ABTransfer::matrix (totalGains,mode,stereoBus);
        auto completeDry = qqsc::ABTransfer::matrix (dryGains,mode,stereoBus);
        auto monitorOutput=mixControlBuffer.getSample(15,baseSample);
        if (abOutputForBlock)
        {
            const auto from = abFromFrozen ? abFrozen : abFrom.evaluate (levels,useExternalKey,stereoBus);
            const auto to = abTo.evaluate (levels,useExternalKey,stereoBus);
            const auto fromDry = abFromFrozen ? abFrozenDry : abFrom.dryMatrix(stereoBus);
            const auto toDry = abTo.dryMatrix(stereoBus);
            const auto t = abFade.getNextValue();
            const auto w = t*t*(3.0f-2.0f*t); // zero slope at either endpoint
            monitorOutput=(1.f-w)*(abFromFrozen ? abFrozenUnityOutput : abFrom.unityOutput)+w*abTo.unityOutput;
            for (size_t c=0;c<4;++c)
            {
                complete[c]=(1.0f-w)*from[c]+w*to[c];
                completeDry[c]=(1.0f-w)*fromDry[c]+w*toDry[c];
            }
            if (abFade.isSmoothing())
                abTailSamples=(currentTotalLatencySamples+64)*currentOversamplingFactor;
            else if (--abTailSamples<=0 && ! restoringDynamicsState.load (std::memory_order_acquire))
                abActive=false;
        }
        abLastMatrix=complete;
        abLastDryMatrix=completeDry;
        abLastUnityOutput=monitorOutput;
        mixControlBuffer.setSample(15,baseSample,monitorOutput);
        for (int c=0;c<4;++c) mixControlBuffer.setSample (11+c,baseSample,completeDry[size_t(c)]);
        if (currentCeilingSharesCoreOversampling)
        {
            // Shared Ceiling needs the complete post-Makeup/Mix/Output signal
            // before the one common downsampling filter.
            oversampledBlock.setSample (6,i,(complete[0]+completeDry[0])*dryLInternal
                                             +(complete[1]+completeDry[1])*dryRInternal);
            oversampledBlock.setSample (7,i,(complete[2]+completeDry[2])*dryLInternal
                                             +(complete[3]+completeDry[3])*dryRInternal);
        }
        else
        {
            oversampledBlock.setSample (6,i,complete[0]*dryLInternal+complete[1]*dryRInternal);
            oversampledBlock.setSample (7,i,complete[2]*dryLInternal+complete[3]*dryRInternal);
        }

        if (uiAnalysisEnabled)
        {
            float gr0 = 0.0f;
            float gr1 = 0.0f;
            if (mode == qqsc::params::midSide)
            {
                gr0 = -juce::Decibels::gainToDecibels (gainM, -180.0f);
                gr1 = -juce::Decibels::gainToDecibels (gainS, -180.0f);
            }
            else if (mode == qqsc::params::leftRight)
            {
                gr0 = -juce::Decibels::gainToDecibels (gainL, -180.0f);
                gr1 = -juce::Decibels::gainToDecibels (gainR, -180.0f);
            }
            else
            {
                const auto linkedReductionDb = -juce::Decibels::gainToDecibels (juce::jmax (linkedGain, 1.0e-9f), -180.0f);
                gr0 = linkedReductionDb;
                gr1 = linkedReductionDb;
            }

            // Parallel Mix changes the relative dB magnitude of boost versus cut.
            // Preserve both extrema until Mix is applied, then select the stronger.
            meterMaxGrDb[0] = juce::jmax (meterMaxGrDb[0], gr0);
            meterMinGrDb[0] = juce::jmin (meterMinGrDb[0], gr0);
            if (stereoBus || mode == qqsc::params::midSide)
            {
                meterMaxGrDb[1] = juce::jmax (meterMaxGrDb[1], gr1);
                meterMinGrDb[1] = juce::jmin (meterMinGrDb[1], gr1);
            }
        } // UI-only gain conversion and accumulation
    }

    if (currentCeilingSharesCoreOversampling)
    {
        auto sharedCeilingBlock=oversampledBlock.getSubsetChannelBlock(6,2);
        outputCeiling.processSharedOversampledBlock(sharedCeilingBlock,numSamples,currentOversamplingFactor);
    }

    auto wetBlock = juce::dsp::AudioBlock<float> (wetBaseBuffer)
                        .getSubsetChannelBlock (0, 8)
                        .getSubBlock (0, static_cast<size_t> (numSamples));
    oversampler.processSamplesDown (wetBlock);
    } // Keep the settled DSP state intact for wake-up.

    // Keep the wet path warm while auditioning bypass. The final crossfade
    // selects the aligned original; it never sends that original through TP.
    float meterInputPeak[2]  { 0.0f, 0.0f };
    float meterOutputPeak[2] { 0.0f, 0.0f };
    float displayInputPeak[2]  { 0.0f, 0.0f };
    std::array<float,2> displayCeilingGainLR {1,1};
    float displayCeilingGainLinear = 1.0f;
    float displayPreCeilingSamplePeakLinear = 0.0f;
    float displayPreCeilingTruePeakLinear = 0.0f;

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

        if (uiAnalysisEnabled && transportPlaying && !isUnityMonitorActive())
        {
            loudnessMatch.processSample (dryL, stereoBus ? dryR : 0.0f,
                                         isLimiterMode() ? wetIndependentL : wetLinkedL,
                                         stereoBus ? (isLimiterMode() ? wetIndependentR : wetLinkedR) : 0.0f,
                                         wetIndependentL, stereoBus ? wetIndependentR : 0.0f,
                                         dryM, stereoBus ? dryS : 0.0f,
                                         wetM, stereoBus ? wetS : 0.0f);
        }

        const auto makeupST = mixControlBuffer.getSample (0,i);
        const auto makeupL  = mixControlBuffer.getSample (1,i);
        const auto makeupR  = mixControlBuffer.getSample (2,i);
        const auto makeupM  = mixControlBuffer.getSample (3,i);
        const auto makeupS  = mixControlBuffer.getSample (4,i);
        const auto mixST = mixControlBuffer.getSample (5,i);
        const auto mixL  = mixControlBuffer.getSample (6,i);
        const auto mixR  = mixControlBuffer.getSample (7,i);
        const auto mixM  = mixControlBuffer.getSample (8,i);
        const auto mixS  = mixControlBuffer.getSample (9,i);

        float wetL = wetLinkedL;
        float wetR = wetLinkedR;
        float mixedL = dryL * (1.0f - mixST) + wetLinkedL * makeupST * mixST;
        float mixedR = dryR * (1.0f - mixST) + wetLinkedR * makeupST * mixST;
        float mixDryL=dryL*(1.f-mixST),mixDryR=dryR*(1.f-mixST);

        if (mode == qqsc::params::midSide)
        {
            wetL = stereoBus ? wetM + wetS : wetM;
            wetR = stereoBus ? wetM - wetS : 0.0f;

            // Independent M/S Mix must occur in the M/S domain before decoding
            // back to L/R. With equal Mix values this is exactly equivalent to
            // the legacy shared post-decode blend; unequal values now remain
            // genuinely independent.
            const float mixedM = dryM * (1.0f - mixM) + wetM * makeupM * mixM;
            const float mixedS = dryS * (1.0f - mixS) + wetS * makeupS * mixS;
            mixedL = stereoBus ? mixedM + mixedS : mixedM;
            mixedR = stereoBus ? mixedM - mixedS : 0.0f;
            mixDryL=dryM*(1.f-mixM)+(stereoBus ? dryS*(1.f-mixS) : 0.f);
            mixDryR=stereoBus ? dryM*(1.f-mixM)-dryS*(1.f-mixS) : 0.f;

            if (uiAnalysisEnabled) { meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryM)); meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryS)); }
        }
        else if (mode == qqsc::params::leftRight)
        {
            wetL = wetIndependentL;
            wetR = wetIndependentR;
            mixedL = dryL * (1.0f - mixL) + wetL * makeupL * mixL;
            mixedR = dryR * (1.0f - mixR) + wetR * makeupR * mixR;
            mixDryL=dryL*(1.f-mixL);mixDryR=dryR*(1.f-mixR);

            if (uiAnalysisEnabled) { meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryL)); meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryR)); }
        }
        else
        {
            if (uiAnalysisEnabled) { meterInputPeak[0] = juce::jmax (meterInputPeak[0], std::abs (dryL)); meterInputPeak[1] = juce::jmax (meterInputPeak[1], std::abs (dryR)); }
        }
        const auto outputGain = mixControlBuffer.getSample (10,i);
        const float activeOutL = abOutputForBlock ? wetBaseBuffer.getSample (6,i)
            + mixControlBuffer.getSample (11,i)*dryL + mixControlBuffer.getSample (12,i)*dryR : mixedL * outputGain;
        const float activeOutR = abOutputForBlock && stereoBus ? wetBaseBuffer.getSample (7,i)
            + mixControlBuffer.getSample (13,i)*dryL + mixControlBuffer.getSample (14,i)*dryR : mixedR * outputGain;

        // Normal mode ends here: Limiter Ceiling is not in its audio path and
        // contributes neither 8x filter latency nor Ceiling lookahead latency.
        // Limiter mode inserts the fixed-latency 8x Ceiling pipeline; TP OFF/ON
        // are two already-warm branches inside that same pipeline.
        std::array<float,2> finalOutput { activeOutL, stereoBus ? activeOutR : 0.0f };
        std::array<float,10> reference { displayDryL,displayDryR,delayedKeyL,delayedKeyR,
            mixControlBuffer.getSample(15,i),mixDryL,mixDryR,mixedL-mixDryL,mixedR-mixDryR,makeupL };

        if (currentLimiterCeilingActive)
        {
            if(stoppedDspSleeping) finalOutput={0.0f,0.0f};
            else if (currentCeilingSharesCoreOversampling)
            {
                const float sharedL=wetBaseBuffer.getSample(6,i);
                const float sharedR=stereoBus ? wetBaseBuffer.getSample(7,i) : 0.0f;
                finalOutput=outputCeiling.finishSharedBaseSample(sharedL,sharedR,i);
            }
            else
            {
                finalOutput=outputCeiling.process(activeOutL,stereoBus ? activeOutR : 0.0f);
            }
            for(int ch=0;ch<2;++ch)displayCeilingGainLR[size_t(ch)]=juce::jmin(displayCeilingGainLR[size_t(ch)],outputCeiling.gainForDisplayLinear(ch));
            displayCeilingGainLinear=juce::jmin(displayCeilingGainLinear,
                                                 outputCeiling.gainForDisplayLinear());
            displayPreCeilingSamplePeakLinear=juce::jmax(displayPreCeilingSamplePeakLinear,
                outputCeiling.inputSamplePeakForDisplayLinear());
            displayPreCeilingTruePeakLinear=juce::jmax(displayPreCeilingTruePeakLinear,
                outputCeiling.inputTruePeakForDisplayLinear());

            ceilingReferenceDelay[ceilingReferenceIndex]=reference;
            int referenceRead=int(ceilingReferenceIndex)-ceilingReferenceDelaySamples;
            while(referenceRead<0) referenceRead+=int(ceilingReferenceDelay.size());
            reference=ceilingReferenceDelay[size_t(referenceRead)];
            ceilingReferenceIndex=(ceilingReferenceIndex+1)%ceilingReferenceDelay.size();
        }
        else
        {
            const float samplePeak=juce::jmax(std::abs(activeOutL),
                                               std::abs(stereoBus ? activeOutR : activeOutL));
            displayPreCeilingSamplePeakLinear=juce::jmax(displayPreCeilingSamplePeakLinear,samplePeak);
            displayPreCeilingTruePeakLinear=juce::jmax(displayPreCeilingTruePeakLinear,samplePeak);
        }
        // Host/plug-in bypass must never inherit a fading ceiling envelope or
        // previously limited samples from the output guard's delay line.
        const auto unityAmount=unityMonitorFade.getNextValue();
        const float inverseOutput=1.f/juce::jmax(1.e-6f,reference[4]);
        const float monitorGain=unityAmount==0 ? 1.f : 1.f+unityAmount*(inverseOutput-1.f);
        const float monitoredL=finalOutput[0]*monitorGain,monitoredR=finalOutput[1]*monitorGain;
        const auto bypassAmount=bypassFade.getNextValue();
        const float outL=bypassAmount==1 ? reference[0] : monitoredL+bypassAmount*(reference[0]-monitoredL);
        const float outR=stereoBus ? (bypassAmount==1 ? reference[1] : monitoredR+bypassAmount*(reference[1]-monitoredR)) : 0.f;
        if(unityMatchSettling>0)--unityMatchSettling;
        if(uiAnalysisEnabled && transportPlaying && isUnityMonitorActive()
            && !forceBypass && !bypassFade.isSmoothing() && unityAmount==1 && !abOutputForBlock)
        {
            // Compare the same aligned source against the actual post-TP,
            // post-monitor stereo signal. No Bypass samples enter the result.
            // Store a unit-Makeup basis, including observed Ceiling gain.
            // Retained frames can then yield an absolute Makeup target on
            // every click; prior Match gain is never added a second time.
            const float invMakeup=1.f/juce::jmax(1.e-6f,reference[9]);
            const auto split=[&](float dry,float wet,float measured)
            {
                const float sum=dry+wet;
                const float observedGain=std::abs(sum)>1.e-12f ? measured/sum : 0.f;
                return std::array<float,2>{dry*observedGain,wet*observedGain*invMakeup};
            };
            const auto left=split(reference[5],reference[7],monitoredL);
            const auto right=split(reference[6],reference[8],stereoBus ? monitoredR : 0.f);
            const float unitL=left[0]+left[1],unitR=right[0]+right[1];
            loudnessMatch.processSample(reference[0],stereoBus ? reference[1] : 0.f,
                unitL,unitR,unitL,unitR,
                .5f*(reference[0]+reference[1]),.5f*(reference[0]-reference[1]),
                .5f*(unitL+unitR),.5f*(unitL-unitR));
            unityMixMatch.processSample(left[0],right[0],left[1],right[1],unitL,unitR,0,0,0,0);
        }

        // v1.0.3 centered domain monitor: audition affects only what reaches the
        // headphones/speakers. Display, meters, Match and stored processing result
        // continue to use the normal pre-monitor outL/outR below. This mirrors the
        // mature QQ ChainScope Mixboard convention for centered L/R/Side audition.
        // Meter the actual post-Ceiling/post-Output stereo result, including
        // A/B and bypass. Never infer TP by clamping to the Ceiling target.
        if (uiAnalysisEnabled)
        {
            truePeakBuffer.setSample(0,i,outL);
            truePeakBuffer.setSample(1,i,stereoBus ? outR : outL);
        }
        float audibleOutL = outL;
        float audibleOutR = outR;

        if (! forceBypass && sidechainListen.load (std::memory_order_relaxed))
        {
            audibleOutL = reference[2];
            audibleOutR = reference[3];
        }
        else if (! forceBypass && stereoBus && !isLimiterMode())
        {
            // Limiter always returns both channels. Preserve the separate
            // Normal LR/MS audition selection for when that mode is restored.
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

        // Read-only measurement after Ceiling, 1:1 monitoring and audition.
        // Mono contributes one channel, never a duplicated +3 LU reading.
        if (measureOutputLoudness)
            outputLoudness.processSample(audibleOutL,stereoBus ? audibleOutR : 0.0f);
        buffer.setSample (0, i, audibleOutL);
        if (stereoBus)
            buffer.setSample (1, i, audibleOutR);

        if (mode == qqsc::params::midSide)
        {
            const float outM = stereoBus ? 0.5f * (outL + outR) : outL;
            const float outS = stereoBus ? 0.5f * (outL - outR) : 0.0f;
            if (uiAnalysisEnabled) { meterOutputPeak[0] = juce::jmax (meterOutputPeak[0], std::abs (outM)); meterOutputPeak[1] = juce::jmax (meterOutputPeak[1], std::abs (outS)); }

            const float displayDryM = stereoBus ? 0.5f * (reference[0] + reference[1]) : reference[0];
            const float displayDryS = stereoBus ? 0.5f * (reference[0] - reference[1]) : 0.0f;
            if (uiAnalysisEnabled) { displayInputPeak[0] = juce::jmax (displayInputPeak[0], std::abs (displayDryM)); displayInputPeak[1] = juce::jmax (displayInputPeak[1], std::abs (displayDryS)); }
        }
        else
        {
            if (uiAnalysisEnabled) { meterOutputPeak[0] = juce::jmax (meterOutputPeak[0], std::abs (outL)); meterOutputPeak[1] = juce::jmax (meterOutputPeak[1], std::abs (outR)); }

            if (mode == qqsc::params::leftRight)
            {
                if (uiAnalysisEnabled) { displayInputPeak[0] = juce::jmax (displayInputPeak[0], std::abs (reference[0])); displayInputPeak[1] = juce::jmax (displayInputPeak[1], std::abs (reference[1])); }
            }
            else
            {
                // ST remains one linked Display panel. Channel 1 is unused.
                if (uiAnalysisEnabled)
                    displayInputPeak[0] = juce::jmax (displayInputPeak[0],
                                                      stereoBus ? maxAbs (reference[0], reference[1]) : std::abs (reference[0]));
            }
        }
    }


    if (measureOutputLoudness)
    {
        meterState.outputIntegratedLufs.store(outputLoudness.integratedLufs(),std::memory_order_relaxed);
        meterState.outputLoudnessSeconds.store(outputLoudness.seconds(),std::memory_order_relaxed);
    }
    lastTransportPlaying = transportPlaying;
    lastTransportSample = transportSample;
    lastTransportBlockSize = numSamples;

    if (uiAnalysisEnabled)
    {
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
            const auto compressedGain = juce::Decibels::decibelsToGain (-coreGrDb, -180.0f);
            return qqsc::StaticCompressionEngine::effectiveGainReductionDb (compressedGain, wetMix);
        };
    
        const auto signedPeakAfterMix = [&] (int channel, float wetMix)
        {
            const auto cut = effectiveGrForMeter (meterMaxGrDb[channel], wetMix);
            const auto boost = effectiveGrForMeter (meterMinGrDb[channel], wetMix);
            return std::abs (boost) > std::abs (cut) ? boost : cut;
        };
        const auto effectiveGr0 = forceBypass ? 0.0f : signedPeakAfterMix (0, meterMix0);
        const auto effectiveGr1 = forceBypass ? 0.0f : signedPeakAfterMix (1, meterMix1);
        // Convert the accumulated OutputCeiling telemetry once per host block. The
        // sample loop stays entirely in the linear gain domain. min(linear gain) is
        // exactly equivalent to max(attenuation dB), without tens of thousands of
        // log10 calls per second per plug-in instance.
        const auto displayCeilingGainReductionDb = (forceBypass || displayCeilingGainLinear >= 1.0f)
            ? 0.0f
            : -juce::Decibels::gainToDecibels (juce::jmax (displayCeilingGainLinear, 1.0e-9f), -180.0f);
        float displayTruePeakExcessDb = 0.0f;
        if (displayPreCeilingSamplePeakLinear > 1.0e-9f
            && displayPreCeilingTruePeakLinear > displayPreCeilingSamplePeakLinear)
            displayTruePeakExcessDb = juce::jmax (0.0f, juce::Decibels::gainToDecibels (
                displayPreCeilingTruePeakLinear / displayPreCeilingSamplePeakLinear, -180.0f));
        // Accumulate until the 60 Hz display consumes it, so a short inter-sample
        // peak between GUI ticks is still represented in the historical point.
        atomicMaxFloat (meterState.displayTruePeakExcessDb, displayTruePeakExcessDb);
        // GAIN +/- and Dynamic Display share the same final-processing semantics:
        // compressor/Mix signed gain change plus the actual stereo-linked OutputCeiling
        // attenuation. TP/Sample-Peak switching is already represented by the
        // OutputCeiling telemetry's 10 ms audio crossfade.
        const auto ceilingGrForVisuals = forceBypass ? 0.0f : juce::jmax (0.0f, displayCeilingGainReductionDb);
        const auto ceilingGrForChannel=[&](size_t ch) {
            return forceBypass ? 0.0f : currentLimiterCeilingActive && mode==qqsc::params::leftRight
                ? -juce::Decibels::gainToDecibels(juce::jmax(displayCeilingGainLR[ch],1.e-9f),-180.f) : ceilingGrForVisuals;
        };
        const auto totalEffectiveGr0 = effectiveGr0 + ceilingGrForChannel(0);
        const auto totalEffectiveGr1 = effectiveGr1 + ceilingGrForChannel(1);
    
        updateTruePeakMeter(numSamples);
        meterState.inputDb0.store  (peakToDb (meterInputPeak[0]), std::memory_order_relaxed);
        meterState.inputDb1.store  (peakToDb (meterInputPeak[1]), std::memory_order_relaxed);
        meterState.outputDb0.store (peakToDb (meterOutputPeak[0]), std::memory_order_relaxed);
        meterState.outputDb1.store (peakToDb (meterOutputPeak[1]), std::memory_order_relaxed);
    
        meterState.displayInputDb0.store  (peakToDb (displayInputPeak[0]), std::memory_order_relaxed);
        meterState.displayInputDb1.store  (peakToDb (displayInputPeak[1]), std::memory_order_relaxed);
        meterState.displayDetectorDb0.store (peakToDb (displayDetectorPeak[0]), std::memory_order_relaxed);
        meterState.displayDetectorDb1.store (peakToDb (displayDetectorPeak[1]), std::memory_order_relaxed);
    
        meterState.gainReductionDb0.store (totalEffectiveGr0, std::memory_order_relaxed);
        meterState.gainReductionDb1.store (totalEffectiveGr1, std::memory_order_relaxed);
        meterState.displayCeilingGainReductionDb.store (forceBypass ? 0.0f : displayCeilingGainReductionDb,
                                                        std::memory_order_relaxed);
        meterState.keyInputDb.store (peakToDb (keyInputPeak), std::memory_order_relaxed);
    
        updateGainReductionHoldChannel (0, totalEffectiveGr0, numSamples);
        updateGainReductionHoldChannel (1, stereoBus ? totalEffectiveGr1 : 0.0f, numSamples);
        meterState.gainReductionHoldDb0.store (gainReductionHoldDb[0], std::memory_order_relaxed);
        meterState.gainReductionHoldDb1.store (gainReductionHoldDb[1], std::memory_order_relaxed);
    }

    if(stoppedInputIsQuiet && !stoppedDspSleeping)
    {
        uint64_t invalid=0;
        const auto peak=measureInputPeak(buffer,numOutputChannels,invalid);
        stoppedQuietOutputSamples=invalid==0 && peak<=fullIdleOutputFloor
            ? juce::jmin(silenceWait,stoppedQuietOutputSamples+numSamples) : 0;
    }
    else if(!stoppedInputIsQuiet) stoppedQuietOutputSamples=0;
    recordPerformanceBlock(performancePhaseForBlock, performanceStart, numSamples,
                           selectedInputIsZero, transportPlaying, transportAvailable, stoppedDspSleeping, idleGate, idleInput);
}

float QQSuperCompressionAudioProcessor::measureInputPeak(const juce::AudioBuffer<float>& input, int channels,
                                                        uint64_t& nonFinite) noexcept
{
    float peak=0.0f;
    for(int ch=0;ch<channels;++ch)
        for(int i=0;i<input.getNumSamples();++i)
        {
            const auto sample=input.getSample(ch,i);
            if(std::isfinite(sample)) peak=juce::jmax(peak,std::abs(sample));
            else ++nonFinite;
        }
    return peak;
}

void QQSuperCompressionAudioProcessor::recordPerformanceBlock(int phase, int64_t start, int samples,
                                                             bool zero, bool playing, bool known, bool sleeping, int idleGate, const IdleInputDiagnostics& input) noexcept
{
    auto& w = performanceWindows[size_t(phase)];
    if (performancePhase != phase)
    {
        performancePhase = phase;
        w.blocks.store(0, std::memory_order_relaxed); w.ticks.store(0, std::memory_order_relaxed);
        w.sleepBlocks.store(0, std::memory_order_relaxed); w.zeroBlocks.store(0, std::memory_order_relaxed);
        w.playingBlocks.store(0, std::memory_order_relaxed); w.unknownBlocks.store(0, std::memory_order_relaxed);
        w.stoppedInputBlocks.store(0,std::memory_order_relaxed);w.nonFiniteSamples.store(0,std::memory_order_relaxed);
        w.mainPeakMin.store(std::numeric_limits<float>::max(),std::memory_order_relaxed);w.mainPeakMax.store(0,std::memory_order_relaxed);
        w.externalPeakMin.store(std::numeric_limits<float>::max(),std::memory_order_relaxed);w.externalPeakMax.store(0,std::memory_order_relaxed);
    }
    if(known && !playing)
    {
        w.stoppedInputBlocks.store(w.stoppedInputBlocks.load(std::memory_order_relaxed)+1,std::memory_order_relaxed);
        w.nonFiniteSamples.store(w.nonFiniteSamples.load(std::memory_order_relaxed)+input.nonFiniteSamples,std::memory_order_relaxed);
        w.mainPeakMin.store(juce::jmin(w.mainPeakMin.load(std::memory_order_relaxed),input.mainPeak),std::memory_order_relaxed);
        w.mainPeakMax.store(juce::jmax(w.mainPeakMax.load(std::memory_order_relaxed),input.mainPeak),std::memory_order_relaxed);
        if(input.externalSelected)
        {
            w.externalPeakMin.store(juce::jmin(w.externalPeakMin.load(std::memory_order_relaxed),input.externalPeak),std::memory_order_relaxed);
            w.externalPeakMax.store(juce::jmax(w.externalPeakMax.load(std::memory_order_relaxed),input.externalPeak),std::memory_order_relaxed);
        }
    }
    w.selectedKey.store(input.externalSelected?1:0,std::memory_order_relaxed);
    w.externalChannels.store(input.externalChannels,std::memory_order_relaxed);
    // One audio-thread writer; relaxed stores avoid locked read/modify/write.
    w.ticks.store(w.ticks.load(std::memory_order_relaxed) + uint64_t(juce::Time::getHighResolutionTicks()-start), std::memory_order_relaxed);
    w.sleepBlocks.store(w.sleepBlocks.load(std::memory_order_relaxed)+uint64_t(sleeping), std::memory_order_relaxed);
    w.zeroBlocks.store(w.zeroBlocks.load(std::memory_order_relaxed)+uint64_t(zero), std::memory_order_relaxed);
    w.playingBlocks.store(w.playingBlocks.load(std::memory_order_relaxed)+uint64_t(playing), std::memory_order_relaxed);
    w.unknownBlocks.store(w.unknownBlocks.load(std::memory_order_relaxed)+uint64_t(!known), std::memory_order_relaxed);
    w.blockSize.store(samples,std::memory_order_relaxed);
    w.coreFactor.store(currentOversamplingFactor,std::memory_order_relaxed);
    w.ceilingFactor.store(currentLimiterCeilingActive ? qqsc::params::ceilingOversamplingFactorForChoiceIndex(currentCeilingOversamplingChoice) : 1,std::memory_order_relaxed);
    w.idleGate.store(idleGate,std::memory_order_relaxed);
    w.blocks.store(w.blocks.load(std::memory_order_relaxed)+1,std::memory_order_relaxed);
}

juce::String QQSuperCompressionAudioProcessor::getPerformanceDiagnostics() const
{
    juce::String result;
    const char* labels[] = {"FULL closed", "FULL open", "ECO closed", "ECO open"};
    const char* gates[] = {"pending", "transport unknown", "host playing", "parameter change", "nonzero input", "A/B transition", "key/HPF tail", "draining", "Ceiling settling", "sleeping", "ECO transport stop", "host recording", "offline rendering", "FULL residual sleep", "FULL output tail", "FULL gain safety"};
    for (size_t i=0;i<performanceWindows.size();++i)
    {
        const auto& w=performanceWindows[i];
        const auto blocks=w.blocks.load(std::memory_order_relaxed);
        if (blocks==0) continue;
        const auto percent=[blocks](uint64_t n){ return juce::String(100.0*double(n)/double(blocks),1)+"%"; };
        const auto us=1.e6*double(w.ticks.load(std::memory_order_relaxed))/double(juce::Time::getHighResolutionTicksPerSecond())/double(blocks);
        result += "\n"+juce::String(labels[i])+": "+juce::String(us,1)+" us/block, N="+juce::String(w.blockSize.load())
            +", core/ceiling="+juce::String(w.coreFactor.load())+"x/"+juce::String(w.ceilingFactor.load())+"x"
            +"\n  blocks="+juce::String(juce::int64(blocks))+", exact zero="+percent(w.zeroBlocks.load())
            +", sleep="+percent(w.sleepBlocks.load())+", playing="+percent(w.playingBlocks.load())
            +", transport unknown="+percent(w.unknownBlocks.load())
            +"\n  Last idle gate: "+juce::String(gates[juce::jlimit(0,15,w.idleGate.load())]);
        if(w.stoppedInputBlocks.load()>0)
        {
            const auto level=[](float peak)
            {
                return peak==0.f ? juce::String("0 (-inf dBFS)")
                    : juce::String::formatted("%.6e (%.1f dBFS)",double(peak),20.0*std::log10(double(peak)));
            };
            result += "\n  Stopped raw main peak min/max: "+level(w.mainPeakMin.load())+" / "+level(w.mainPeakMax.load());
            const auto extMin=w.externalPeakMin.load();
            result += "\n  Key="+juce::String(w.selectedKey.load()==0 ? "INT" : "EXT")
                +", external bus channels="+juce::String(w.externalChannels.load());
            if(extMin!=std::numeric_limits<float>::max())
                result += "; selected EXT peak min/max: "+level(extMin)+" / "+level(w.externalPeakMax.load());
            else result += "; EXT not measured (not selected during stopped blocks)";
            result += "\n  Stopped measurement blocks="+juce::String(juce::int64(w.stoppedInputBlocks.load()))
                +", nonfinite input samples="+juce::String(juce::int64(w.nonFiniteSamples.load()));
        }
    }
    return result;
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

    const auto currentPeak = blockPeakGrDb;

    // A genuinely deeper reduction becomes the new Hold immediately and starts
    // a fresh two-second timer. Otherwise the existing marker remains visible.
    if (std::abs (currentPeak) > std::abs (gainReductionHoldDb[channel]) + 0.0001f)
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
    snapshot.thresholdDb = getBoundaryForBankDb (0, false, 0);
    snapshot.thresholdLDb = getBoundaryForBankDb (0, false, 1);
    snapshot.thresholdRDb = getBoundaryForBankDb (0, false, 2);
    snapshot.thresholdMDb = getBoundaryForBankDb (0, false, 3);
    snapshot.thresholdSDb = getBoundaryForBankDb (0, false, 4);
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
    snapshot.limiterMode = isLimiterMode();
    snapshot.limiterLink = apvts.getRawParameterValue (qqsc::params::limiterLink)->load() >= 0.5f;
    snapshot.truePeakLimiting = isTruePeakSelected();
    snapshot.tpRecoveryMode = getTpRecoveryMode();
    snapshot.ceilingOversampling = juce::jlimit(0,3,juce::roundToInt(apvts.getRawParameterValue(qqsc::params::ceilingOversampling)->load()));
    snapshot.ceilingDb = apvts.getRawParameterValue (qqsc::params::ceilingDb)->load();
    snapshot.limiterCalibrationDb = apvts.getRawParameterValue(qqsc::params::limiterCalibrationDb)->load();
    snapshot.limiterOutputDb = apvts.getRawParameterValue (qqsc::params::limiterOutputDb)->load();
    snapshot.outputGainDb = apvts.getRawParameterValue (qqsc::params::outputGainDb)->load();
    snapshot.lookaheadMs = qqsc::params::snapLookaheadMs (apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load());
    snapshot.detectorWindowMs = readSoundParameter (qqsc::params::detectorWindowMs);
    snapshot.distort = readSoundParameter(qqsc::params::distort);
    snapshot.limiterStereoLink=readSoundParameter(qqsc::params::limiterStereoLink);
    snapshot.detectorMode = juce::roundToInt (readSoundParameter (qqsc::params::detectorMode));
    snapshot.oversampling = juce::jlimit (0, 3, juce::roundToInt (apvts.getRawParameterValue (qqsc::params::oversampling)->load()));
    snapshot.mode = juce::roundToInt (apvts.getRawParameterValue (qqsc::params::processingMode)->load());
    snapshot.keySource = juce::jlimit (0, 1, juce::roundToInt (apvts.getRawParameterValue (qqsc::params::keySource)->load()));
    snapshot.keyGainDb = apvts.getRawParameterValue (qqsc::params::keyGainDb)->load();
    snapshot.keyHpfHz = apvts.getRawParameterValue (qqsc::params::keyHpfHz)->load();
    snapshot.algorithmMode = juce::jlimit (0, 1, juce::roundToInt (apvts.getRawParameterValue (qqsc::params::algorithmMode)->load()));
    snapshot.upAlgorithmMode=juce::jlimit(0,1,juce::roundToInt(apvts.getRawParameterValue("upAlgorithmMode")->load()));
    snapshot.downAlgorithmMode=juce::jlimit(0,1,juce::roundToInt(apvts.getRawParameterValue("downAlgorithmMode")->load()));
    snapshot.compressionMode = juce::jlimit (0, 1, juce::roundToInt (apvts.getRawParameterValue (qqsc::params::compressionMode)->load()));
    for (size_t d = 0; d < 5; ++d)
    {
        snapshot.range[d] = getBoundaryForBankDb (0, true, static_cast<int> (d));
        snapshot.upThreshold[d] = getBoundaryForBankDb (1, false, static_cast<int> (d));
        snapshot.downThreshold[d] = getBoundaryForBankDb (1, true, static_cast<int> (d));
        snapshot.upRatio[d] = apvts.getRawParameterValue (qqsc::params::upRatioIds[d])->load();
        snapshot.upEnabled[d] = apvts.getRawParameterValue (qqsc::params::upEnabledIds[d])->load() >= 0.5f;
        snapshot.downRatio[d] = apvts.getRawParameterValue (qqsc::params::downRatioIds[d])->load();
        snapshot.downEnabled[d] = apvts.getRawParameterValue (qqsc::params::downEnabledIds[d])->load() >= 0.5f;
    }
    for(size_t i=0;i<35;++i) snapshot.limiterSound[i]=apvts.getRawParameterValue(qqsc::params::limiterSoundIds[i])->load();
    for(size_t d=0;d<5;++d)
    {
        snapshot.limiterSound[15+d]=getBoundaryForBankDb(2,false,int(d));
        snapshot.limiterSound[20+d]=getBoundaryForBankDb(2,true,int(d));
        snapshot.limiterSound[25+d]=getBoundaryForBankDb(3,false,int(d));
        snapshot.limiterSound[30+d]=getBoundaryForBankDb(3,true,int(d));
    }
    for(size_t i=0;i<snapshot.limiterSettings.size();++i)
        snapshot.limiterSettings[i]=apvts.getRawParameterValue(qqsc::params::limiterModeIds[i])->load();
    snapshot.limiterSettings[30]=qqsc::params::snapLookaheadMs(snapshot.limiterSettings[30]);
    // Use the mode published with the canonical boundaries, including when a
    // host callback runs before APVTS exposes its updated raw parameter value.
    snapshot.limiterSettings[12] = static_cast<float> (limiterCompressionModeForAudio.load (std::memory_order_acquire));
    snapshot.limiterInitialised=limiterBankInitialised.load(std::memory_order_acquire);
    snapshot.domainLink=apvts.getRawParameterValue(qqsc::params::domainLink)->load()>=.5f;
    snapshot.dualRatioLink=apvts.getRawParameterValue(qqsc::params::dualRatioLink)->load()>=.5f;
    snapshot.inputOutputLink=apvts.getRawParameterValue(qqsc::params::inputOutputLink)->load()>=.5f;
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

void QQSuperCompressionAudioProcessor::migrateLimiterStereoSnapshot(ParameterSnapshot& s) noexcept
{
    const int oldMode=juce::roundToInt(s.limiterSettings[13]);
    if(oldMode!=qqsc::params::leftRight)
    {
        const size_t l=oldMode==qqsc::params::midSide ? 3u : 0u;
        const size_t r=oldMode==qqsc::params::midSide ? 4u : 0u;
        for(size_t first=0;first<35;first+=5)
        {
            s.limiterSound[first+1]=s.limiterSound[first+l];
            s.limiterSound[first+2]=s.limiterSound[first+r];
        }
        for(size_t first:{1u,6u,16u,21u})
        {
            s.limiterSettings[first+1]=s.limiterSettings[first+l];
            s.limiterSettings[first+2]=s.limiterSettings[first+r];
        }
    }
    s.limiterStereoLink=oldMode==qqsc::params::stereoLinked ? 100.0f : 0.0f;
    s.limiterSettings[13]=float(qqsc::params::leftRight);
}

QQSuperCompressionAudioProcessor::ParameterSnapshot QQSuperCompressionAudioProcessor::activeSnapshot(ParameterSnapshot s) noexcept
{
    if(!s.limiterMode) return s;
    s.inputGainDb=float(s.limiterSettings[0]);
    s.makeupST=float(s.limiterSettings[1]);
    s.makeupL=float(s.limiterSettings[2]);
    s.makeupR=s.makeupL; // Shared Limiter controls; independent audio detectors.
    s.makeupM=float(s.limiterSettings[4]);
    s.makeupS=float(s.limiterSettings[5]);
    s.mix=float(s.limiterSettings[6]);
    s.mixL=float(s.limiterSettings[7]);
    s.mixR=s.mixL;
    s.mixM=float(s.limiterSettings[9]);
    s.mixS=float(s.limiterSettings[10]);
    s.algorithmMode=int(s.limiterSettings[11]);
    s.upAlgorithmMode=int(s.limiterSettings[31]);s.downAlgorithmMode=int(s.limiterSettings[32]);
    s.compressionMode=int(s.limiterSettings[12]);
    s.mode=qqsc::params::leftRight;
    s.domainLink=bool(s.limiterSettings[14]);
    s.dualRatioLink=bool(s.limiterSettings[15]);
    for(size_t d=0;d<5;++d) { s.upEnabled[d]=s.limiterSettings[16+d]>=.5f; s.downEnabled[d]=s.limiterSettings[21+d]>=.5f; }
    s.upEnabled[2]=s.upEnabled[1];s.downEnabled[2]=s.downEnabled[1];
    s.keySource=int(s.limiterSettings[26]);s.keyGainDb=s.limiterSettings[27];s.keyHpfHz=s.limiterSettings[28];s.oversampling=int(s.limiterSettings[29]);s.lookaheadMs=qqsc::params::snapLookaheadMs(s.limiterSettings[30]);
    return s;
}

qqsc::ABTransfer QQSuperCompressionAudioProcessor::makeABTransfer (const ParameterSnapshot& source) noexcept
{
    auto s=activeSnapshot(source);
    if(s.limiterMode)
    {
        s.ratio=s.limiterSound[0];s.ratioL=s.limiterSound[1];s.ratioR=s.limiterSound[2];s.ratioM=s.limiterSound[3];s.ratioS=s.limiterSound[4];
        s.thresholdDb=s.limiterSound[15];s.thresholdLDb=s.limiterSound[16];s.thresholdRDb=s.limiterSound[17];s.thresholdMDb=s.limiterSound[18];s.thresholdSDb=s.limiterSound[19];
        for(size_t d=0;d<5;++d)
        {s.upRatio[d]=s.limiterSound[5+d];s.downRatio[d]=s.limiterSound[10+d];s.range[d]=s.limiterSound[20+d];s.upThreshold[d]=s.limiterSound[25+d];s.downThreshold[d]=s.limiterSound[30+d];}
        s.ratioR=s.ratioL;s.upRatio[2]=s.upRatio[1];s.downRatio[2]=s.downRatio[1];
        s.thresholdRDb=s.thresholdLDb;s.range[2]=s.range[1];
        s.upThreshold[2]=s.upThreshold[1];s.downThreshold[2]=s.downThreshold[1];
    }
    qqsc::ABTransfer t;
    t.dual=s.compressionMode==1; t.mode=s.mode;
    t.stereoLink=s.limiterMode ? s.limiterStereoLink*.01f : 0.0f;
    t.algorithm=s.algorithmMode==0 ? qqsc::CompressionAlgorithm::classic : qqsc::CompressionAlgorithm::super;
    t.upAlgorithm=s.upAlgorithmMode==0 ? qqsc::CompressionAlgorithm::classic : qqsc::CompressionAlgorithm::super;
    t.downAlgorithm=s.downAlgorithmMode==0 ? qqsc::CompressionAlgorithm::classic : qqsc::CompressionAlgorithm::super;
    t.ratio={s.ratio,s.ratioL,s.ratioR,s.ratioM,s.ratioS};
    t.lower={s.thresholdDb,s.thresholdLDb,s.thresholdRDb,s.thresholdMDb,s.thresholdSDb};
    t.makeup={s.makeupST,s.makeupL,s.makeupR,s.makeupM,s.makeupS};
    t.mix={s.mix,s.mixL,s.mixR,s.mixM,s.mixS};
    t.output=juce::Decibels::decibelsToGain(s.limiterMode ? s.limiterOutputDb+s.ceilingDb+s.limiterCalibrationDb : s.outputGainDb);
    t.unityOutput=juce::Decibels::decibelsToGain(s.limiterMode ? s.limiterOutputDb : 0.f);
    for(size_t d=0;d<5;++d)
    {
        t.lower[d]=qqsc::params::thresholdLinear(t.dual?s.upThreshold[d]:t.lower[d]);
        t.upper[d]=t.dual?qqsc::params::thresholdLinear(s.downThreshold[d]):qqsc::params::rangeLinear(s.range[d]);
        if(s.limiterMode)
        {
            const auto floor=qqsc::params::thresholdLinear(qqsc::params::limiterThresholdMinimumDb);
            t.lower[d]=juce::jlimit(floor,1.0f,t.lower[d]);
            t.upper[d]=t.dual ? juce::jlimit(t.lower[d],1.0f,t.upper[d])
                               : qqsc::params::rangeLinear(qqsc::params::rangeOffDb);
        }
        t.upRatio[d]=s.upEnabled[d]?qqsc::params::upwardRatio(s.upRatio[d],s.limiterMode):1.f;
        t.downRatio[d]=s.downEnabled[d]?qqsc::params::limiterRatio(s.downRatio[d],s.limiterMode,true):1.f;
        t.ratio[d]=qqsc::params::limiterRatio(t.ratio[d],s.limiterMode);
        t.makeup[d]=juce::Decibels::decibelsToGain(t.makeup[d], -180.0f); t.mix[d]*=.01f;
    }
    return t;
}

void QQSuperCompressionAudioProcessor::queueABTransfer (const ParameterSnapshot& from, const ParameterSnapshot& to)
{
    const juce::SpinLock::ScopedLockType lock(abTransferLock);
    // A gain-only result crossfade is valid when both banks share the carrier
    // and detector timing/source. Latency/source changes retain their existing
    // reconfiguration path; never blend differently delayed signals here.
    const auto a=activeSnapshot(from), b=activeSnapshot(to);
    requestedABCompatible=a.inputGainDb==b.inputGainDb && a.lookaheadMs==b.lookaheadMs
        && a.distort==b.distort && a.detectorMode==b.detectorMode
        && a.oversampling==b.oversampling && a.keySource==b.keySource
        && a.keyGainDb==b.keyGainDb && a.keyHpfHz==b.keyHpfHz
        && a.limiterMode==b.limiterMode
        && (!a.limiterMode || (a.ceilingOversampling==b.ceilingOversampling
                              && a.truePeakLimiting==b.truePeakLimiting));
    // Preserve the audible source when several clicks arrive before the next
    // callback. Reading APVTS at the end of an audio block could already see
    // the destination even though that block still rendered the old bank.
    if (! abTransferPending.load (std::memory_order_relaxed))
        requestedABFrom=makeABTransfer(from);
    requestedABTo=makeABTransfer(to);
    abTransferPending.store(true,std::memory_order_release);
}

void QQSuperCompressionAudioProcessor::applySnapshot (const ParameterSnapshot& snapshot)
{
    algorithmPreferenceInitialised.store (true);
    for(auto& initialised:dualAlgorithmPreferenceInitialised)initialised.store(true);
    queueABTransfer (captureCurrentSnapshot(), snapshot);
    restoringDynamicsState.store (true, std::memory_order_release);
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
    for(size_t i=0;i<35;++i) setActualParameterValue(qqsc::params::limiterSoundIds[i],snapshot.limiterSound[i]);
    for(size_t i=0;i<snapshot.limiterSettings.size();++i) setActualParameterValue(qqsc::params::limiterModeIds[i],snapshot.limiterSettings[i]);
    limiterBankInitialised.store(snapshot.limiterInitialised,std::memory_order_release);
    setActualParameterValue(qqsc::params::domainLink,snapshot.domainLink ? 1.f : 0.f);
    setActualParameterValue(qqsc::params::dualRatioLink,snapshot.dualRatioLink ? 1.f : 0.f);
    setActualParameterValue(qqsc::params::inputOutputLink,snapshot.inputOutputLink ? 1.f : 0.f);
    setActualParameterValue (qqsc::params::limiterMode, snapshot.limiterMode ? 1.0f : 0.0f);
    setActualParameterValue (qqsc::params::limiterLink, snapshot.limiterLink ? 1.0f : 0.0f);
    setActualParameterValue (qqsc::params::truePeakLimiting, snapshot.truePeakLimiting ? 1.0f : 0.0f);
    setActualParameterValue (qqsc::params::tpRecoveryMode, float(snapshot.tpRecoveryMode));
    setActualParameterValue (qqsc::params::ceilingOversampling, float(snapshot.ceilingOversampling));
    setActualParameterValue (qqsc::params::ceilingDb, snapshot.ceilingDb);
    setActualParameterValue (qqsc::params::limiterOutputDb, snapshot.limiterOutputDb);
    setActualParameterValue (qqsc::params::limiterCalibrationDb, snapshot.limiterCalibrationDb);
    setActualParameterValue (qqsc::params::outputGainDb, snapshot.outputGainDb);
    const auto snapshotLookaheadMs = qqsc::params::snapLookaheadMs (snapshot.lookaheadMs);
    setActualParameterValue (qqsc::params::lookaheadMs, snapshotLookaheadMs);
    setActualParameterValue (qqsc::params::detectorWindowMs, snapshot.detectorWindowMs);
    setActualParameterValue (qqsc::params::distort, snapshot.distort);
    setActualParameterValue(qqsc::params::limiterStereoLink,snapshot.limiterStereoLink);
    distortForAudio.store(snapshot.distort,std::memory_order_release);
    setActualParameterValue (qqsc::params::detectorMode, float(snapshot.detectorMode));
    setActualParameterValue (qqsc::params::oversampling, static_cast<float> (juce::jlimit (0, 3, snapshot.oversampling)));
    notifyHostProcessingLatency();
    setActualParameterValue (qqsc::params::processingMode, static_cast<float> (snapshot.mode));
    setActualParameterValue (qqsc::params::keySource, static_cast<float> (juce::jlimit (0, 1, snapshot.keySource)));
    setActualParameterValue (qqsc::params::keyGainDb, snapshot.keyGainDb);
    setActualParameterValue (qqsc::params::keyHpfHz, snapshot.keyHpfHz);
    setActualParameterValue (qqsc::params::algorithmMode, static_cast<float> (snapshot.algorithmMode));
    setActualParameterValue("upAlgorithmMode",float(snapshot.upAlgorithmMode));
    setActualParameterValue("downAlgorithmMode",float(snapshot.downAlgorithmMode));
    setActualParameterValue (qqsc::params::compressionMode, static_cast<float> (snapshot.compressionMode));
    for (size_t d = 0; d < 5; ++d)
    {
        setActualParameterValue (qqsc::params::rangeIds[d], snapshot.range[d]);
        setActualParameterValue (qqsc::params::upThresholdIds[d], snapshot.upThreshold[d]);
        setActualParameterValue (qqsc::params::downThresholdIds[d], snapshot.downThreshold[d]);
        setActualParameterValue (qqsc::params::upRatioIds[d], snapshot.upRatio[d]);
        setActualParameterValue (qqsc::params::upEnabledIds[d], snapshot.upEnabled[d] ? 1.0f : 0.0f);
        setActualParameterValue (qqsc::params::downRatioIds[d], snapshot.downRatio[d]);
        setActualParameterValue (qqsc::params::downEnabledIds[d], snapshot.downEnabled[d] ? 1.0f : 0.0f);
    }
    rebuildBoundaryPairs();
    restoringDynamicsState.store (false, std::memory_order_release);

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
        for(size_t i=0;i<35;++i) state.setProperty(abProperty(prefix+qqsc::params::limiterSoundIds[i]),s.limiterSound[i],nullptr);
        for(size_t i=0;i<s.limiterSettings.size();++i) state.setProperty(abProperty(prefix+qqsc::params::limiterModeIds[i]),s.limiterSettings[i],nullptr);
        state.setProperty(abProperty(prefix+"limiterInitialised"),s.limiterInitialised,nullptr);
        state.setProperty(abProperty(prefix+"domainLink"),s.domainLink,nullptr);
        state.setProperty(abProperty(prefix+"dualRatioLink"),s.dualRatioLink,nullptr);
        state.setProperty(abProperty(prefix+"inputOutputLink"),s.inputOutputLink,nullptr);
        state.setProperty (abProperty (prefix + "limiterMode"), s.limiterMode, nullptr);
        state.setProperty (abProperty (prefix + "limiterLink"), s.limiterLink, nullptr);
        state.setProperty (abProperty (prefix + "truePeakLimiting"), s.truePeakLimiting, nullptr);
        state.setProperty (abProperty (prefix + "tpRecoveryMode"), s.tpRecoveryMode, nullptr);
        state.setProperty (abProperty (prefix + "ceilingOversampling"), s.ceilingOversampling, nullptr);
        state.setProperty (abProperty (prefix + "ceilingDb"), s.ceilingDb, nullptr);
        state.setProperty(abProperty(prefix+"limiterCalibrationDb"),s.limiterCalibrationDb,nullptr);
        state.setProperty (abProperty (prefix + "limiterOutputDb"), s.limiterOutputDb, nullptr);
        state.setProperty (abProperty (prefix + "outputGainDb"), s.outputGainDb, nullptr);
        state.setProperty (abProperty (prefix + "lookaheadMs"), s.lookaheadMs, nullptr);
        state.setProperty (abProperty (prefix + "detectorWindowMs"), s.detectorWindowMs, nullptr);
        state.setProperty(abProperty(prefix + "distort"),s.distort,nullptr);
        state.setProperty(abProperty(prefix+"limiterStereoLink"),s.limiterStereoLink,nullptr);
        state.setProperty (abProperty (prefix + "detectorMode"), s.detectorMode, nullptr);
        state.setProperty (abProperty (prefix + "oversampling"), s.oversampling, nullptr);
        state.setProperty (abProperty (prefix + "mode"), s.mode, nullptr);
        state.setProperty (abProperty (prefix + "keySource"), s.keySource, nullptr);
        state.setProperty (abProperty (prefix + "keyGainDb"), s.keyGainDb, nullptr);
        state.setProperty (abProperty (prefix + "keyHpfHz"), s.keyHpfHz, nullptr);
        state.setProperty (abProperty (prefix + "algorithmMode"), s.algorithmMode, nullptr);
        state.setProperty(abProperty(prefix+"upAlgorithmMode"),s.upAlgorithmMode,nullptr);
        state.setProperty(abProperty(prefix+"downAlgorithmMode"),s.downAlgorithmMode,nullptr);
        state.setProperty (abProperty (prefix + "compressionMode"), s.compressionMode, nullptr);
        for (size_t d = 0; d < 5; ++d)
        {
            state.setProperty (abProperty (prefix + qqsc::params::rangeIds[d]), s.range[d], nullptr);
            state.setProperty (abProperty (prefix + qqsc::params::upThresholdIds[d]), s.upThreshold[d], nullptr);
            state.setProperty (abProperty (prefix + qqsc::params::downThresholdIds[d]), s.downThreshold[d], nullptr);
            state.setProperty (abProperty (prefix + qqsc::params::upRatioIds[d]), s.upRatio[d], nullptr);
            state.setProperty (abProperty (prefix + qqsc::params::upEnabledIds[d]), s.upEnabled[d], nullptr);
            state.setProperty (abProperty (prefix + qqsc::params::downRatioIds[d]), s.downRatio[d], nullptr);
            state.setProperty (abProperty (prefix + qqsc::params::downEnabledIds[d]), s.downEnabled[d], nullptr);
        }
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
            const bool hadSnapshotLimiterBank = state.hasProperty(abProperty(prefix+qqsc::params::limiterSoundIds[0]));
            for(size_t i=0;i<35;++i)
            {
                auto* parameter = apvts.getParameter(qqsc::params::limiterSoundIds[i]);
                const auto fallbackValue = hadSnapshotLimiterBank ? s.limiterSound[i]
                    : parameter->convertFrom0to1(parameter->getDefaultValue());
                s.limiterSound[i]=float(state.getProperty(abProperty(prefix+qqsc::params::limiterSoundIds[i]),fallbackValue));
            }
            s.limiterMode = static_cast<bool> (state.getProperty (abProperty (prefix + "limiterMode"), false));
            s.limiterLink = static_cast<bool> (state.getProperty (abProperty (prefix + "limiterLink"), true));
            s.truePeakLimiting = static_cast<bool> (state.getProperty (abProperty (prefix + "truePeakLimiting"), false));
            s.tpRecoveryMode = juce::jlimit(int(qqsc::params::tpTight),int(qqsc::params::tpSmooth),
                int(state.getProperty(abProperty(prefix + "tpRecoveryMode"),qqsc::params::tpAuto)));
            const bool legacyCeiling=int(state.getProperty(stateSchemaProperty,0))<28;
            const int savedCeiling=int(state.getProperty(abProperty(prefix + "ceilingOversampling"),
                legacyCeiling ? 1 : int(qqsc::params::ceiling8x)));
            s.ceilingOversampling = legacyCeiling ? qqsc::params::migrateLegacyOversamplingChoice(savedCeiling)
                : juce::jlimit(0,3,savedCeiling);
            s.ceilingDb = static_cast<float> (state.getProperty (abProperty (prefix + "ceilingDb"), 0.0f));
            s.limiterCalibrationDb = float(state.getProperty(abProperty(prefix+"limiterCalibrationDb"),0.0f));
            s.limiterOutputDb = static_cast<float> (state.getProperty (abProperty (prefix + "limiterOutputDb"), 0.0f));
            s.outputGainDb = static_cast<float> (state.getProperty (abProperty (prefix + "outputGainDb"), s.outputGainDb));
            s.lookaheadMs = qqsc::params::snapLookaheadMs (
                static_cast<float> (state.getProperty (abProperty (prefix + "lookaheadMs"), s.lookaheadMs)));
            s.detectorWindowMs = juce::jlimit (0.0f, 100.0f,
                float(state.getProperty (abProperty (prefix + "detectorWindowMs"), 100.0f)));
            s.detectorMode = juce::jlimit (0, 1,
                int(state.getProperty (abProperty (prefix + "detectorMode"), s.detectorMode)));
            if(state.hasProperty(abProperty(prefix+"oversampling")))
            {
                const int storedOversampling=int(state.getProperty(abProperty(prefix+"oversampling")));
                s.oversampling = legacyOversamplingSchema
                    ? (storedOversampling<=0 ? qqsc::params::osNative : qqsc::params::os8x)
                    : int(state.getProperty(stateSchemaProperty,0))<28
                        ? qqsc::params::migrateLegacyOversamplingChoice(storedOversampling)
                        : juce::jlimit(0,3,storedOversampling);
            }
            s.mode = static_cast<int> (state.getProperty (abProperty (prefix + "mode"), s.mode));
            s.keySource = juce::jlimit (0, 1, static_cast<int> (
                state.getProperty (abProperty (prefix + "keySource"), qqsc::params::keyInternal)));
            s.keyGainDb = static_cast<float> (
                state.getProperty (abProperty (prefix + "keyGainDb"), 0.0f));
            s.keyHpfHz = static_cast<float> (
                state.getProperty (abProperty (prefix + "keyHpfHz"), qqsc::params::keyHpfOffHz));
            s.algorithmMode = juce::jlimit (0, 1, static_cast<int> (state.getProperty (abProperty (prefix + "algorithmMode"), s.algorithmMode)));
            s.upAlgorithmMode=juce::jlimit(0,1,int(state.getProperty(abProperty(prefix+"upAlgorithmMode"),s.algorithmMode)));
            s.downAlgorithmMode=juce::jlimit(0,1,int(state.getProperty(abProperty(prefix+"downAlgorithmMode"),s.algorithmMode)));
            s.compressionMode = juce::jlimit (0, 1, static_cast<int> (state.getProperty (abProperty (prefix + "compressionMode"), 0)));
            for (size_t d = 0; d < 5; ++d)
            {
                s.range[d] = static_cast<float> (state.getProperty (abProperty (prefix + qqsc::params::rangeIds[d]), qqsc::params::rangeOffDb));
                s.upThreshold[d] = static_cast<float> (state.getProperty (abProperty (prefix + qqsc::params::upThresholdIds[d]), qqsc::params::thresholdOffDb));
                s.downThreshold[d] = static_cast<float> (state.getProperty (abProperty (prefix + qqsc::params::downThresholdIds[d]), 0.0f));
                s.upRatio[d] = static_cast<float> (state.getProperty (abProperty (prefix + qqsc::params::upRatioIds[d]), 1.0f));
                s.upEnabled[d] = static_cast<bool> (state.getProperty (abProperty (prefix + qqsc::params::upEnabledIds[d]), true));
                s.downRatio[d] = static_cast<float> (state.getProperty (abProperty (prefix + qqsc::params::downRatioIds[d]), 1.0f));
                s.downEnabled[d] = static_cast<bool> (state.getProperty (abProperty (prefix + qqsc::params::downEnabledIds[d]), true));
            }
            s.domainLink=bool(state.getProperty(abProperty(prefix+"domainLink"),fallback.domainLink));
            s.dualRatioLink=bool(state.getProperty(abProperty(prefix+"dualRatioLink"),fallback.dualRatioLink));
            s.inputOutputLink=bool(state.getProperty(abProperty(prefix+"inputOutputLink"),fallback.inputOutputLink));
            const std::array<float,31> legacySettings { float(s.inputGainDb),float(s.makeupST),float(s.makeupL),float(s.makeupR),float(s.makeupM),float(s.makeupS),float(s.mix),float(s.mixL),float(s.mixR),float(s.mixM),float(s.mixS),float(s.algorithmMode),float(s.compressionMode),float(s.mode),float(s.domainLink),float(s.dualRatioLink),float(s.upEnabled[0]),float(s.upEnabled[1]),float(s.upEnabled[2]),float(s.upEnabled[3]),float(s.upEnabled[4]),float(s.downEnabled[0]),float(s.downEnabled[1]),float(s.downEnabled[2]),float(s.downEnabled[3]),float(s.downEnabled[4]),float(s.keySource),s.keyGainDb,s.keyHpfHz,float(s.oversampling),s.lookaheadMs };
            for(size_t i=0;i<s.limiterSettings.size();++i)
            {
                auto* parameter = apvts.getParameter(qqsc::params::limiterModeIds[i]);
                const auto legacyValue = i<31 ? legacySettings[i]
                    : state.hasProperty(abProperty(prefix+"limiterAlgorithmMode"))
                        ? s.limiterSettings[11] : float(i==31 ? s.upAlgorithmMode : s.downAlgorithmMode);
                const auto fallbackValue = hadSnapshotLimiterBank ? legacyValue
                    : parameter->convertFrom0to1(parameter->getDefaultValue());
                const auto property=abProperty(prefix+qqsc::params::limiterModeIds[i]);
                s.limiterSettings[i]=float(state.getProperty(property,fallbackValue));
                if(i==30) s.limiterSettings[i]=qqsc::params::snapLookaheadMs(s.limiterSettings[i]);
                if(i==29 && state.hasProperty(property) && int(state.getProperty(stateSchemaProperty,0))<28)
                    s.limiterSettings[i]=float(qqsc::params::migrateLegacyOversamplingChoice(
                        juce::roundToInt(s.limiterSettings[i])));
            }
            s.limiterInitialised=bool(state.getProperty(abProperty(prefix+"limiterInitialised"),state.hasProperty(abProperty(prefix+"limiterRatio"))));
            if (int(state.getProperty(stateSchemaProperty,0)) < 23 && !s.limiterInitialised)
                for (size_t d = 0; d < 5; ++d)
                    if (s.limiterSound[15+d] <= -120.0f) s.limiterSound[15+d] = 0.0f;
            // Clamp only the Normal bank, after any legacy Limiter migration.
            for (auto* makeup : { &s.makeupST, &s.makeupL, &s.makeupR, &s.makeupM, &s.makeupS })
                *makeup = juce::jlimit(-qqsc::normalMaximumMakeupDb, qqsc::normalMaximumMakeupDb, *makeup);
            s.inputGainDb = juce::jlimit(-24.0f, 24.0f, s.inputGainDb);
            s.outputGainDb = juce::jlimit(-24.0f, 24.0f, s.outputGainDb);
        };

        read ("A_", newA);
        read ("B_", newB);
        newActive = juce::jlimit (0, 1, static_cast<int> (state.getProperty (abProperty ("active"), 0)));
    }

    const juce::ScopedLock lock (abLock);
    for(auto entry : {std::make_pair("A_",&newA),std::make_pair("B_",&newB)})
    {
        auto& saved=*entry.second;
        const auto legacy=qqsc::params::distortForWindowMs(activeSnapshot(saved).lookaheadMs,saved.detectorWindowMs);
        if(!state.hasProperty(abProperty(juce::String(entry.first)+"limiterStereoLink")))
            migrateLimiterStereoSnapshot(saved);
        saved.limiterStereoLink=juce::jlimit(0.0f,100.0f,float(state.getProperty(abProperty(juce::String(entry.first)+"limiterStereoLink"),saved.limiterStereoLink)));
        saved.distort=juce::jlimit(0.0f,100.0f,float(state.getProperty(abProperty(juce::String(entry.first)+"distort"),legacy)));
    }
    snapshotA = newA;
    snapshotB = newB;
    activeABSlot.store (newActive, std::memory_order_relaxed);
}

void QQSuperCompressionAudioProcessor::resetMatchAccumulator() noexcept
{
    matchGeneration.fetch_add(1,std::memory_order_acq_rel);
    loudnessMatch.reset();
    unityMixMatch.reset();
    unityMatchSettling=int(currentSampleRate*.4);
    matchSTValid.store (false, std::memory_order_relaxed);
    matchLValid.store  (false, std::memory_order_relaxed);
    matchRValid.store  (false, std::memory_order_relaxed);
    matchMValid.store  (false, std::memory_order_relaxed);
    matchSValid.store  (false, std::memory_order_relaxed);
    matchGeneration.fetch_add(1,std::memory_order_release);
}

void QQSuperCompressionAudioProcessor::refreshMatchResults()
{
    if (! shouldRunUiAnalysis()) return;
    loudnessMatch.servicePending();
    unityMixMatch.servicePending();
    if(loudnessMatch.isQueueOverloaded() || unityMixMatch.isQueueOverloaded())
    {
        matchReady.store(false,std::memory_order_relaxed);
        // Missing queued history cannot be labelled a complete playback pass.
        matchPassComplete.store(false,std::memory_order_release);
        return;
    }
    if (! resetMatchOnNextPlaybackBlock.load(std::memory_order_acquire))
        updateMatchResults();
}

void QQSuperCompressionAudioProcessor::updateMatchResults() noexcept
{
    const auto publishedGeneration = matchGeneration.load(std::memory_order_acquire);
    if((publishedGeneration & 1u)!=0u) return;
    const auto& result = loudnessMatch.getLatestMatch();
    // The dry component is silent at 100% wet Mix. Its standalone MATCH
    // validity cannot gate the joint mixed-energy correction.
    if(isUnityMonitorActive() && (unityMixMatch.getBlockCount()==0
        || unityMixMatch.getBlockCount()!=loudnessMatch.getBlockCount()))
    {
        matchReady.store(false,std::memory_order_relaxed);
        return;
    }

    const auto stDelta = isUnityMonitorActive()
        ? unityMixMatch.makeupAdjustmentForMixedGain(result.st) : result.st;
    matchSTDb.store (stDelta, std::memory_order_relaxed);
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
                                   juce::roundToInt (readSoundParameter(qqsc::params::processingMode)));

    bool ready = result.validST;
    if (isUnityMonitorActive() || isLimiterMode())
        ready=result.validST;
    else if (mode == qqsc::params::leftRight)
        ready = result.validL || result.validR;
    else if (mode == qqsc::params::midSide)
        ready = result.validM || result.validS;

    matchPublishedGeneration.store(publishedGeneration,std::memory_order_release);
    matchReady.store (ready, std::memory_order_relaxed);
}

bool QQSuperCompressionAudioProcessor::hasMatchData() const noexcept
{
    if(!matchPassComplete.load(std::memory_order_acquire)) return false;
    if(loudnessMatch.isQueueOverloaded() || (isUnityMonitorActive() && unityMixMatch.isQueueOverloaded()))
        return false;
    if(resetMatchOnNextPlaybackBlock.load(std::memory_order_acquire)
        || matchPublishedGeneration.load(std::memory_order_acquire)!=matchGeneration.load(std::memory_order_acquire))
        return false;
    if (! matchReady.load (std::memory_order_relaxed))
        return false;

    if(isUnityMonitorActive() || isLimiterMode()) return matchSTValid.load(std::memory_order_relaxed);

    const int mode = juce::jlimit (static_cast<int> (qqsc::params::stereoLinked),
                                   static_cast<int> (qqsc::params::leftRight),
                                   juce::roundToInt (readSoundParameter(qqsc::params::processingMode)));

    if (mode == qqsc::params::leftRight)
        return matchLValid.load (std::memory_order_relaxed) || matchRValid.load (std::memory_order_relaxed);
    if (mode == qqsc::params::midSide)
        return matchMValid.load (std::memory_order_relaxed) || matchSValid.load (std::memory_order_relaxed);
    return matchSTValid.load (std::memory_order_relaxed);
}

bool QQSuperCompressionAudioProcessor::applyMatchForCurrentMode()
{
    refreshMatchResults(); // Include pending analysis blocks up to this click.
    if (! hasMatchData())
        return false;

    const int mode = juce::jlimit (static_cast<int> (qqsc::params::stereoLinked),
                                   static_cast<int> (qqsc::params::leftRight),
                                   juce::roundToInt (readSoundParameter(qqsc::params::processingMode)));

    struct MatchWriteScope
    {
        std::atomic<bool>& active;
        explicit MatchWriteScope(std::atomic<bool>& flag):active(flag){active.store(true,std::memory_order_release);}
        ~MatchWriteScope(){active.store(false,std::memory_order_release);}
    } scope(applyingCumulativeMatch);
    undoManager.beginNewTransaction ("Match Makeup Gain");

    // MATCH is a Makeup edit, not a new absolute Output calibration. Preserve
    // the user's existing Output offset, including after Ratio/Mix/mode edits.
    // Use the mixed reference for this measured loudness correction only.
    const bool linkOutput=isLimiterLinked();
    const auto outputBefore=readSoundParameter(qqsc::params::outputGainDb);
    const auto referenceBefore=linkOutput ? getLimiterReferencePeakDb() : 0.0f;
    const auto linkMakeupChange=[&]
    {
        if(linkOutput)
            setActualParameterValue(qqsc::params::limiterOutputDb,
                outputBefore+referenceBefore-getLimiterReferencePeakDb());
    };

    if(isLimiterMode())
    {
        // Both ordinary and unity-monitor statistics now publish an
        // absolute shared Makeup target from the complete retained capture.
        const float target=matchSTDb.load(std::memory_order_relaxed);
        setActualParameterValue(soundParameterID(qqsc::params::makeupGainLDb),
            juce::jlimit(-qqsc::maximumMakeupDb,qqsc::maximumMakeupDb,target));
        linkMakeupChange();
        return true;
    }
    if(isUnityMonitorActive())
    {
        // A common correction preserves existing L/R or M/S balance. When
        // linked, adjusting Makeup also updates the normal output reference;
        // the 1:1 monitor makes that Makeup correction audible after TP.
        float delta=juce::jlimit(-120.f,120.f,matchSTDb.load(std::memory_order_relaxed));
        const auto limit=[&](const char* id)
        {
            const auto value=readSoundParameter(id);
            delta=juce::jlimit(-qqsc::maximumMakeupDb-value,qqsc::maximumMakeupDb-value,delta);
        };
        if(mode==qqsc::params::leftRight) {limit(qqsc::params::makeupGainLDb);limit(qqsc::params::makeupGainRDb);}
        else if(mode==qqsc::params::midSide) {limit(qqsc::params::makeupGainMDb);limit(qqsc::params::makeupGainSDb);}
        else limit(qqsc::params::makeupGainDb);
        const auto correct=[&](const char* id)
        { setActualParameterValue(soundParameterID(id),readSoundParameter(id)+delta); };
        if(mode==qqsc::params::leftRight) {correct(qqsc::params::makeupGainLDb);correct(qqsc::params::makeupGainRDb);}
        else if(mode==qqsc::params::midSide) {correct(qqsc::params::makeupGainMDb);correct(qqsc::params::makeupGainSDb);}
        else correct(qqsc::params::makeupGainDb);
        linkMakeupChange();
        return true;
    }

    if (mode == qqsc::params::leftRight)
    {
        if (matchLValid.load (std::memory_order_relaxed))
            setActualParameterValue(soundParameterID(qqsc::params::makeupGainLDb), matchLDb.load (std::memory_order_relaxed));
        if (matchRValid.load (std::memory_order_relaxed))
            setActualParameterValue(soundParameterID(qqsc::params::makeupGainRDb), matchRDb.load (std::memory_order_relaxed));
    }
    else if (mode == qqsc::params::midSide)
    {
        if (matchMValid.load (std::memory_order_relaxed))
            setActualParameterValue(soundParameterID(qqsc::params::makeupGainMDb), matchMDb.load (std::memory_order_relaxed));
        if (matchSValid.load (std::memory_order_relaxed))
            setActualParameterValue(soundParameterID(qqsc::params::makeupGainSDb), matchSDb.load (std::memory_order_relaxed));
    }
    else if (matchSTValid.load (std::memory_order_relaxed))
    {
        setActualParameterValue(soundParameterID(qqsc::params::makeupGainDb), matchSTDb.load (std::memory_order_relaxed));
    }

    linkMakeupChange();
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
    state.getChildWithProperty("id",qqsc::params::distort).setProperty("value",readSoundParameter(qqsc::params::distort),nullptr);
    state.getChildWithProperty("id",qqsc::params::detectorWindowMs).setProperty("value",readSoundParameter(qqsc::params::detectorWindowMs),nullptr);
    writeCanonicalBoundariesTo (state);
    state.setProperty (stateSchemaProperty, currentStateSchemaVersion, nullptr);
    state.setProperty (performanceEcoProperty, isEcoMode(), nullptr);
    state.setProperty ("qqscCurveVariant", "classic-super-selectable", nullptr);
    state.setProperty (monitorLRProperty, getDomainMonitorSelection (qqsc::params::leftRight), nullptr);
    state.setProperty (monitorMSProperty, getDomainMonitorSelection (qqsc::params::midSide), nullptr);
    state.setProperty("qqscLimiterUnityMonitor",isUnityMonitorEnabled(),nullptr);
    state.setProperty("qqscLimiterBankInitialised",limiterBankInitialised.load(std::memory_order_acquire),nullptr);
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
            dualRatioLinkPreferenceInitialised.store (true);
            inputOutputLinkPreferenceInitialised.store (true);
            algorithmPreferenceInitialised.store (true);
            for(auto& initialised:dualAlgorithmPreferenceInitialised)initialised.store(true);
            const bool stateHasDualRatioLink = stateContainsParameter (state, qqsc::params::dualRatioLink);
            const bool stateHasInputOutputLink = stateContainsParameter (state, qqsc::params::inputOutputLink);
            const bool stateHasAlgorithm = stateContainsParameter (state, qqsc::params::algorithmMode);
            const bool stateHasCeilingOs = stateContainsParameter (state, qqsc::params::ceilingOversampling);
            const bool stateHasDetectorMode = stateContainsParameter (state, qqsc::params::detectorMode);
            const bool stateHasStereoLink=stateContainsParameter(state,qqsc::params::limiterStereoLink);
            const bool stateHasDistort = stateContainsParameter(state,qqsc::params::distort);
            const bool stateHasDetectorWindow = stateContainsParameter (state, qqsc::params::detectorWindowMs);
            const bool stateHasTpRecovery = stateContainsParameter (state, qqsc::params::tpRecoveryMode);
            // replaceState adds missing parameter nodes to its shared tree.
            // Record legacy absence BEFORE that happens, for active-bank
            // migration as well as the inactive A/B snapshot defaults.
            const std::array<bool,2> stateHasDualAlgorithms {
                stateContainsParameter(state,"upAlgorithmMode"),stateContainsParameter(state,"downAlgorithmMode") };
            const auto schemaVersion = static_cast<int> (state.getProperty (stateSchemaProperty, 0));
            const bool legacyOversamplingSchema = schemaVersion < oversamplingSchemaVersion;
            const bool stateHasOversampling = stateContainsParameter (state, qqsc::params::oversampling);
            const bool stateHasInputGain = stateContainsParameter (state, qqsc::params::inputGainDb);
            for (const auto& entry : std::array<std::pair<const char*, float>,6> {{{qqsc::params::limiterMode,0}, {qqsc::params::limiterLink,1}, {qqsc::params::ceilingDb,0}, {qqsc::params::limiterOutputDb,0}, {qqsc::params::limiterCalibrationDb,0}, {qqsc::params::truePeakLimiting,0}}})
                if (! stateContainsParameter (state, entry.first))
                {
                    juce::ValueTree node ("PARAM"); node.setProperty ("id", entry.first, nullptr); node.setProperty ("value", entry.second, nullptr); state.addChild (node,-1,nullptr);
                }
            const bool hadLimiterBank=stateContainsParameter(state,qqsc::params::limiterSoundIds[0]);
            std::array<bool,33> missingModeSettings {};
            for(size_t i=0;i<missingModeSettings.size();++i)
                missingModeSettings[i]=!stateContainsParameter(state,qqsc::params::limiterModeIds[i]);
            std::array<float,5> legacySharedMakeup {};
            for(size_t d=0;d<5;++d)
                legacySharedMakeup[d]=float(state.getChildWithProperty("id",qqsc::params::normalModeIds[1+d]).getProperty("value",0.0f));
            for(const auto* id : qqsc::params::limiterSoundIds)
                if(!stateContainsParameter(state,id))
                { auto* p=apvts.getParameter(id); juce::ValueTree node("PARAM"); node.setProperty("id",id,nullptr); node.setProperty("value",p->convertFrom0to1(p->getDefaultValue()),nullptr);state.addChild(node,-1,nullptr); }
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
            restoringDynamicsState.store (true, std::memory_order_release);
            // Before 1.2.9 an unused Limiter bank carried -120 dB Single
            // placeholders, overwritten by first-entry cloning. Replace only
            // those unused placeholders; configured Limiter banks are untouched.
            if (schemaVersion < 23 && !bool(state.getProperty("qqscLimiterBankInitialised",hadLimiterBank)))
                for (auto node : state)
                    for (const auto* id : qqsc::params::limiterThresholdIds)
                        if (node.getProperty("id").toString() == id && float(node.getProperty("value")) <= -120.0f)
                            node.setProperty("value",0.0f,nullptr);
            // Preserve physical dB inside the new Normal range. Limiter nodes
            // keep their +/-120 dB values. This cannot remap DAW-owned automation.
            for (auto node : state)
            {
                for (size_t i = 1; i <= 5; ++i)
                    if (node.getProperty("id").toString() == qqsc::params::normalModeIds[i])
                        node.setProperty("value", juce::jlimit(-qqsc::normalMaximumMakeupDb,
                            qqsc::normalMaximumMakeupDb, float(node.getProperty("value"))), nullptr);
                const auto id = node.getProperty("id").toString();
                if (id == qqsc::params::inputGainDb || id == qqsc::params::outputGainDb)
                    node.setProperty("value", juce::jlimit(-24.0f, 24.0f,
                        float(node.getProperty("value"))), nullptr);
            }
            // Older projects never stored an instance preference. Restore FULL
            // explicitly, including when loading over an existing ECO instance.
            ecoMode.store (bool(state.getProperty(performanceEcoProperty, false)), std::memory_order_release);
            // APVTS stores actual choice indices; 1x/8x/16x -> 1x/4x/8x/16x.
            // Read the old schema before replacement; A/B snapshots migrate separately.
            if(schemaVersion<28)
                for(auto node:state)
                {
                    const auto id=node.getProperty("id").toString();
                    if(id==qqsc::params::ceilingOversampling || id=="limiterOversampling"
                       || (id==qqsc::params::oversampling && !legacyOversamplingSchema))
                        node.setProperty("value",qqsc::params::migrateLegacyOversamplingChoice(
                            juce::roundToInt(float(node.getProperty("value")))),nullptr);
                }
            apvts.replaceState (state);
            if (! stateHasTpRecovery)
                setActualParameterValue(qqsc::params::tpRecoveryMode,float(qqsc::params::tpAuto));
            if (! stateHasAlgorithm)
            {
                // 1.2.3 / dB comparison states retain their fixed-dB law.
                // Earlier saved projects retain the original rational law.
                const bool fixedDb = schemaVersion >= 17
                    || state.getProperty ("qqscCurveVariant").toString() == "fixed-db-finite-threshold";
                setActualParameterValue (qqsc::params::algorithmMode, fixedDb ? 0.0f : 1.0f);
            }
            if (! stateHasDualRatioLink)
                setActualParameterValue (qqsc::params::dualRatioLink, 1.0f);
            if (! stateHasInputOutputLink)
                setActualParameterValue (qqsc::params::inputOutputLink, 0.0f);
            setDomainMonitorSelection (qqsc::params::leftRight, restoredMonitorLR);
            setDomainMonitorSelection (qqsc::params::midSide, restoredMonitorMS);
            setUnityMonitorEnabled(bool(state.getProperty("qqscLimiterUnityMonitor",false)));

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
                    parameter->setValueNotifyingHost (parameter->convertTo0to1 (float(qqsc::params::os8x)));
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

            // Retain the parameter identity while migrating obsolete presets.
            const auto restoredLookahead = apvts.getRawParameterValue (qqsc::params::lookaheadMs)->load();
            const auto snappedLookahead = qqsc::params::snapLookaheadMs (restoredLookahead);
            if (std::abs (restoredLookahead - snappedLookahead) > 0.0001f)
            {
                if (auto* parameter = apvts.getParameter (qqsc::params::lookaheadMs))
                    parameter->setValueNotifyingHost (parameter->convertTo0to1 (snappedLookahead));
            }

            // Missing new parameters use explicit defaults even when an older
            // project is loaded into an already-edited v1.2.0 instance.
            if (! stateContainsParameter (state, qqsc::params::compressionMode))
                setActualParameterValue (qqsc::params::compressionMode, 0.0f);
            if (! stateHasCeilingOs)
                setActualParameterValue (qqsc::params::ceilingOversampling, float(qqsc::params::ceiling8x));
            if (! stateHasDetectorWindow)
                setActualParameterValue (qqsc::params::detectorWindowMs, 100.0f);
            if (! stateHasDetectorMode)
                setActualParameterValue (qqsc::params::detectorMode, 0.0f);
            for (size_t d = 0; d < 5; ++d)
            {
                if (! stateContainsParameter (state, qqsc::params::rangeIds[d])) setActualParameterValue (qqsc::params::rangeIds[d], qqsc::params::rangeOffDb);
                if (! stateContainsParameter (state, qqsc::params::upThresholdIds[d])) setActualParameterValue (qqsc::params::upThresholdIds[d], qqsc::params::thresholdOffDb);
                if (! stateContainsParameter (state, qqsc::params::downThresholdIds[d])) setActualParameterValue (qqsc::params::downThresholdIds[d], 0.0f);
                if (! stateContainsParameter (state, qqsc::params::upRatioIds[d])) setActualParameterValue (qqsc::params::upRatioIds[d], 1.0f);
                if (! stateContainsParameter (state, qqsc::params::upEnabledIds[d])) setActualParameterValue (qqsc::params::upEnabledIds[d], 1.0f);
                if (! stateContainsParameter (state, qqsc::params::downRatioIds[d])) setActualParameterValue (qqsc::params::downRatioIds[d], 1.0f);
                if (! stateContainsParameter (state, qqsc::params::downEnabledIds[d])) setActualParameterValue (qqsc::params::downEnabledIds[d], 1.0f);
            }
            for(size_t i=0;i<2;++i)
                if(!stateHasDualAlgorithms[i])setActualParameterValue(i==0 ? "upAlgorithmMode" : "downAlgorithmMode",
                    apvts.getRawParameterValue(qqsc::params::algorithmMode)->load());
            for(size_t i=0;i<missingModeSettings.size();++i)
                if(missingModeSettings[i])
                {
                    auto* parameter = apvts.getParameter(qqsc::params::limiterModeIds[i]);
                    // Only pre-independent-bank Limiter sessions need the old
                    // shared-setting migration. A project with no Limiter bank
                    // gets the Limiter parameter's own default, not Normal.
                    const auto value = hadLimiterBank && i>=1 && i<=5 ? legacySharedMakeup[i-1]
                        : hadLimiterBank ? apvts.getRawParameterValue(i<31 || missingModeSettings[11]
                            ? qqsc::params::normalModeIds[i] : "limiterAlgorithmMode")->load()
                        : parameter->convertFrom0to1(parameter->getDefaultValue());
                    setActualParameterValue(qqsc::params::limiterModeIds[i],value);
                }
            if(!stateHasStereoLink)
            {
                ParameterSnapshot legacy;
                for(size_t i=0;i<legacy.limiterSound.size();++i)
                    legacy.limiterSound[i]=apvts.getRawParameterValue(qqsc::params::limiterSoundIds[i])->load();
                for(size_t i=0;i<legacy.limiterSettings.size();++i)
                    legacy.limiterSettings[i]=apvts.getRawParameterValue(qqsc::params::limiterModeIds[i])->load();
                migrateLimiterStereoSnapshot(legacy);
                for(size_t i=0;i<legacy.limiterSound.size();++i)
                    setActualParameterValue(qqsc::params::limiterSoundIds[i],legacy.limiterSound[i]);
                for(size_t i=0;i<legacy.limiterSettings.size();++i)
                    setActualParameterValue(qqsc::params::limiterModeIds[i],legacy.limiterSettings[i]);
                setActualParameterValue(qqsc::params::limiterStereoLink,legacy.limiterStereoLink);
            }
            setActualParameterValue("limiterLookaheadMs",qqsc::params::snapLookaheadMs(
                apvts.getRawParameterValue("limiterLookaheadMs")->load()));
            limiterBankInitialised.store(bool(state.getProperty("qqscLimiterBankInitialised",hadLimiterBank)),std::memory_order_release);
            const auto restoredDistort = stateHasDistort
                ? apvts.getRawParameterValue(qqsc::params::distort)->load()
                : qqsc::params::distortForWindowMs(readSoundParameter(qqsc::params::lookaheadMs),
                    apvts.getRawParameterValue(qqsc::params::detectorWindowMs)->load());
            setActualParameterValue(qqsc::params::distort,restoredDistort);
            distortForAudio.store(restoredDistort,std::memory_order_release);
            rebuildBoundaryPairs();
            restoringDynamicsState.store (false, std::memory_order_release);
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
