#pragma once
#include <JuceHeader.h>
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
        look=juce::jmax(1,lookSamples);
        window=look;
        nodes.assign(size_t(window+2),{});
        slowWindow=look+int(std::ceil(rate*.030));
        slowNodes.assign(size_t(slowWindow+2),{});slowHead=slowTail=0;
        audio.assign(size_t(look+1),{});
        gains.assign(size_t(look+1),1.0);
        head=tail=audioIndex=gainIndex=0;index=0;sum=double(look+1);released=1;
        remainingHold=0;
        repeatedRemaining=quietSamples=0;trough=crest=1;rising=false;
        for(auto& p:periods)p={};
        energy={};
        energyCoefficient=1.0-std::exp(-1.0/(rate*.010));
        periodCoefficient=1.0-std::exp(-1.0/(rate*.030));
    }
    bool useConservativeTiming() const noexcept { return repeatedRemaining>0; }
    std::array<float,2> process(float l,float r,float ceiling,float detectedPeak=0,bool repeatedHint=false) noexcept
    {
        const std::array<float,2> input {l,r};
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
        const double target=peak>ceiling ? double(ceiling)/peak : 1.0;
        while(head!=tail && nodes[head].index<index-window) head=(head+1)%nodes.size();
        while(head!=tail)
        {
            const auto back=(tail+nodes.size()-1)%nodes.size();
            if(nodes[back].value<target)break;
            tail=back;
        }
        nodes[tail]={index++,target};tail=(tail+1)%nodes.size();
        const auto now=index-1;
        while(slowHead!=slowTail && slowNodes[slowHead].index<now-slowWindow) slowHead=(slowHead+1)%slowNodes.size();
        while(slowHead!=slowTail)
        {
            const auto back=(slowTail+slowNodes.size()-1)%slowNodes.size();
            if(slowNodes[back].value<target)break;
            slowTail=back;
        }
        slowNodes[slowTail]={now,target};slowTail=(slowTail+1)%slowNodes.size();
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
        const bool repeated=repeatedRemaining>0 || repeatedHint;
        const double controlledMinimum=repeated ? slowNodes[slowHead].value : minimum;
        const int holdSamples=repeated ? 0 : int(std::ceil(rate()*juce::jlimit(.0015,.030,1.1*halfPeriod-double(look)/rate()+.0015)));
        const double releaseSeconds=repeated ? .060 : juce::jlimit(.015,.075,2.5*halfPeriod);
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
        gainIndex=(gainIndex+1)%gains.size();
        audio[audioIndex]={l,r};audioIndex=(audioIndex+1)%audio.size();
        const auto gain=juce::jlimit(0.0,1.0,sum/double(gains.size()));
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
    double sum=0,released=1,sampleRate=48000,energyCoefficient=0,periodCoefficient=0;
    int remainingHold=0;
    int repeatedRemaining=0,quietSamples=0;
    double trough=1,crest=1;
    bool rising=false;
};

