#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include <array>
#include <vector>

namespace qqsc
{
// Stereo-linked lookahead protection. The fixed safety window is never
// shortened by the adaptive timing: each minimum contributing to the moving
// average still contains the delayed sample. Timing only adds attenuation.
class SmoothPeakCap
{
public:
    void prepare(double rate,int lookSamples)
    {
        sampleRate=rate;
        silenceSettlingSamples=int64_t(std::ceil(rate*.12));
        look=juce::jmax(1,lookSamples);
        window=look;
        nodes.assign(size_t(window+2),{});
        slowWindow=look+int(std::ceil(rate*.030));
        slowNodes.assign(size_t(slowWindow+2),{});slowHead=slowTail=0;
        audio.assign(size_t(look+1),{});
        gains.assign(size_t(look+1),1.0);
        head=tail=audioIndex=gainIndex=0;index=0;sum=double(look+1);released=1;
        displayGain=1.0;
        inputPeakForTelemetry=0.0;silentSamples=0;
        remainingHold=0;
        repeatedRemaining=quietSamples=0;trough=crest=1;rising=false;
        for(auto& p:periods)p={};
        energy={};
        energyCoefficient=1.0-std::exp(-1.0/(rate*.010));
        periodCoefficient=1.0-std::exp(-1.0/(rate*.030));
    }
    void reset() noexcept
    {
        std::fill(nodes.begin(),nodes.end(),Node{});
        std::fill(slowNodes.begin(),slowNodes.end(),Node{});
        std::fill(audio.begin(),audio.end(),std::array<float,2>{});
        std::fill(gains.begin(),gains.end(),1.0);
        head=tail=audioIndex=gainIndex=0;
        slowHead=slowTail=0;
        index=0;sum=double(look+1);released=1.0;displayGain=1.0;
        inputPeakForTelemetry=0.0;silentSamples=0;remainingHold=0;repeatedRemaining=quietSamples=0;
        trough=crest=1.0;rising=false;energy={};
        for(auto& p:periods)p={};
    }
    // Suspend only after the safety windows drain and recovery is within
    // 1e-10 of unity. Keep this state on wake; resetting changes the next hit.
    bool isSilentAndSettled() const noexcept
    {
        return silentSamples>=silenceSettlingSamples
            && repeatedRemaining==0 && energy[0]<=1.e-20 && energy[1]<=1.e-20
            && std::abs(released-1.0)<=1.e-10
            && std::abs(sum/double(gains.size())-1.0)<=1.e-10;
    }
    // The caller has already observed quiet input/output for at least a second.
    // Ignore exact-zero counting, but never freeze an unfinished gain release.
    bool isQuietAndSettled(float floor) const noexcept
    {
        const double energyFloor=double(floor)*floor;
        return repeatedRemaining==0
            && energy[0]<=energyFloor && energy[1]<=energyFloor
            && std::abs(released-1.0)<=1.e-10
            && std::abs(sum/double(gains.size())-1.0)<=1.e-10;
    }
    bool useConservativeTiming(int recoveryMode) const noexcept
    {
        recoveryMode=juce::jlimit(int(qqsc::params::tpTight),int(qqsc::params::tpSmooth),recoveryMode);
        if(recoveryMode==qqsc::params::tpTight) return false;
        if(recoveryMode==qqsc::params::tpSmooth) return true;
        return repeatedRemaining>0;
    }
    float gainForDisplay() const noexcept { return static_cast<float> (displayGain); }
    float inputPeakForDisplay() const noexcept { return static_cast<float> (inputPeakForTelemetry); }
    std::array<float,2> process(float l,float r,float ceiling,float detectedPeak=0,bool repeatedHint=false,float triggerCeiling=-1.0f,int recoveryMode=qqsc::params::tpAuto) noexcept
    {
        const std::array<float,2> input {l,r};
        silentSamples=(l==0.0f && r==0.0f && detectedPeak==0.0f)
            ? juce::jmin(silentSamples+1,silenceSettlingSamples):0;
        double halfPeriod=.0005;
        for(size_t ch=0;ch<2;++ch)
        {
            energy[ch]+=energyCoefficient*(double(input[ch])*input[ch]-energy[ch]);
            auto& p=periods[ch];
            // Require a real sign change; zero-filled silence is not a cycle.
            const int sign=input[ch]>1.e-10f ? 1 : input[ch]<-1.e-10f ? -1 : 0;
            if(sign!=0 && p.sign!=0 && sign!=p.sign)
            {
                if(p.lastCross>=0)
                    p.target=juce::jlimit(.00005,.030,double(index-p.lastCross)/rate());
                p.lastCross=index;
            }
            if(sign!=0)p.sign=sign;
            // Missing a crossing must not leave a stale fast estimate when
            // a low-frequency half-cycle arrives after high-frequency audio.
            const double age=p.lastCross<0 ? .025 : double(index-p.lastCross)/rate();
            const double desired=juce::jlimit(.00005,.030,juce::jmax(p.target,age));
            p.estimate=desired>p.estimate ? desired : p.estimate+periodCoefficient*(desired-p.estimate);
        }
        const double largestEnergy=juce::jmax(energy[0],energy[1]);
        for(size_t ch=0;ch<2;++ch)
            if(energy[ch]>=largestEnergy*.0001)
                halfPeriod=juce::jmax(halfPeriod,periods[ch].estimate);
        const double peak=juce::jmax(double(detectedPeak),juce::jmax(std::abs(double(l)),std::abs(double(r))));
        inputPeakForTelemetry=peak;
        const double trigger=triggerCeiling>0.0f ? double(triggerCeiling) : double(ceiling);
        const double target=peak>trigger ? double(ceiling)/peak : 1.0;
        // Each ring cursor advances by one; conditional wrap is exact and
        // avoids runtime integer division at every oversampled sample.
        while(head!=tail && nodes[head].index<index-window) { if(++head==nodes.size()) head=0; }
        while(head!=tail)
        {
            const auto back=(tail==0 ? nodes.size()-1 : tail-1);
            if(nodes[back].value<target)break;
            tail=back;
        }
        nodes[tail]={index++,target};if(++tail==nodes.size()) tail=0;
        const auto now=index-1;
        while(slowHead!=slowTail && slowNodes[slowHead].index<now-slowWindow) { if(++slowHead==slowNodes.size()) slowHead=0; }
        while(slowHead!=slowTail)
        {
            const auto back=(slowTail==0 ? slowNodes.size()-1 : slowTail-1);
            if(slowNodes[back].value<target)break;
            slowTail=back;
        }
        slowNodes[slowTail]={now,target};if(++slowTail==slowNodes.size()) slowTail=0;
        const auto minimum=nodes[head].value;
        // Repeated deep envelope reversals are modulation, not isolated
        // overshoots. Keep the conservative timing while they persist, so
        // a fast carrier does not make its slow AM envelope pump the gain.
        if(repeatedRemaining>0)--repeatedRemaining;
        quietSamples=minimum>=.999 ? quietSamples+1 : 0;
        if(quietSamples>int(rate()*.040)) {trough=crest=1;rising=false;quietSamples=int(rate()*.040)+1;}
        if(!rising)
        {
            trough=juce::jmin(trough,minimum);
            if(minimum>trough*1.12) {rising=true;crest=minimum;}
        }
        else
        {
            crest=juce::jmax(crest,minimum);
            if(minimum<crest/1.12 && minimum<.999)
            { repeatedRemaining=int(rate()*.080);rising=false;trough=minimum; }
        }
        recoveryMode=juce::jlimit(int(qqsc::params::tpTight),int(qqsc::params::tpSmooth),recoveryMode);
        const bool autoRepeated=repeatedRemaining>0 || repeatedHint;
        const bool repeated=recoveryMode==qqsc::params::tpSmooth
            || (recoveryMode==qqsc::params::tpAuto && autoRepeated);
        const double controlledMinimum=repeated ? slowNodes[slowHead].value : minimum;
        int holdSamples=0;
        double releaseSeconds=.060;
        if(recoveryMode==qqsc::params::tpTight)
        {
            // Loudness-first recovery. The lookahead moving minimum remains the
            // hard safety constraint; only post-peak recovery is accelerated.
            holdSamples=0;
            releaseSeconds=juce::jlimit(.005,.025,halfPeriod);
        }
        else if(recoveryMode==qqsc::params::tpSmooth)
        {
            // Smooth deliberately retains the longer safety minimum and a slow
            // release. It trades short-term loudness for a calmer gain envelope.
            holdSamples=0;
            releaseSeconds=juce::jlimit(.060,.180,6.0*halfPeriod);
        }
        else
        {
            // AUTO is the verified 1.2.17 adaptive timing, unchanged.
            holdSamples=autoRepeated ? 0 : int(std::ceil(rate()*juce::jlimit(.0015,.030,1.1*halfPeriod-double(look)/rate()+.0015)));
            releaseSeconds=autoRepeated ? .060 : juce::jlimit(.015,.075,2.5*halfPeriod);
        }
        // Backward-Euler coefficient avoids a per-sample transcendental and
        // remains a convex, stable move toward the current safety minimum.
        const double releaseCoefficient=1.0/(1.0+rate()*releaseSeconds);
        if(controlledMinimum<=released)
        {
            released=controlledMinimum;
            remainingHold=holdSamples;
        }
        else if(remainingHold>0) --remainingHold;
        else released+=releaseCoefficient*(controlledMinimum-released);
        sum+=released-gains[gainIndex];gains[gainIndex]=released;
        if(++gainIndex==gains.size()) gainIndex=0;
        audio[audioIndex]={l,r};if(++audioIndex==audio.size()) audioIndex=0;
        const auto gain=juce::jlimit(0.0,1.0,sum/double(gains.size()));
        displayGain=gain;
        return {float(audio[audioIndex][0]*gain),float(audio[audioIndex][1]*gain)};
    }
private:
    double rate() const noexcept { return sampleRate; }
    struct Period { int sign=0;int64_t lastCross=-1;double target=.025,estimate=.025; };
    std::array<Period,2> periods;
    std::array<double,2> energy {};
    struct Node { int64_t index=0;double value=1; };
    std::vector<Node> nodes;
    std::vector<Node> slowNodes;
    std::vector<std::array<float,2>> audio;
    std::vector<double> gains;
    size_t head=0,tail=0,audioIndex=0,gainIndex=0;
    int look=1,window=1;int64_t index=0;
    int slowWindow=1;
    size_t slowHead=0,slowTail=0;
    double sum=0,released=1,displayGain=1,inputPeakForTelemetry=0,sampleRate=48000,energyCoefficient=0,periodCoefficient=0;
    int64_t silentSamples=0,silenceSettlingSamples=1;
    int remainingHold=0;
    int repeatedRemaining=0,quietSamples=0;
    double trough=1,crest=1;
    bool rising=false;
};

// Independent protection histories; coupling only adds attenuation after each
// channel has computed its own safe gain. At zero no detector or recovery
// state from one channel can affect the other. Existing TP timing is retained.
class StereoPeakCap
{
public:
    void prepare(double rate,int look) { for(auto& c:channels)c.prepare(rate,look); }
    void reset() noexcept { for(auto& c:channels)c.reset();gain={1,1}; }
    bool isSilentAndSettled() const noexcept {return channels[0].isSilentAndSettled() && channels[1].isSilentAndSettled();}
    bool isQuietAndSettled(float floor) const noexcept {return channels[0].isQuietAndSettled(floor) && channels[1].isQuietAndSettled(floor);}
    uint8_t conservativeMask(int recovery) const noexcept
    {return uint8_t((channels[0].useConservativeTiming(recovery)?1:0)|(channels[1].useConservativeTiming(recovery)?2:0));}
    float gainForDisplay() const noexcept {return juce::jmin(gain[0],gain[1]);}
    float gainForDisplay(int channel) const noexcept {return gain[size_t(channel)];}
    std::array<float,2> process(float l,float r,float ceiling,float link,
        std::array<float,2> detected={},uint8_t hint=0,float trigger=-1,int recovery=params::tpAuto) noexcept
    {
        auto a=channels[0].process(l,0,ceiling,detected[0],(hint&1)!=0,trigger,recovery);
        auto b=channels[1].process(r,0,ceiling,detected[1],(hint&2)!=0,trigger,recovery);
        const float gl=channels[0].gainForDisplay(),gr=channels[1].gainForDisplay();
        gain={gl,gr};params::coupleLimiterGains(gain[0],gain[1],link);
        return {a[0]*(gl>0 ? gain[0]/gl : 0),b[0]*(gr>0 ? gain[1]/gr : 0)};
    }
private:
    std::array<SmoothPeakCap,2> channels;
    std::array<float,2> gain {1,1};
};

class OutputCeiling
{
public:
    // Ceiling uses the same 1x/4x/8x/16x audio rate as the dynamics core.
    // All filters are prepared up-front; changing quality may reset state/PDC
    // but never allocates an oversampler on the audio thread.
    void prepare(double rate,bool tp,float ceilingDb,int recovery=qqsc::params::tpAuto,
                 int osChoice=qqsc::params::ceiling8x,int maximumBlockSize=16384)
    {
        sampleRate=juce::jmax(1.0,rate);
        look=juce::jmax(1,int(std::ceil(sampleRate*.003)));
        maxBlock=juce::jmax(1,maximumBlockSize);

        oversampling4.initProcessing(1);
        oversampling8.initProcessing(1); // Standalone path processes one host sample at a time.
        oversampling16.initProcessing(1);
        oversampling4.reset();
        oversampling8.reset();
        oversampling16.reset();
        filterLatency4=juce::roundToInt(oversampling4.getLatencyInSamples());
        filterLatency8=juce::roundToInt(oversampling8.getLatencyInSamples());
        filterLatency16=juce::roundToInt(oversampling16.getLatencyInSamples());

        // The residual reconstruction remains a fixed 16x read-only guard.
        // This preserves the verified 1.2.28 TP target semantics for 8x while
        // giving the 4x and 16x audio paths the same final safety check.
        analysis.initProcessing(256);analysis.reset();
        juce::AudioBuffer<float> impulse(2,256);impulse.clear();impulse.setSample(0,0,1);
        auto measured=analysis.processSamplesUp(juce::dsp::AudioBlock<float>(impulse));
        int maximum=0;for(int i=1;i<int(measured.getNumSamples());++i)
            if(std::abs(measured.getSample(0,i))>std::abs(measured.getSample(0,maximum)))maximum=i;
        analysisDelay=juce::roundToInt(double(maximum)/16.0);analysis.reset();

        truePeak1.prepare(sampleRate,look);
        truePeak4.prepare(sampleRate*4.0,look*4);
        truePeak8.prepare(sampleRate*8.0,look*8);
        truePeak16.prepare(sampleRate*16.0,look*16);
        post.prepare(sampleRate,look);

        hardOversampledAlignment.assign(size_t(look*16+2),{});
        analysisAlignment.assign(size_t(analysisDelay+1),{});
        hardPostAlignment.assign(size_t(look+1),{});
        buffer.setSize(2,1);

        sharedCeiling.assign(size_t(maxBlock),1.0f);
        sharedBlend.assign(size_t(maxBlock),0.0f);
        sharedHardGain.assign(size_t(maxBlock),1.0f);
        sharedTpGain.assign(size_t(maxBlock),1.0f);
        sharedTruePeak.assign(size_t(maxBlock),0.0f);
        sharedSamplePeak.assign(size_t(maxBlock),0.0f);
        sharedConservative.assign(size_t(maxBlock),0u);

        linkSmoother.reset(sampleRate,.010);linkSmoother.setCurrentAndTargetValue(stereoLink);
        sharedLink.assign(size_t(maxBlock),stereoLink);
        sharedHardLR.assign(size_t(maxBlock),{1,1});sharedTpLR.assign(size_t(maxBlock),{1,1});
        truePeakBlend.reset(sampleRate,.010);
        ceiling.reset(sampleRate,.010);
        selectedChoice=juce::jlimit(0,3,osChoice);
        recoveryMode=juce::jlimit(int(qqsc::params::tpTight),int(qqsc::params::tpSmooth),recovery);
        resetForLimiter(tp,ceilingDb,recovery);
    }

