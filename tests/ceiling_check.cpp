#include "OutputCeiling.h"
#include <iostream>
#include <random>
#include <stdexcept>

int main()
{
    std::cout<<std::unitbuf;
    try
    {
        double worstSP=-200,worstTP=-200;int cases=0,failures=0;
        for(double rate:{44100.,48000.,96000.}) for(bool tp:{false,true})
        for(int shape=0;shape<12;++shape)
        {
            qqsc::OutputCeiling limiter;limiter.prepare(rate,true,tp,-1.f);
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
            std::cout<<rate<<" shape="<<shape<<" tp="<<tp<<" SP="<<spDb<<" TP16="<<tpDb<<" latency="<<limiter.latencySamples()<<'\n';
            if(spDb>-.999 || (tp&&tpDb>-.99))++failures;
            ++cases;
        }
        std::cout<<"RESULT "<<cases<<" cases; worst sample="<<worstSP<<" TP16="<<worstTP<<" dBFS/dBTP at -1 ceiling; failures="<<failures<<".\n";
        if(failures)throw std::runtime_error("Ceiling exceeded");
        std::cout<<"PASS: independent peak and true-peak bounds.\n";
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
