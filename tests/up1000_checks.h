void up1000Checks()
{
    constexpr float upMinimum=1.0f/1000.0f;
    using E=qqsc::StaticCompressionEngine;
    QQSuperCompressionAudioProcessor p;
    for(const auto& ids:{qqsc::params::ratioIds,qqsc::params::upRatioIds})
        for(auto id:ids)
        {
            auto* parameter=p.getAPVTS().getParameter(id);
            check(std::abs(get(p,id)-1)<1e-6f,"Unity Ratio default changed");
            check(std::abs(parameter->convertFrom0to1(0)-upMinimum)<1e-9f,"Up endpoint is not 1/1000");
            check(parameter->getText(0,32)=="1:1000","Host endpoint label is wrong");
            for(const char* text:{"1:1000","1/1000","0.001"})
                check(std::abs(parameter->convertFrom0to1(parameter->getValueForText(text))-upMinimum)<1e-9f,
                      "Up endpoint host text input failed");
            set(p,id,upMinimum);check(std::abs(get(p,id)-upMinimum)<1e-9f,"Up endpoint lost in parameter mapping");
        }
    for(auto id:qqsc::params::downRatioIds)
    {
        auto* parameter=p.getAPVTS().getParameter(id);
        check(parameter->convertFrom0to1(0)==1 && parameter->convertFrom0to1(1)==1000,"Down range is not 1 to 1000");
        check(parameter->getText(1,32)=="1000:1","Down endpoint must be finite 1000:1");
    }
    for(auto id:qqsc::params::ratioIds)
    {
        auto* parameter=p.getAPVTS().getParameter(id);
        check(parameter->convertFrom0to1(0.5f)==1 && parameter->convertTo0to1(1)==0.5f,"Single unity must remain centred");
        for(float n:{0.5f,0.6f,0.75f,0.9f,1.0f})
            check(std::abs(parameter->convertFrom0to1(n)-std::pow(1000.0f,(n-0.5f)*2))<1e-5f,
                  "Single downward half has the wrong finite scale");
    }
    p.copyAToB();
    for(const auto& ids:{qqsc::params::ratioIds,qqsc::params::upRatioIds})for(auto id:ids)set(p,id,0.125f);
    p.selectABSlot(1);juce::MemoryBlock state;p.getStateInformation(state);
    QQSuperCompressionAudioProcessor restored;restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    for(const auto& ids:{qqsc::params::ratioIds,qqsc::params::upRatioIds})for(auto id:ids)
        check(std::abs(get(restored,id)-upMinimum)<1e-9f,"1/1000 was lost in A/B or project state");
    restored.selectABSlot(0);
    for(const auto& ids:{qqsc::params::ratioIds,qqsc::params::upRatioIds})for(auto id:ids)
        check(std::abs(get(restored,id)-0.125f)<1e-6f,"Existing finite A/B bank changed");
    std::cout<<"PASS: ten 1/1000 endpoints, finite Down 1000:1, host labels/text, centred Single unity, unity defaults, saved state/A-B.\n";

    const char* mixes[]={"mix","mixL","mixR","mixM","mixS"};
    const auto configure=[&](QQSuperCompressionAudioProcessor& q,int dual,int mode,float wet)
    {
        baseline(q);set(q,"compressionMode",static_cast<float>(dual));set(q,"processingMode",static_cast<float>(mode));
        for(size_t d=0;d<5;++d)
        {
            set(q,qqsc::params::thresholdIds[d],-120);set(q,qqsc::params::rangeIds[d],1);
            set(q,qqsc::params::upThresholdIds[d],-120);set(q,qqsc::params::downThresholdIds[d],0);
            set(q,qqsc::params::ratioIds[d],upMinimum);set(q,qqsc::params::upRatioIds[d],upMinimum);
            set(q,qqsc::params::downRatioIds[d],1);set(q,mixes[d],wet);
        }
    };
    constexpr float amplitude=1.0e-5f;
    const auto expected=1.0f/(upMinimum+(1-upMinimum)*amplitude);
    check(expected>900 && E::effectiveGainForMix(expected,1)>900,"Mix or Display still clamps upward gain to 32/100");
    for(int dual:{0,1})
        for(int domainCase:{0,1,2,3})
            for(double rate:{44100.0,48000.0,96000.0})
            {
                const int mode=domainCase==0?0:domainCase==1?2:1;
                const int block=rate==44100?17:rate==48000?256:1024;
                QQSuperCompressionAudioProcessor q;configure(q,dual,mode,100);
                const auto result=render(q,amplitude,block,false,400,rate,domainCase==3?-1.0f:1.0f);
                const auto measured=rmsGain(result,amplitude,rate);
                check(std::abs(measured/expected-1)<0.0005,"Actual output does not use 1/1000 Ratio in every domain");
                check(result.latency==juce::roundToInt(rate*0.026),"Up range change added latency");
                if(mode==0 && rate==48000)
                {
                    check(std::abs(result.meter+20*std::log10(expected))<0.01,"Boost meter still clamps high upward gain");
                    const auto thd=harmonicDb(result);check(thd<-110,"1/1000 steady-tone harmonics regressed");
                    std::cout<<"PASS: "<<(dual?"Dual":"Single")<<" 1:1000, -100 dBFS input -> "
                             <<20*std::log10(measured*amplitude)<<" dBFS; boost "<<20*std::log10(measured)
                             <<" dB, H2-H7 "<<thd<<" dBc.\n";
                }
            }
    for(int dual:{0,1})
    {
        QQSuperCompressionAudioProcessor q;configure(q,dual,0,50);
        const auto result=render(q,amplitude);
        check(std::abs(rmsGain(result,amplitude)/(1+(expected-1)*0.5f)-1)<0.0005,"High upward gain broke parallel Mix");
        set(q,dual?"upThresholdDb":"thresholdDb",-90);
        check(std::abs(rmsGain(render(q,amplitude),amplitude)-1)<1e-5,"1/1000 boosts below its enabling gate");
        const auto silent=render(q,0);
        for(float x:silent.output)check(x==0,"1/1000 produces nonzero silence");
    }
    for(float lower:{0.00001f,0.01f,0.1f})
    {
        const float upper=lower*2;
        check(std::abs(E::singleGainForLevel(std::nextafter(lower,upper),upMinimum,lower,upper)-1)<1e-6f,
              "Stronger upward ratio broke gate continuity");
        check(std::abs(E::dualGainForLevel(std::nextafter(upper,lower),upMinimum,8,lower,upper)-1)<2e-6f,
              "Stronger upward ratio broke Down handoff continuity");
    }
    for(int dual:{0,1})
    {
        QQSuperCompressionAudioProcessor q;configure(q,dual,0,100);
        set(q,"ratio",1000);set(q,"downRatio",1000);set(q,"thresholdDb",-12);set(q,"downThresholdDb",-12);
        constexpr float loud=0.5f;
        const auto expectedDown=(1+999*juce::Decibels::decibelsToGain(-12.0f))/(1+999*loud);
        const auto result=render(q,loud);
        check(std::abs(rmsGain(result,loud)-expectedDown)<1e-5,"Actual finite 1000:1 law or parameter binding is wrong");
        check(harmonicDb(result)<-110,"1000:1 steady-tone harmonic regression");
        juce::MemoryBlock saved;q.getStateInformation(saved);QQSuperCompressionAudioProcessor r;
        r.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
        check(std::abs(get(r,"ratio")-1000)<0.001 && std::abs(get(r,"downRatio")-1000)<0.001,"Finite high Ratio not restored");
    }
    std::cout<<"PASS: actual 1/1000 ST/LR/M/S at three rates/block sizes, full/parallel Mix, gate, silence, boundaries; finite 1000:1 Single/Dual audio and state.\n";
}