    // Source-compatibility overload for legacy regression harnesses.
    void prepare(double rate,bool /*enabled*/,bool tp,float ceilingDb,int recovery=qqsc::params::tpAuto)
    {
        prepare(rate,tp,ceilingDb,recovery,qqsc::params::ceiling8x,16384);
    }

    void selectOversamplingChoice(int osChoice,bool tp,float ceilingDb,int recovery=qqsc::params::tpAuto) noexcept
    {
        osChoice=juce::jlimit(0,3,osChoice);
        if(selectedChoice==osChoice)
        {
            set(tp,ceilingDb,recovery);
            return;
        }
        selectedChoice=osChoice;
        resetForLimiter(tp,ceilingDb,recovery);
    }

    void setStereoLink(float amount) noexcept { stereoLink=juce::jlimit(0.0f,1.0f,amount);linkSmoother.setTargetValue(stereoLink); }
    int oversamplingChoice() const noexcept { return selectedChoice; }
    int oversamplingFactor() const noexcept { return qqsc::params::ceilingOversamplingFactorForChoiceIndex(selectedChoice); }
    int filterLatencySamples() const noexcept
    { return selectedChoice==1 ? filterLatency4 : selectedChoice==2 ? filterLatency8 : selectedChoice==3 ? filterLatency16 : 0; }

