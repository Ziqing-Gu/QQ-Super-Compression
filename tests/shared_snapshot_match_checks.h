// Test-only existing friend: validate the A/B audio projection and the new
// common MATCH target without changing production access restrictions.
struct QQSCReviewCheck
{
    static void sharedSnapshotMatchChecks()
    {
        QQSuperCompressionAudioProcessor p;p.enterLimiterMode();
        const auto set=[&](const char* id,float v){auto* a=p.apvts.getParameter(id);if(!a)throw std::runtime_error(id);a->setValueNotifyingHost(a->convertTo0to1(v));};
        set("limiterLink",0);set("limiterRatioL",5);set("limiterRatioR",20);
        set("limiterUpRatioL",.7f);set("limiterUpRatioR",1);
        set("limiterDownRatioL",9);set("limiterDownRatioR",1);
        set("limiterMakeupGainLDb",3);set("limiterMakeupGainRDb",-8);
        set("limiterMixL",72);set("limiterMixR",6);
        set("limiterThresholdLDb",-12);set("limiterThresholdRDb",-30);
        set("limiterUpThresholdLDb",-60);set("limiterUpThresholdRDb",-90);
        set("limiterDownThresholdLDb",-10);set("limiterDownThresholdRDb",-25);
        set("limiterUpEnabledL",0);set("limiterUpEnabledR",1);
        auto transfer=QQSuperCompressionAudioProcessor::makeABTransfer(p.captureCurrentSnapshot());
        for(const auto* values:{&transfer.ratio,&transfer.upRatio,&transfer.downRatio,&transfer.makeup,&transfer.mix,&transfer.lower,&transfer.upper})
            if(std::abs((*values)[1]-(*values)[2])>.000001f)throw std::runtime_error("A/B transfer still uses hidden right controls");
        const auto fixture=[&](float target){
            p.loudnessMatch.prepare(48000);p.unityMixMatch.prepare(48000);
            const float gain=juce::Decibels::decibelsToGain(-target);
            for(int i=0;i<48000;++i){const float dry=.1f*std::sin(float(i)*.13f),wet=dry*gain;
                p.loudnessMatch.processSample(dry,dry,wet,wet,wet,wet,0,0,0,0);
                p.unityMixMatch.processSample(0,0,wet,wet,wet,wet,0,0,0,0);}
            p.resetMatchOnNextPlaybackBlock.store(false);p.matchReady.store(true);
            p.matchPublishedGeneration.store(p.matchGeneration.load());
            p.matchSTValid.store(true);p.matchLValid.store(false);p.matchRValid.store(false);
            p.matchSTDb.store(target);
        };
        fixture(6);
        if(!p.applyMatchForCurrentMode() || std::abs(p.readSoundParameter("makeupGainLDb")-6)>.001f
            || std::abs(p.readSoundParameter("makeupGainRDb")-6)>.001f)
            throw std::runtime_error("Common MATCH absolute stereo target");
        p.setUnityMonitorEnabled(true);fixture(8);
        if(!p.applyMatchForCurrentMode() || std::abs(p.readSoundParameter("makeupGainLDb")-8)>.001f
            || std::abs(p.readSoundParameter("makeupGainRDb")-8)>.001f)
            throw std::runtime_error("Common unity MATCH delta applied twice or per channel");
        if(!p.applyMatchForCurrentMode() || std::abs(p.readSoundParameter("makeupGainLDb")-8)>.001f)
            throw std::runtime_error("Repeated unity MATCH added old correction");
        std::cout<<"PASS: A/B gain transfer uses shared controls; MATCH uses one stereo target and repeated unity Match is idempotent.\n";
    }
#include "cumulative_match_checks.inc"
};
