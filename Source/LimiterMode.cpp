#include "PluginProcessor.h"
#include "LimiterModeContinuity.h"

void QQSuperCompressionAudioProcessor::setUnityMonitorEnabled(bool enabled)
{
    if(unityMonitor.exchange(enabled,std::memory_order_relaxed)==enabled)return;
    // Monitoring is downstream of the shared Match taps; retain their capture.
    resetTruePeakHold();
    updateHostDisplay(juce::AudioProcessor::ChangeDetails().withNonParameterStateChanged(true));
}

float QQSuperCompressionAudioProcessor::getUnityMonitorCeilingDb() const noexcept
{
    const auto c=apvts.getRawParameterValue(qqsc::params::ceilingDb)->load();
    return c-(isUnityMonitorActive() ? apvts.getRawParameterValue(qqsc::params::limiterOutputDb)->load() : 0.f);
}

const char* QQSuperCompressionAudioProcessor::soundParameterID (const char* id) const noexcept
{
    if(isMasteringMode())
    {
        const auto& sound = isClipperMode() ? qqsc::params::clipperSoundIds : qqsc::params::limiterSoundIds;
        const auto& settings = isClipperMode() ? qqsc::params::clipperModeIds : qqsc::params::limiterModeIds;
        for(size_t i=0;i<qqsc::params::normalSoundIds.size();++i)
            if(std::strcmp(id,qqsc::params::normalSoundIds[i])==0) return sound[i];
        for(size_t i=0;i<qqsc::params::normalModeIds.size();++i)
            if(std::strcmp(id,qqsc::params::normalModeIds[i])==0) return settings[i];
        if(std::strcmp(id,qqsc::params::outputGainDb)==0) return isClipperMode() ? "clipperOutputDb" : qqsc::params::limiterOutputDb;
        if(isClipperMode())
        {
            if(std::strcmp(id,qqsc::params::limiterOutputDb)==0) return "clipperOutputDb";
            if(std::strcmp(id,qqsc::params::limiterLink)==0) return "clipperLink";
            if(std::strcmp(id,qqsc::params::limiterStereoLink)==0) return "clipperStereoLink";
        }
    }
    return id;
}
float QQSuperCompressionAudioProcessor::readSoundParameter (const char* id) const noexcept
{
    // 1.4.19: Input is retired, including old projects and host automation.
    // Preserve parameter IDs/order, but never let legacy values reach DSP/UI.
    if(std::strcmp(id,"inputGainDb")==0 || std::strcmp(id,"limiterInputGainDb")==0
        || std::strcmp(id,"clipperInputGainDb")==0 || std::strcmp(id,"inputOutputLink")==0)
        return 0.f;
    if(isClipperMode() && std::strcmp(id,qqsc::params::compressionMode)==0) return 0.f;
    if (isMasteringMode() && std::strcmp(id,qqsc::params::processingMode)==0)
        return float(qqsc::params::leftRight);
    if (isMasteringMode())
    {
        // Retain legacy host IDs for project compatibility; Range no longer
        // participates in Limiter processing, automation or display.
        for (const auto* rangeID : qqsc::params::rangeIds)
            if (std::strcmp(id,rangeID)==0) return qqsc::params::rangeOffDb;
        // One shared control set drives both independent detectors. Keep the
        // legacy R parameter IDs/state, but do not let hidden R values sound.
        constexpr const char* right[] = {"ratioR","upRatioR","downRatioR",
            "makeupGainRDb","mixR","upEnabledR","downEnabledR",
            "thresholdRDb","rangeRDb","upThresholdRDb","downThresholdRDb"};
        constexpr const char* shared[] = {"ratioL","upRatioL","downRatioL",
            "makeupGainLDb","mixL","upEnabledL","downEnabledL",
            "thresholdLDb","rangeLDb","upThresholdLDb","downThresholdLDb"};
        for(size_t i=0;i<std::size(right);++i) if(std::strcmp(id,right[i])==0) {id=shared[i];break;}
    }
    if (std::strcmp(id,qqsc::params::distort)==0)
        return distortForAudio.load(std::memory_order_acquire);
    if (std::strcmp(id,qqsc::params::detectorWindowMs)==0)
        return qqsc::params::windowMsForDistort(readSoundParameter(qqsc::params::lookaheadMs),
                                              distortForAudio.load(std::memory_order_acquire));
    const auto* mappedID = soundParameterID (id);
    if (std::strcmp (mappedID, "limiterCompressionMode") == 0)
        return static_cast<float> (limiterCompressionModeForAudio.load (std::memory_order_acquire));
    if(std::strcmp(mappedID,"clipperCompressionMode")==0)
        return float(clipperCompressionModeForAudio.load(std::memory_order_acquire));
    return apvts.getRawParameterValue(mappedID)->load();
}