    // Full latency includes the Ceiling's own audio up/down filter. When the
    // core already runs at the same factor the processor may share that
    // filter; only the two safety windows + residual alignment remain extra.
    int latencySamples() const noexcept
    { return latencySamplesForChoice(selectedChoice); }
    int sharedLatencySamples() const noexcept
    { return sharedLatencySamplesForChoice(selectedChoice); }
    int latencySamplesForChoice(int choice) const noexcept
    {
        choice=juce::jlimit(0,3,choice);
        if(choice==0) return 2*look+analysisDelay;
        const int filter=choice==1 ? filterLatency4 : choice==2 ? filterLatency8 : filterLatency16;
        return 2*look+filter+analysisDelay;
    }
    int sharedLatencySamplesForChoice(int choice) const noexcept
    { juce::ignoreUnused(choice); return 2*look+analysisDelay; }
    bool canShareAudioOversampling(int factor) const noexcept
    { return selectedChoice>0 && factor==oversamplingFactor(); }

    void resetForLimiter(bool tp,float ceilingDb,int recovery=qqsc::params::tpAuto) noexcept
    {
        oversampling4.reset();oversampling8.reset();oversampling16.reset();analysis.reset();
        truePeak1.reset();truePeak4.reset();truePeak8.reset();truePeak16.reset();post.reset();
        std::fill(hardOversampledAlignment.begin(),hardOversampledAlignment.end(),std::array<float,2>{});
        std::fill(analysisAlignment.begin(),analysisAlignment.end(),std::array<float,2>{});
        std::fill(hardPostAlignment.begin(),hardPostAlignment.end(),std::array<float,2>{});
        hardOversampledIndex=analysisIndex=hardPostIndex=0;
        linkSmoother.setCurrentAndTargetValue(stereoLink);activeStereoLink=stereoLink;
        displayGainLR={1,1};
        const bool effectiveTp=tp;
        truePeakBlend.setCurrentAndTargetValue(effectiveTp ? 1.f : 0.f);
        ceiling.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(juce::jlimit(-24.f,0.f,ceilingDb)));
        recoveryMode=juce::jlimit(int(qqsc::params::tpTight),int(qqsc::params::tpSmooth),recovery);
        displayGainLinear=1.0f;inputSamplePeakLinear=0.0f;inputTruePeakLinear=0.0f;
    }

