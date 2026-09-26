#define main LegacyDynamicsSelftestMain
#include "updown_processor_test.cpp"
#undef main

namespace
{
void setupDb(QQSuperCompressionAudioProcessor& p,int dual,int mode,float lower,float upper,float ratio)
{
    baseline(p);set(p,"compressionMode",static_cast<float>(dual));set(p,"processingMode",static_cast<float>(mode));
    set(p,"inputGainDb",0);set(p,"outputGainDb",0);set(p,"keySource",0);set(p,"keyHpfHz",0);set(p,"bypass",0);
    const char* mixes[]={"mix","mixL","mixR","mixM","mixS"};
    const char* trims[]={"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"};
    for(size_t d=0;d<5;++d)
    {
        p.setBoundaryForDomainDb(dual!=0,false,static_cast<int>(d),lower);
        p.setBoundaryForDomainDb(dual!=0,true,static_cast<int>(d),upper);
        set(p,qqsc::params::ratioIds[d],ratio);
        set(p,qqsc::params::upRatioIds[d],std::min(ratio,1.0f));
        set(p,qqsc::params::downRatioIds[d],std::max(ratio,1.0f));
        set(p,qqsc::params::upEnabledIds[d],1);set(p,qqsc::params::downEnabledIds[d],1);
        set(p,mixes[d],100);set(p,trims[d],0);
    }
}
void fixedDbChecks()
{
    for(auto threshold:{-10.0f,-30.0f,-90.0f,-110.0f})for(auto ratio:{2.0f,5.0f,6.75f,8.0f,1000.0f})
    {
        const float inputDb=threshold+10,amplitude=std::pow(10.0f,inputDb/20);
        QQSuperCompressionAudioProcessor p;setupDb(p,0,0,threshold,1,ratio);
        const auto result=render(p,amplitude);
        const auto attenuation=20*std::log10(rmsGain(result,amplitude));
        check(std::abs(attenuation+10*(1-1/ratio))<0.002,"Finite dB ratio is not threshold-relative");
        check(result.latency==1248,"Fixed dB law changed 26ms latency");
        if(ratio==5)std::cout<<"dB5:1 input="<<inputDb<<" threshold="<<threshold<<" output="<<inputDb+attenuation<<" dBFS\n";
        if(threshold==-10 && ratio==5)check(harmonicDb(result)<-110,"Steady Down carrier harmonic regression");
        const auto quiet=std::pow(10.0f,(threshold-3)/20);
        check(std::abs(rmsGain(render(p,quiet),quiet)-1)<1e-5,"Finite downward threshold affects sub-threshold material");
    }
    std::cout<<"PASS: finite thresholds -10/-30/-90/-110 and Ratios2/5/6.75/8/1000; equal exceedance/equal dB reduction; actual0dB to-8dB at5:1/-10dB.\n";
}
void dbDomainChecks()
{
    for(double rate:{44100.0,48000.0,96000.0})for(int c:{0,1,2,3})for(int dual:{0,1})for(bool up:{false,true})
    {
        const int mode=c==0?0:c==1?2:1,block=rate==44100?17:rate==48000?256:1024;
        const float ratio=up?0.125f:8.0f,amp=up?0.01f:std::pow(10.0f,-6.0f/20);
        QQSuperCompressionAudioProcessor p;
        setupDb(p,dual,mode,up?-60.0f:(dual?-60.0f:-12.0f),dual?-12.0f:1.0f,ratio);
        const auto result=render(p,amp,block,false,400,rate,c==3?-1.0f:1.0f);
        const auto expectedDb=up?(dual?28.0f:40.0f)*0.875f:-6.0f*0.875f;
        check(std::abs(20*std::log10(rmsGain(result,amp,rate))-expectedDb)<0.015,"Fixed dB law lost in a processing domain");
        check(result.latency==juce::roundToInt(rate*0.026),"Domain latency changed");
        if(rate==48000 && mode==0)check(harmonicDb(result)<-110,"Steady db carrier harmonic regression");
    }
    std::cout<<"PASS: actual new Up/Down in Single/Dual ST/LR/M/S at44.1/48/96kHz,17/256/1024sample blocks and26ms PDC.\n";
}
void dbLegacyAndSymmetryChecks()
{
    for(float ratio:{0.001f,0.125f,1.0f,8.0f,1000.0f})for(float db:{-80.0f,-30.0f,-6.0f})
    {
        const auto amp=std::pow(10.0f,db/20);QQSuperCompressionAudioProcessor p;setupDb(p,0,0,-120,1,ratio);
        const auto expected=ratio<1?1/(ratio+(1-ratio)*amp):1/(1+(ratio-1)*amp);
        check(std::abs(rmsGain(render(p,amp),amp)/expected-1)<0.0001,"-inf fallback does not preserve the original family");
    }
    constexpr float symmetryOffset=35; // T=-40,A=0,R=8: (A-T)*(1-1/R)
    for(float db:{-30.0f,-24.0f,-12.0f,0.0f})
    {
        const auto amp=std::pow(10.0f,db/20);QQSuperCompressionAudioProcessor down,up;
        setupDb(down,0,0,-40,1,8);setupDb(up,0,0,-40,1,0.125f);
        const auto a=render(down,amp),b=render(up,amp);
        const auto offset=20*std::log10(rmsGain(b,amp)/rmsGain(a,amp));
        check(std::abs(offset-symmetryOffset)<0.002,"Reciprocal db ratios have different interior slopes");
        double peakError=0;
        for(size_t i=15000;i<a.output.size();++i)peakError=std::max(peakError,std::abs(b.output[i]*std::pow(10.0,-symmetryOffset/20)-a.output[i]));
        check(peakError<1e-7,"Matched reciprocal waveforms do not null in their common interior");
    }
    QQSuperCompressionAudioProcessor p;setupDb(p,0,0,-110,1,0.125f);
    constexpr float amp=1e-5f;const auto deep=render(p,amp);
    const auto boost=20*std::log10(rmsGain(deep,amp));
    check(std::abs(boost-87.5)<0.01 && std::abs(deep.meter+87.5)<0.02,"Deep finite Up boost is clipped or meter is wrong");
    for(auto sample:render(p,0).output)check(sample==0,"Silence produced output");
    std::cout<<"PASS: -inf legacy fallback; actual8:1/1:8 common-interior waveforms null after constant35dB compensation; deep finite Up +87.5dB and silence.\n";
}
void deepDownPrecisionChecks()
{
    const char* mixes[]={"mix","mixL","mixR","mixM","mixS"};
    for(int dual:{0,1})for(int domain:{0,1,2,3})for(float mix:{0.0f,25.0f,100.0f})
    {
        const int mode=domain==0?0:domain==1?2:1;
        QQSuperCompressionAudioProcessor p;setupDb(p,dual,mode,dual?-119.9f:-110.0f,dual?-110.0f:1.0f,1000);
        for(auto id:mixes)set(p,id,mix);
        const auto result=render(p,1,256,false,400,48000,domain==3?-1.0f:1.0f);
        const double wet=std::pow(10.0,-110*.999/20),amount=mix/100;
        const double expected=wet*amount+1-amount,wantedDb=20*std::log10(expected);
        check(std::abs(20*std::log10(rmsGain(result,1))-wantedDb)<.01,
              "Deep Down lost precision during wet/dry or branch blending");
        const auto meter=domain==3?p.getMeterState().gainReductionDb1.load():result.meter;
        check(std::abs(meter+wantedDb)<.01,"Deep Down gain meter prematurely floors to silence");
        if(mix==100 && domain==0)check(harmonicDb(result)<-110,"Deep Down float cancellation adds carrier harmonics");
    }
    std::cout<<"PASS: -109.89dB deep Down with accurate audio/Mix/meters, ST/LR/M/S Single/Dual and0/25/100% Mix.\n";
}
}
int main()
{
    juce::ScopedJuceInitialiser_GUI initialiser;
    try
    {
        fixedDbChecks();dbDomainChecks();dbLegacyAndSymmetryChecks();deepDownPrecisionChecks();
        collisionChecks();stateChecks();blockChecks();domainChecks();
        boundaryContinuityChecks();branchEnableStateChecks();branchCrossfadeChecks();rangeCrossingAudioChecks();branchOversampledCrossfadeChecks();
        std::cout<<"PASS: QQ Super Compression 1.2.3 Stable fixed-dB dynamics.\n";return 0;
    }
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