void QQSuperCompressionAudioProcessor::setCompressionModeFromEditor (int mode)
{
    if(isClipperMode()) return;
    const int nextMode = juce::jlimit (0, 1, mode);
    if (readSoundParameter (qqsc::params::compressionMode) == static_cast<float> (nextMode))
        return;

    // Flush prior edits into their original undo transaction first. Both this
    // method and the editor's button callback run on the message thread.
    timerCallback();
    (void) apvts.copyState();
    undoManager.beginNewTransaction ("Single / Dual Compression");

    std::array<juce::RangedAudioParameter*, 10> companions {};
    size_t numCompanions = 0;
    if (isMasteringMode())
    {
        const bool dual = nextMode == 1;
        const int sourceBank = activeBoundaryBank() + (dual ? 0 : 1);
        const int destinationBank = activeBoundaryBank() + (dual ? 1 : 0);
        for (int d = 0; d < 5; ++d)
        {
            const auto desired = qqsc::continueLimiterModeBoundaries (dual,
                { getBoundaryForBankDb (sourceBank, false, d), getBoundaryForBankDb (sourceBank, true, d) },
                { getBoundaryForBankDb (destinationBank, false, d), getBoundaryForBankDb (destinationBank, true, d) });
            for (bool upper : { false, true })
            {
                const auto* id = qqsc::params::boundaryBankIds (destinationBank, upper)[static_cast<size_t> (d)];
                const auto value = upper ? desired.upperDb : desired.lowerDb;
                if (std::abs (apvts.getRawParameterValue (id)->load() - value) > 0.0001f)
                {
                    auto* parameter = apvts.getParameter (id);
                    companions[numCompanions++] = parameter;
                    parameter->beginChangeGesture();
                }
            }
        }
    }

    auto* parameter = apvts.getParameter (soundParameterID (qqsc::params::compressionMode));
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (nextMode)));
    // parameterChanged has transferred the canonical threshold before it
    // publishes the new mode. Flush now so attachments cannot bind stale raw
    // values and Ctrl+Z includes every changed companion in this transaction.
    timerCallback();
    parameter->endChangeGesture();
    for (size_t i = 0; i < numCompanions; ++i)
        companions[i]->endChangeGesture();
    (void) apvts.copyState();
}

void QQSuperCompressionAudioProcessor::setProcessorModeFromEditor(int mode)
{
    mode=juce::jlimit(0,2,mode);
    if(mode==getProcessorMode()) return;
    timerCallback();
    (void)apvts.copyState();
    const auto from=captureCurrentSnapshot();
    if(mode==2 && !limiterBankInitialised.load(std::memory_order_acquire))
    {
        struct RememberInitialisation final : juce::UndoableAction
        {
            explicit RememberInitialisation(std::atomic<bool>& value) : flag(value), before(value.load()) {}
            bool perform() override { flag.store(true,std::memory_order_release);return true; }
            bool undo() override { flag.store(before,std::memory_order_release);return true; }
            std::atomic<bool>& flag;const bool before;
        };
        undoManager.perform(new RememberInitialisation(limiterBankInitialised));
    }
    auto to=from;to.limiterMode=mode==2;to.clipperMode=mode==1;
    queueABTransfer(from,to);
    // Suppress companion parameter propagation during a bank-only switch.
    restoringDynamicsState.store(true,std::memory_order_release);
    setActualParameterValue("clipperMode",mode==1 ? 1.f : 0.f);
    setActualParameterValue(qqsc::params::limiterMode,mode==2 ? 1.f : 0.f);
    restoringDynamicsState.store(false,std::memory_order_release);
    if(mode==2)limiterBankInitialised.store(true,std::memory_order_release);
    notifyHostProcessingLatency();
}

void QQSuperCompressionAudioProcessor::enterLimiterMode() { setProcessorModeFromEditor(2); }
void QQSuperCompressionAudioProcessor::leaveLimiterMode() { setProcessorModeFromEditor(0); }

float QQSuperCompressionAudioProcessor::effectiveSingleRatio (size_t domain) const noexcept
{
    return qqsc::params::limiterRatio (readSoundParameter (qqsc::params::ratioIds[domain]), isMasteringMode());
}