    bool isSilentAndSettled() const noexcept
    {
        if(ceiling.isSmoothing() || truePeakBlend.isSmoothing()) return false;
        return (selectedChoice==0 ? truePeak1 : selectedChoice==1 ? truePeak4 : selectedChoice==2 ? truePeak8 : truePeak16).isSilentAndSettled()
            && post.isSilentAndSettled();
    }

    bool isQuietAndSettled(float floor) const noexcept
    {
        if(ceiling.isSmoothing() || truePeakBlend.isSmoothing()) return false;
        return (selectedChoice==0 ? truePeak1 : selectedChoice==1 ? truePeak4 : selectedChoice==2 ? truePeak8 : truePeak16).isQuietAndSettled(floor)
            && post.isQuietAndSettled(floor);
    }

    // Display telemetry is independent from the peak detection used by the
    // audible protection. Switching this flag must never reset audio state.
    void setAnalysisEnabled(bool enabled) noexcept
    {
        analysisEnabled=enabled;
        if(!enabled){displayGainLinear=1.0f;inputSamplePeakLinear=inputTruePeakLinear=0.0f;}
    }
    float inputSamplePeakForDisplayLinear() const noexcept { return inputSamplePeakLinear; }
    float inputTruePeakForDisplayLinear() const noexcept { return inputTruePeakLinear; }
    float gainForDisplayLinear() const noexcept { return displayGainLinear; }
    float gainForDisplayLinear(int channel) const noexcept {return displayGainLR[size_t(channel)];}
    float gainReductionDbForDisplay() const noexcept
    { return -juce::Decibels::gainToDecibels(juce::jmax(displayGainLinear,1.0e-9f),-180.0f); }

