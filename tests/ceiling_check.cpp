#include "OutputCeiling.h"
#include <iostream>
#include <cmath>
#include <random>
#include <stdexcept>

int main(int argc,char** argv)
{
    std::cout<<std::unitbuf;
    try
    {
        if(argc>1 && std::string(argv[1])=="native-sweep")
        {
            double worst=-200;int cases=0,failures=0;
            for(double rate:{44100.,48000.,96000.})for(int recovery=0;recovery<3;++recovery)
            for(int f=0;f<21;++f)for(int phase=0;phase<3;++phase)
            {
                qqsc::OutputCeiling cap;cap.prepare(rate,true,-1.f,recovery,0);
                const int count=int(rate*.18);juce::AudioBuffer<float> audio(2,count);
                for(int i=0;i<count;++i)
                {
                    const double amp=i<count/3?.03:i<2*count/3?4.:.05;
                    const double frequency=.40+.0049*f;
                    const float x=float(amp*std::sin(2*juce::MathConstants<double>::pi*frequency*i+phase*.71));
                    const auto y=cap.process(x,-x*.8f);audio.setSample(0,i,y[0]);audio.setSample(1,i,y[1]);
                }
                juce::dsp::Oversampling<float> meter(2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,false);
                meter.initProcessing(size_t(count));auto up=meter.processSamplesUp(juce::dsp::AudioBlock<float>(audio));
                float peak=0;for(int ch=0;ch<2;++ch)for(int i=0;i<int(up.getNumSamples());++i)peak=std::max(peak,std::abs(up.getSample(ch,i)));
                const double db=20*std::log10(peak);worst=std::max(worst,db);++cases;
                if(db>-.999){++failures;std::cout<<"NATIVE FAIL rate="<<rate<<" recovery="<<recovery<<" f="<<f<<" phase="<<phase<<" TP="<<db<<"\n";}
            }
            std::cout<<"NATIVE SWEEP cases="<<cases<<" worstTP="<<worst<<" failures="<<failures<<"\n";
            if(failures)throw std::runtime_error("Native near-Nyquist TP bound failed");
            return 0;
        }
        double worstSP=-200,worstTP=-200;int cases=0,failures=0;
        for(int os:{qqsc::params::ceilingNative,qqsc::params::ceiling4x,qqsc::params::ceiling8x,qqsc::params::ceiling16x})
        for(double rate:{44100.,48000.,96000.}) for(bool tp:{false,true})
        for(int shape=0;shape<12;++shape)
        {
            qqsc::OutputCeiling limiter;limiter.prepare(rate,tp,-1.f,qqsc::params::tpAuto,os);
            juce::dsp::Oversampling<float> meter(2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,false);
            const int count=int(rate*.8);meter.initProcessing(size_t(count));
            juce::AudioBuffer<float> audio(2,count);
            std::mt19937 rng(137);std::uniform_real_distribution<float> random(-1,1);
            double sp=0;
            for(int i=0;i<count;++i)
            {
                const double t=i/rate;
                float x=0;
                if(shape<5)x=float(std::sin(juce::MathConstants<double>::twoPi*t*std::array<double,5>{20,1000,rate*.25,rate*.43,rate*.49}[size_t(shape)]+.785398));
                if(shape==5)x=(i%137==0) ? 1.f : 0.f;
                if(shape==6)x=(i%137<68) ? 1.f : -1.f;
                if(shape==7)x=random(rng);
                if(shape==8)x=float((.51+.49*std::sin(juce::MathConstants<double>::twoPi*t*170))*std::sin(juce::MathConstants<double>::twoPi*t*5000));
                if(shape==9)x=i%2 ? -.7f : 1.f;
                if(shape==10)x=float(std::sin(juce::MathConstants<double>::twoPi*t*997)+.8*std::sin(juce::MathConstants<double>::twoPi*t*19871));
                if(shape==11)x=(i>int(rate*.3)&&i<int(rate*.33)) ? 1.f : 0.f;
                const float amp=t<.15? .03f : t<.45 ? (shape==5?100.f:4.f) : .05f;
                const auto y=limiter.process(amp*x,amp*x*(i%37<18 ? -.8f : .6f));
                for(int c=0;c<2;++c){audio.setSample(c,i,y[size_t(c)]);sp=std::max(sp,std::abs(double(y[size_t(c)])));}
            }
            auto up=meter.processSamplesUp(juce::dsp::AudioBlock<float>(audio));
            double peak=sp;
            for(size_t c=0;c<2;++c)for(size_t i=0;i<up.getNumSamples();++i)peak=std::max(peak,std::abs(double(up.getSample(int(c),i))));
            const double spDb=20*std::log10(sp),tpDb=20*std::log10(peak);
            worstSP=std::max(worstSP,spDb);if(tp)worstTP=std::max(worstTP,tpDb);
            std::cout<<rate<<" os="<<limiter.oversamplingFactor()<<" shape="<<shape<<" tp="<<tp<<" SP="<<spDb<<" TP16="<<tpDb<<" latency="<<limiter.latencySamples()<<'\n';
            if(spDb>-.999 || (tp&&tpDb>-.99))++failures;
            ++cases;
        }
        std::cout<<"RESULT "<<cases<<" cases; worst sample="<<worstSP<<" TP16="<<worstTP<<" dBFS/dBTP at -1 ceiling; failures="<<failures<<".\n";
        if(failures)throw std::runtime_error("Ceiling exceeded");
        std::cout<<"PASS: independent peak and true-peak bounds.\n";

        // Ceiling is the target upper bound, not a request for fixed hidden
        // headroom. Under sustained overload, TP mode should use the available
        // headroom closely while remaining below the 0 dBTP ceiling.
        for(double rate:{44100.,48000.,96000.})
        {
            qqsc::OutputCeiling limiter;limiter.prepare(rate,true,true,0.f);
            const int count=int(rate*1.5);
            juce::AudioBuffer<float> audio(2,count);
            for(int i=0;i<count;++i)
            {
                const double t=i/rate;
                const float x=float(4.0*std::sin(juce::MathConstants<double>::twoPi*997.0*t+0.37));
                const auto y=limiter.process(x,x);
                audio.setSample(0,i,y[0]);audio.setSample(1,i,y[1]);
            }
            juce::dsp::Oversampling<float> meter(2,4,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,false);
            meter.initProcessing(size_t(count));
            const auto up=meter.processSamplesUp(juce::dsp::AudioBlock<float>(audio));
            double tpPeak=0;
            for(int ch=0;ch<2;++ch)for(int i=0;i<int(up.getNumSamples());++i)
                tpPeak=std::max(tpPeak,std::abs(double(up.getSample(ch,i))));
            const double tpDb=20*std::log10(std::max(tpPeak,1.e-30));
            std::cout<<"TIGHT rate="<<rate<<" ceiling=0 TP16="<<tpDb<<" dBTP\n";
            if(tpDb>.005 || tpDb<-.05)throw std::runtime_error("TP Ceiling does not approach 0 dBTP tightly");
        }
        std::cout<<"PASS: TP Ceiling=0 approaches 0 dBTP without the former fixed -0.10 dB reserve.\n";

        // 1.4.19: SP uses final host samples; TP also detects intersample peaks.
        // Both have recovery and identical latency when the peak reference changes.
        for(double rate:{44100.,48000.,96000.})
        {
            qqsc::OutputCeiling sp,tp,switched;
            sp.prepare(rate,false,0.f,qqsc::params::tpAuto,0);
            tp.prepare(rate,true,0.f,qqsc::params::tpAuto,0);
            switched.prepare(rate,false,0.f,qqsc::params::tpAuto,0);
            if(sp.latencySamples()!=tp.latencySamples())
                throw std::runtime_error("SP/TP Ceiling latency differs");
            const int count=int(rate*.45);
            float maxSpGr=0,maxTpGr=0;
            for(int i=0;i<count;++i)
            {
                const float x=(i&3)<2 ? .8f : -.8f;
                sp.process(x,x);
                tp.process(x,x);
                if(i>int(rate*.15))
                {
                    maxSpGr=juce::jmax(maxSpGr,sp.gainReductionDbForDisplay());
                    maxTpGr=juce::jmax(maxTpGr,tp.gainReductionDbForDisplay());
                }
            }
            if(maxSpGr>.001f || maxTpGr<.10f)
                throw std::runtime_error("SP/TP telemetry uses the wrong peak reference");

            for(int i=0;i<int(rate*.20);++i)
            {
                const float x=(i&3)<2 ? .8f : -.8f;
                switched.process(x,x);
            }
            const auto latencyBefore=switched.latencySamples();
            const float before=switched.gainReductionDbForDisplay();
            switched.set(true,true,0.f);
            float maxDelta=0.0f;
            for(int i=0;i<int(rate*.04);++i)
            {
                const float x=(i&3)<2 ? .8f : -.8f;
                switched.process(x,x);
                const auto gr=switched.gainReductionDbForDisplay();
                if(!std::isfinite(gr) || gr<-.0001f)
                    throw std::runtime_error("TP display GR became invalid during crossfade");
                maxDelta=juce::jmax(maxDelta,std::abs(gr-before));
            }
            if(switched.latencySamples()!=latencyBefore)
                throw std::runtime_error("TP switch changed Ceiling latency");
            if(maxDelta<.01f)
                throw std::runtime_error("TP switch is not visible in display GR");
            std::cout<<"DISPLAY rate="<<rate<<" spGR="<<maxSpGr<<" tpGR="<<maxTpGr
                     <<" switchDelta="<<maxDelta<<" dB latency="<<latencyBefore<<'\n';
        }
        std::cout<<"PASS: OutputCeiling exposes Sample-Peak / True-Peak attenuation and TP switching keeps fixed latency.\n";


        // Recovery changes only the post-hit return toward unity. All modes keep
        // the same lookahead safety target; TIGHT must recover fastest, AUTO must
        // preserve the 1.2.17 adaptive behaviour, and SMOOTH must retain GR longest.
        for(double rate:{44100.,48000.,96000.})
        {
            std::array<float,3> grAfter20ms {};
            std::array<float,3> grAfter80ms {};
            for(int recovery=qqsc::params::tpTight;recovery<=qqsc::params::tpSmooth;++recovery)
            {
                qqsc::OutputCeiling limiter; limiter.prepare(rate,true,true,0.f,recovery);
                const int warm=int(rate*.08), hit=int(rate*.06), tail=int(rate*.16);
                int tailIndex=0;
                float g20=0,g80=0;
                for(int i=0;i<warm+hit+tail;++i)
                {
                    float amp=.10f;
                    if(i>=warm && i<warm+hit) amp=2.0f;
                    const float x=(i&3)<2 ? amp : -amp;
                    const auto y=limiter.process(x,x);
                    (void)y;
                    if(i>=warm+hit)
                    {
                        ++tailIndex;
                        if(tailIndex==juce::jmax(1,int(rate*.020))) g20=limiter.gainReductionDbForDisplay();
                        if(tailIndex==juce::jmax(1,int(rate*.080))) g80=limiter.gainReductionDbForDisplay();
                    }
                }
                grAfter20ms[size_t(recovery)]=g20;
                grAfter80ms[size_t(recovery)]=g80;
            }
            std::cout<<"RECOVERY rate="<<rate<<" 20ms T/A/S="<<grAfter20ms[0]<<"/"<<grAfter20ms[1]<<"/"<<grAfter20ms[2]
                     <<" 80ms="<<grAfter80ms[0]<<"/"<<grAfter80ms[1]<<"/"<<grAfter80ms[2]<<" dB\n";
            if(!(grAfter20ms[0]+.02f<grAfter20ms[1] && grAfter20ms[1]+.02f<grAfter20ms[2]))
                throw std::runtime_error("TP Recovery 20 ms ordering is not TIGHT < AUTO < SMOOTH GR");
            if(grAfter80ms[0]>.10f || grAfter80ms[2]<grAfter80ms[0]+.05f)
                throw std::runtime_error("TP Recovery tails are not sufficiently distinct");
        }
        std::cout<<"PASS: TP Recovery produces ordered TIGHT/AUTO/SMOOTH release envelopes.\n";
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