class OutputCeiling
{
public:
    void prepare(double rate,bool enabled,bool tp,float ceilingDb)
    {
        look=juce::jmax(1,int(std::ceil(rate*.003)));
        oversampling.initProcessing(1);oversampling.reset();
        filterLatency=juce::roundToInt(oversampling.getLatencyInSamples());
        analysis.initProcessing(256);analysis.reset();
        juce::AudioBuffer<float> impulse(2,256);impulse.clear();impulse.setSample(0,0,1);
        auto measured=analysis.processSamplesUp(juce::dsp::AudioBlock<float>(impulse));
        int maximum=0;for(int i=1;i<int(measured.getNumSamples());++i)
            if(std::abs(measured.getSample(0,i))>std::abs(measured.getSample(0,maximum)))maximum=i;
        analysisDelay=juce::roundToInt(double(maximum)/8.0);analysis.reset();
        latency=2*look+filterLatency+analysisDelay;
        native.prepare(rate,look);truePeak.prepare(rate*8,look*8);
        post.prepare(rate,look);
        peakAlignment.assign(size_t(latency-look+1),{});
        analysisAlignment.assign(size_t(analysisDelay+1),{});
        dry.assign(size_t(latency+1),{});peakIndex=dryIndex=analysisIndex=0;
        buffer.setSize(2,1);
        active.reset(rate,.010);active.setCurrentAndTargetValue(enabled ? 1.f : 0.f);
        truePeakBlend.reset(rate,.010);truePeakBlend.setCurrentAndTargetValue(tp ? 1.f : 0.f);
        ceiling.reset(rate,.010);ceiling.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(ceilingDb));
    }
    int latencySamples() const noexcept { return latency; }
    void set(bool enabled,bool tp,float ceilingDb) noexcept
    {
        active.setTargetValue(enabled ? 1.f : 0.f);
        truePeakBlend.setTargetValue(tp ? 1.f : 0.f);
        ceiling.setTargetValue(juce::Decibels::decibelsToGain(juce::jlimit(-24.f,0.f,ceilingDb)));
    }
    std::array<float,2> process(float l,float r) noexcept
    {
        const float c=ceiling.getNextValue();
        auto peak=native.process(l,r,c);
        peakAlignment[peakIndex]=peak;peakIndex=(peakIndex+1)%peakAlignment.size();
        peak=peakAlignment[peakIndex];
        dry[dryIndex]={l,r};dryIndex=(dryIndex+1)%dry.size();
        buffer.setSample(0,0,l);buffer.setSample(1,0,r);
        auto block=juce::dsp::AudioBlock<float>(buffer);
        auto up=oversampling.processSamplesUp(block);
        // Small reconstruction allowance; final validation uses a separate
        // 16x meter, not this processing interpolator or the GUI's 4x meter.
        const float tpCeiling=c*juce::Decibels::decibelsToGain(-0.02f);
        for(size_t i=0;i<up.getNumSamples();++i)
        {
            const auto limited=truePeak.process(up.getSample(0,int(i)),up.getSample(1,int(i)),tpCeiling);
            up.setSample(0,int(i),limited[0]);up.setSample(1,int(i),limited[1]);
        }
        oversampling.processSamplesDown(block);
        const auto reconstructed=analysis.processSamplesUp(block);
        float detected=0;
        for(int ch=0;ch<2;++ch)for(int i=0;i<int(reconstructed.getNumSamples());++i)
            detected=juce::jmax(detected,std::abs(reconstructed.getSample(ch,i)));
        analysisAlignment[analysisIndex]={buffer.getSample(0,0),buffer.getSample(1,0)};
        analysisIndex=(analysisIndex+1)%analysisAlignment.size();
        const auto aligned=analysisAlignment[analysisIndex];
        // The residual TP correction also follows the main guard's modulation
        // decision: its tiny gain excursions alone may not trip the detector.
        const auto corrected=post.process(aligned[0],aligned[1],c*juce::Decibels::decibelsToGain(-0.10f),detected,truePeak.useConservativeTiming());
        const auto t=truePeakBlend.getNextValue(),a=active.getNextValue();
        // OFF is an exact delayed copy, independent of hidden TP/Ceiling values.
        if (a == 0.0f) return dry[dryIndex];
        for(size_t ch=0;ch<2;++ch)
        {
            const float limited=peak[ch]+t*(corrected[ch]-peak[ch]);
            peak[ch]=dry[dryIndex][ch]+a*(limited-dry[dryIndex][ch]);
        }
        return peak;
    }
private:
    juce::dsp::Oversampling<float> oversampling {2,3,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true};
    juce::dsp::Oversampling<float> analysis {2,3,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,false};
    juce::AudioBuffer<float> buffer;
    SmoothPeakCap native,truePeak,post;
    std::vector<std::array<float,2>> peakAlignment,dry,analysisAlignment;
    size_t peakIndex=0,dryIndex=0,analysisIndex=0;
    int look=1,filterLatency=0,analysisDelay=0,latency=0;
    juce::SmoothedValue<float> active,truePeakBlend,ceiling;
};
}