    void set(bool tp,float ceilingDb,int recovery=qqsc::params::tpAuto) noexcept
    {
        // At 1x the audio remains native-rate. The independent reconstruction
        // detector still supplies inter-sample peak protection when TP is on.
        const bool effectiveTp=tp;
        truePeakBlend.setTargetValue(effectiveTp ? 1.f : 0.f);
        ceiling.setTargetValue(juce::Decibels::decibelsToGain(juce::jlimit(-24.f,0.f,ceilingDb)));
        recoveryMode=juce::jlimit(int(qqsc::params::tpTight),int(qqsc::params::tpSmooth),recovery);
    }
    void set(bool /*enabled*/,bool tp,float ceilingDb,int recovery=qqsc::params::tpAuto) noexcept
    { set(tp,ceilingDb,recovery); }

    // Standalone/fallback path used whenever the dynamics-core OS cannot share
    // the exact requested Ceiling factor. 8x preserves the 1.2.28 topology;
    // 16x is the same law at a 16x audible reconstruction rate.
    std::array<float,2> process(float l,float r) noexcept
    {
        activeStereoLink=linkSmoother.getNextValue();
        const float c=ceiling.getNextValue();
        const float t=truePeakBlend.getNextValue();
        if(analysisEnabled)
        {
            inputSamplePeakLinear=juce::jmax(std::abs(l),std::abs(r));
            inputTruePeakLinear=inputSamplePeakLinear;
        }

        if(selectedChoice==0)
        {
            HighStats stats;
            const auto out=processHighSample(l,r,c,t,stats);
            return processPostBaseSample(out[0],out[1],c,t,stats,inputSamplePeakLinear);
        }

        buffer.setSample(0,0,l);buffer.setSample(1,0,r);
        auto block=juce::dsp::AudioBlock<float>(buffer);
        auto& os=currentOversampler();
        auto up=os.processSamplesUp(block);
        HighStats stats;
        for(size_t i=0;i<up.getNumSamples();++i)
        {
            const auto out=processHighSample(up.getSample(0,int(i)),up.getSample(1,int(i)),c,t,stats);
            up.setSample(0,int(i),out[0]);up.setSample(1,int(i),out[1]);
        }
        os.processSamplesDown(block);
        if(analysisEnabled) inputTruePeakLinear=juce::jmax(inputTruePeakLinear,stats.truePeak);
        return processPostBaseSample(buffer.getSample(0,0),buffer.getSample(1,0),c,t,stats,
                                     inputSamplePeakLinear);
    }