float QQSuperCompressionAudioProcessor::getActiveOutputGainDb() const noexcept
{
    if(isClipperMode()) return apvts.getRawParameterValue("clipperOutputDb")->load();
    return isLimiterMode()
        ? apvts.getRawParameterValue (qqsc::params::limiterOutputDb)->load()
            + (isCeilingEnabled() ? apvts.getRawParameterValue (qqsc::params::ceilingDb)->load()
                + apvts.getRawParameterValue (qqsc::params::limiterCalibrationDb)->load() : 0.f)
        : apvts.getRawParameterValue (qqsc::params::outputGainDb)->load();
}

float QQSuperCompressionAudioProcessor::getLimiterReferencePeakDb (float shift, bool includeMix) const noexcept
{
    const auto snapshot = captureCurrentSnapshot();
    auto s = activeSnapshot(snapshot);
    auto t = makeABTransfer (snapshot);
    std::array<float,5> bound {};
    for (size_t d=0; d<5; ++d)
    {
        // Reference carrier: post-Input peak <= 1, matched/unfiltered internal
        // key. No signal-dependent gain is introduced by this calculation.
        const auto floor = qqsc::params::thresholdMinimumDb;
        if (shift != 0.0f && t.dual && s.downEnabled[d])
            t.upper[d] = qqsc::params::thresholdLinear (juce::jlimit (juce::jmax (floor,juce::jmin(qqsc::params::thresholdMaximumDb,getBoundaryForDomainDb(true,false,int(d))+0.01f)), qqsc::params::thresholdMaximumDb, getBoundaryForDomainDb(true,true,int(d))+shift));
        else if (shift != 0.0f && !t.dual && t.ratio[d]>1.0f)
        {
            t.lower[d] = qqsc::params::thresholdLinear (juce::jlimit (floor, qqsc::params::thresholdMaximumDb, getBoundaryForDomainDb(false,false,int(d))+shift));
        }
        const auto g = t.dual
            ? qqsc::StaticCompressionEngine::dualGainForLevel (1.0f,t.upRatio[d],t.downRatio[d],t.lower[d],t.upper[d],t.upAlgorithm,t.downAlgorithm)
            : qqsc::StaticCompressionEngine::singleGainForLevel (1.0f,t.ratio[d],t.lower[d],t.upper[d],t.algorithm);
        // Evaluate the completed curve, wet Makeup and linear dry/wet mix.
        // At shift == 0 use the actual boundaries, including equal gates.
        bound[d] = includeMix ? (1-t.mix[d]) + t.mix[d]*t.makeup[d]*g
                              : t.makeup[d]*g;
    }
    // LR uses the larger channel bound. MS uses the conservative sum of
    // independent M/S bounds before decoding; it is not a tight stereo maximum.
    const auto peak = t.mode == qqsc::params::midSide ? bound[3]+bound[4]
        : t.mode == qqsc::params::leftRight ? juce::jmax (bound[1],bound[2]) : bound[0];
    return juce::Decibels::gainToDecibels (juce::jmax (peak,1.0e-18f),-360.0f);
}

void QQSuperCompressionAudioProcessor::resetTruePeakHold() noexcept
{
    // Clear the readout immediately, even while the host has stopped calling
    // processBlock. Only the audio thread changes the running accumulator.
    truePeakResetRequested.store(true,std::memory_order_release);
    meterState.truePeakHoldDb.store(-120.0f,std::memory_order_relaxed);
}

void QQSuperCompressionAudioProcessor::updateTruePeakMeter (int numSamples)
{
    // An independent 4x FIR interpolation meter. Its filter is outside the
    // audio path; it adds no audible latency and never controls audio gain.
    auto block=juce::dsp::AudioBlock<float>(truePeakBuffer).getSubBlock(0,size_t(numSamples));
    auto up=truePeakOversampler.processSamplesUp(block);
    float peak=0;
    for(size_t c=0;c<2;++c)
    {
        const auto* samples=up.getChannelPointer(c);
        for(size_t i=0;i<up.getNumSamples();++i) peak=juce::jmax(peak,std::abs(samples[i]));
        peak=juce::jmax(peak,truePeakBuffer.getMagnitude(int(c),0,numSamples));
    }
    updateTruePeakHold(juce::Decibels::gainToDecibels(peak,-120.0f),numSamples);
}

void QQSuperCompressionAudioProcessor::updateTruePeakHold (float db, int numSamples)
{
    if (truePeakResetRequested.exchange(false,std::memory_order_acquire))
    {
        truePeakHold=-120.0f;
        truePeakRemaining=0;
    }
    truePeakRemaining-=numSamples;
    if(db>=truePeakHold || truePeakRemaining<=0)
    { truePeakHold=db; truePeakRemaining=juce::jmax<int64_t>(1,int64_t(std::llround(currentSampleRate*20))); }
    meterState.truePeakHoldDb.store(truePeakHold,std::memory_order_relaxed);
}