    // Shared path at every Lookahead: the dynamics core has already upsampled
    // the mixed/output signal. Apply the Ceiling law at that same 4x/8x/16x
    // rate, then use the core's single downsampler to return to host rate.
    void processSharedOversampledBlock(juce::dsp::AudioBlock<float> block,
                                       int hostSamples,int factor) noexcept
    {
        if(!canShareAudioOversampling(factor) || block.getNumChannels()<2u) return;
        hostSamples=juce::jlimit(0,juce::jmin(maxBlock,int(block.getNumSamples()/size_t(factor))),hostSamples);
        for(int b=0;b<hostSamples;++b)
        {
            activeStereoLink=linkSmoother.getNextValue();
        const float c=ceiling.getNextValue();
            const float t=truePeakBlend.getNextValue();
            HighStats stats;
            // Measure the complete incoming signal on its host-rate sample grid.
            // It already contains A/B, Dry, Makeup, Mix and Output exactly once.
            if(analysisEnabled) sharedSamplePeak[size_t(b)]=juce::jmax(std::abs(block.getSample(0,b*factor)),
                                                   std::abs(block.getSample(1,b*factor)));
            for(int j=0;j<factor;++j)
            {
                const int i=b*factor+j;
                const auto out=processHighSample(block.getSample(0,i),block.getSample(1,i),c,t,stats);
                block.setSample(0,i,out[0]);block.setSample(1,i,out[1]);
            }
            sharedCeiling[size_t(b)]=c;sharedBlend[size_t(b)]=t;sharedLink[size_t(b)]=activeStereoLink;
            if(analysisEnabled)
            {
                sharedHardGain[size_t(b)]=stats.hardGain;sharedTpGain[size_t(b)]=stats.tpGain;
                sharedHardLR[size_t(b)]=stats.hardLR;sharedTpLR[size_t(b)]=stats.tpLR;
                sharedTruePeak[size_t(b)]=stats.truePeak;
            }
            sharedConservative[size_t(b)]=currentTruePeak().conservativeMask(recoveryMode);
        }
    }

    std::array<float,2> finishSharedBaseSample(float l,float r,int baseSample) noexcept
    {
        baseSample=juce::jlimit(0,maxBlock-1,baseSample);
        const float samplePeakLinear=analysisEnabled ? sharedSamplePeak[size_t(baseSample)] : 0.0f;
        HighStats stats;
        if(analysisEnabled)
        {
            stats.hardGain=sharedHardGain[size_t(baseSample)];
            stats.tpGain=sharedTpGain[size_t(baseSample)];
            stats.truePeak=sharedTruePeak[size_t(baseSample)];
            inputSamplePeakLinear=samplePeakLinear;
            inputTruePeakLinear=juce::jmax(samplePeakLinear,stats.truePeak);
        }
        stats.conservative=sharedConservative[size_t(baseSample)];
        stats.hardLR=sharedHardLR[size_t(baseSample)];stats.tpLR=sharedTpLR[size_t(baseSample)];
        activeStereoLink=sharedLink[size_t(baseSample)];
        return processPostBaseSample(l,r,sharedCeiling[size_t(baseSample)],sharedBlend[size_t(baseSample)],stats,
                                     samplePeakLinear);
    }

private:
    static std::array<float,2> clipStereo(float l,float r,float ceiling,float link) noexcept
    {
        float gl=std::abs(l)>ceiling ? ceiling/std::abs(l) : 1.0f;
        float gr=std::abs(r)>ceiling ? ceiling/std::abs(r) : 1.0f;
        params::coupleLimiterGains(gl,gr,link);
        return {l*gl,r*gr};
    }
    struct HighStats
    {
        float hardGain=1.0f,tpGain=1.0f,truePeak=0.0f;
        std::array<float,2> hardLR {1,1},tpLR {1,1};
        uint8_t conservative=0;
    };

    juce::dsp::Oversampling<float>& currentOversampler() noexcept
    { return selectedChoice==1 ? oversampling4 : selectedChoice==2 ? oversampling8 : oversampling16; }
    StereoPeakCap& currentTruePeak() noexcept
    { return selectedChoice==0 ? truePeak1 : selectedChoice==1 ? truePeak4 : selectedChoice==2 ? truePeak8 : truePeak16; }

    std::array<float,2> processHighSample(float inL,float inR,float c,float t,HighStats& stats) noexcept
    {
        if(analysisEnabled)
        {
            const float reconstructedPeak=juce::jmax(std::abs(inL),std::abs(inR));
            stats.truePeak=juce::jmax(stats.truePeak,reconstructedPeak);
            if(reconstructedPeak>c)
                stats.hardGain=juce::jmin(stats.hardGain,c/juce::jmax(reconstructedPeak,1.0e-12f));
        }

        auto& tp=currentTruePeak();
        const auto tpLimited=tp.process(inL,inR,c,activeStereoLink,{},0,-1.0f,recoveryMode);
        if(analysisEnabled)
        {
            stats.tpGain=juce::jmin(stats.tpGain,tp.gainForDisplay());
            float hl=std::abs(inL)>c ? c/std::abs(inL) : 1.f,hr=std::abs(inR)>c ? c/std::abs(inR) : 1.f;
            params::coupleLimiterGains(hl,hr,activeStereoLink);
            stats.hardLR[0]=juce::jmin(stats.hardLR[0],hl);stats.hardLR[1]=juce::jmin(stats.hardLR[1],hr);
            for(int ch=0;ch<2;++ch)stats.tpLR[size_t(ch)]=juce::jmin(stats.tpLR[size_t(ch)],tp.gainForDisplay(ch));
        }
        stats.conservative=tp.conservativeMask(recoveryMode);

        const auto hardClipped=clipStereo(inL,inR,c,activeStereoLink);
        const int delay=look*oversamplingFactor();
        hardOversampledAlignment[hardOversampledIndex]=hardClipped;
        int read=int(hardOversampledIndex)-delay;
        while(read<0)read+=int(hardOversampledAlignment.size());
        const auto hardAligned=hardOversampledAlignment[size_t(read)];
        if(++hardOversampledIndex==hardOversampledAlignment.size()) hardOversampledIndex=0;
        return {hardAligned[0]+t*(tpLimited[0]-hardAligned[0]),
                hardAligned[1]+t*(tpLimited[1]-hardAligned[1])};
    }

    std::array<float,2> processPostBaseSample(float l,float r,float c,float t,HighStats stats,
                                               float samplePeakLinear) noexcept
    {
        buffer.setSample(0,0,l);buffer.setSample(1,0,r);
        auto block=juce::dsp::AudioBlock<float>(buffer);
        const auto reconstructed=analysis.processSamplesUp(block);
        std::array<float,2> detected {};
        for(int ch=0;ch<2;++ch)for(int i=0;i<int(reconstructed.getNumSamples());++i)
            detected[size_t(ch)]=juce::jmax(detected[size_t(ch)],std::abs(reconstructed.getSample(ch,i)));

        analysisAlignment[analysisIndex]={l,r};
        if(++analysisIndex==analysisAlignment.size()) analysisIndex=0;
        const auto aligned=analysisAlignment[analysisIndex];

        constexpr float residualSafetyDb=-0.01f;
        const float residualTarget=c*juce::Decibels::decibelsToGain(residualSafetyDb);
        const auto corrected=post.process(aligned[0],aligned[1],residualTarget,activeStereoLink,detected,
            stats.conservative,c,recoveryMode);

        hardPostAlignment[hardPostIndex]=aligned;
        if(++hardPostIndex==hardPostAlignment.size()) hardPostIndex=0;
        const auto hardFinal=hardPostAlignment[hardPostIndex];
        const auto hardFinalClipped=clipStereo(hardFinal[0],hardFinal[1],c,activeStereoLink);

        if(analysisEnabled)
        {
            const float hardFinalPeak=juce::jmax(std::abs(hardFinal[0]),std::abs(hardFinal[1]));
            if(hardFinalPeak>c)
                stats.hardGain=juce::jmin(stats.hardGain,c/juce::jmax(hardFinalPeak,1.0e-12f));
            const float tpDisplayGain=juce::jlimit(0.0f,1.0f,stats.tpGain*post.gainForDisplay());
            displayGainLinear=juce::jlimit(0.0f,1.0f,stats.hardGain+t*(tpDisplayGain-stats.hardGain));
            float hl=std::abs(hardFinal[0])>c ? c/std::abs(hardFinal[0]) : 1.f;
            float hr=std::abs(hardFinal[1])>c ? c/std::abs(hardFinal[1]) : 1.f;
            params::coupleLimiterGains(hl,hr,activeStereoLink);
            const std::array<float,2> hard {hl,hr};
            for(int ch=0;ch<2;++ch)
            {
                const auto h=juce::jmin(stats.hardLR[size_t(ch)],hard[size_t(ch)]);
                const auto tpGain=stats.tpLR[size_t(ch)]*post.gainForDisplay(ch);
                displayGainLR[size_t(ch)]=juce::jlimit(0.f,1.f,h+t*(tpGain-h));
            }
            inputSamplePeakLinear=samplePeakLinear;
            inputTruePeakLinear=juce::jmax(inputTruePeakLinear,stats.truePeak);
        }
        return {hardFinalClipped[0]+t*(corrected[0]-hardFinalClipped[0]),
                hardFinalClipped[1]+t*(corrected[1]-hardFinalClipped[1])};
    }

    juce::dsp::Oversampling<float> oversampling4{2,2,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true};
    juce::dsp::Oversampling<float> oversampling8{2,3,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true};
    juce::dsp::Oversampling<float> oversampling16{2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true};
    juce::dsp::Oversampling<float> analysis{2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,false};
    juce::AudioBuffer<float> buffer;
    StereoPeakCap truePeak1;
    StereoPeakCap truePeak4,truePeak8,truePeak16,post;
    std::vector<std::array<float,2>> hardOversampledAlignment,analysisAlignment,hardPostAlignment;
    std::vector<float> sharedCeiling,sharedBlend,sharedHardGain,sharedTpGain,sharedTruePeak,sharedSamplePeak;
    std::vector<uint8_t> sharedConservative;
    size_t hardOversampledIndex=0,analysisIndex=0,hardPostIndex=0;
    int look=1,filterLatency4=0,filterLatency8=0,filterLatency16=0,analysisDelay=0,selectedChoice=qqsc::params::ceiling8x,maxBlock=1;
    int recoveryMode=qqsc::params::tpAuto;
    double sampleRate=48000.0;
    juce::SmoothedValue<float> truePeakBlend,ceiling;
    std::vector<float> sharedLink;
    std::vector<std::array<float,2>> sharedHardLR,sharedTpLR;
    std::array<float,2> displayGainLR {1,1};
    juce::SmoothedValue<float> linkSmoother;
    float stereoLink=1.0f,activeStereoLink=1.0f;
    bool analysisEnabled=true;
    float displayGainLinear=1.0f,inputSamplePeakLinear=0.0f,inputTruePeakLinear=0.0f;
};
}
