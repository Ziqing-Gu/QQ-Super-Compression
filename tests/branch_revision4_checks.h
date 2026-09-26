// Included inside the actual processor test translation unit.
void boundaryContinuityChecks()
{
    using E = qqsc::StaticCompressionEngine;
    for (float upper : {0.001f, 0.01f, 0.25f, 1.0f})
        for (float lower : {0.0f, upper * 0.1f, upper * 0.99f})
            for (float ratio : {2.0f, 8.0f, 32.0f})
            {
                const auto before = E::singleGainForLevel(std::nextafter(upper,0.0f),ratio,lower,upper);
                check(std::abs(before-1)<1e-6f,"Finite Range still has a discontinuity");
                check(E::singleGainForLevel(upper,ratio,lower,upper)==1,"Range must be exactly unity");
                check(E::singleGainForLevel(upper*1.2f,ratio,lower,upper)==1,"Above Range must be unity");
                for (float p : {lower, (lower+upper)*0.5f, upper})
                    check(E::singleGainForLevel(p,ratio,lower,std::numeric_limits<float>::infinity())
                          == E::gainForLevel(p,ratio,lower),"Range OFF changed original downward law");
                if (lower>0)
                {
                    const auto p=std::nextafter(lower,upper);
                    check(std::abs(E::singleGainForLevel(p,1.0f/32,lower,upper)-1)<1e-6f,
                          "Single upward gate still jumps");
                    check(std::abs(E::dualGainForLevel(p,1.0f/32,ratio,lower,upper)-1)<1e-6f,
                          "Dual upward gate still jumps");
                }
            }
    std::cout<<"PASS: finite Range and upward gate continuity including narrow ranges; strict outside unity and exact Range OFF unbounded-law parity.\n";
}

void branchEnableStateChecks()
{
    QQSuperCompressionAudioProcessor p;
    set(p,"compressionMode",1);
    for(size_t d=0;d<5;++d)
    {
        check(get(p,qqsc::params::upEnabledIds[d])==1 && get(p,qqsc::params::downEnabledIds[d])==1,
              "Branch enables must default ON");
        p.setBoundaryForDomainDb(true,false,static_cast<int>(d),-50);
        p.setBoundaryForDomainDb(true,true,static_cast<int>(d),-12);
        set(p,qqsc::params::upRatioIds[d],0.125f); set(p,qqsc::params::downRatioIds[d],8);
        const auto savedUp=get(p,qqsc::params::upRatioIds[d]),savedDown=get(p,qqsc::params::downRatioIds[d]);
        const auto up=p.getDynamicsGainForDomain(0.05f,static_cast<int>(d));
        const auto down=p.getDynamicsGainForDomain(0.6f,static_cast<int>(d));
        check(up>1 && down<1,"Branch test must exercise both directions");
        set(p,qqsc::params::upEnabledIds[d],0);
        check(p.getDynamicsGainForDomain(0.05f,static_cast<int>(d))==1
              && p.getDynamicsGainForDomain(0.6f,static_cast<int>(d))==down,"Up OFF changed wrong branch");
        set(p,qqsc::params::downEnabledIds[d],0);
        check(p.getDynamicsGainForDomain(0.6f,static_cast<int>(d))==1,"Down OFF not reflected in Display law");
        check(get(p,qqsc::params::upRatioIds[d])==savedUp && get(p,qqsc::params::downRatioIds[d])==savedDown,
              "Branch OFF overwrote stored Ratios");
    }
    p.copyAToB();
    for(const auto& ids : {qqsc::params::upEnabledIds,qqsc::params::downEnabledIds})
        for(auto id:ids) set(p,id,1);
    p.selectABSlot(1);
    juce::MemoryBlock data;p.getStateInformation(data);
    QQSuperCompressionAudioProcessor restored;
    restored.setStateInformation(data.getData(),static_cast<int>(data.getSize()));
    for(const auto& ids : {qqsc::params::upEnabledIds,qqsc::params::downEnabledIds})
        for(auto id:ids) check(get(p,id)==0 && get(restored,id)==0,"Branch enables lost in A/B or host state");
    restored.selectABSlot(0);
    for(const auto& ids : {qqsc::params::upEnabledIds,qqsc::params::downEnabledIds})
        for(auto id:ids) check(get(restored,id)==1,"A/B enable banks are not independent");
    // Restore an older, enable-free project into an already dirty new instance.
    auto state=p.getAPVTS().copyState();
    for(const auto& ids : {qqsc::params::upEnabledIds,qqsc::params::downEnabledIds})
        for(auto id:ids)
            for(int i=state.getNumChildren()-1;i>=0;--i)
                if(state.getChild(i).getProperty("id").toString()==id)state.removeChild(i,nullptr);
    juce::MemoryBlock legacy;auto xml=state.createXml();
    juce::AudioProcessor::copyXmlToBinary(*xml,legacy);
    p.setStateInformation(legacy.getData(),static_cast<int>(legacy.getSize()));
    for(const auto& ids : {qqsc::params::upEnabledIds,qqsc::params::downEnabledIds})
        for(auto id:ids)check(get(p,id)==1,"Old state did not default missing enables ON");
    std::cout<<"PASS: independent enable parameters in ST/LR/MS, Ratio preservation, Display gain, A/B, project state and older-state migration.\n";
}

void branchCrossfadeChecks()
{
    for(double rate : {44100.0,48000.0,96000.0})
        for(bool upward : {true,false})
        {
            QQSuperCompressionAudioProcessor p;baseline(p);set(p,"compressionMode",1);
            set(p,"upThresholdDb",-50);set(p,"downThresholdDb",-12);
            p.setRateAndBufferSizeDetails(rate,256);p.prepareToPlay(rate,256);
            const auto amplitude=upward?0.05f:0.6f;
            const auto* id=upward?qqsc::params::upEnabled:qqsc::params::downEnabled;
            juce::AudioBuffer<float> audio(2,1);juce::MidiBuffer midi;
            const auto sample=[&]()
            {
                audio.setSample(0,0,amplitude);audio.setSample(1,0,amplitude);p.processBlock(audio,midi);
                return audio.getSample(0,0)/amplitude;
            };
            float initial=1;
            for(int i=0;i<static_cast<int>(rate*0.15);++i)initial=sample();
            check(std::abs(initial-1)>0.05f,"Crossfade test requires non-neutral processing");
            const int length=static_cast<int>(rate*0.010);
            float previous=initial,maxStep=0;
            set(p,id,0);
            for(int i=0;i<length+8;++i)
            {
                const auto now=sample();maxStep=std::max(maxStep,std::abs(now-previous));
                const auto expected=initial+(1-initial)*std::min(1.0f,static_cast<float>(i+1)/length);
                check(std::abs(now-expected)<0.001f,"OFF is not a ten millisecond wet/dry crossfade");previous=now;
            }
            check(std::abs(previous-1)<1e-5f,"OFF did not finish at unity");
            set(p,id,1);
            for(int i=0;i<length/2;++i)previous=sample();
            const auto mid=previous;set(p,id,0);
            const auto first=sample();
            check(std::abs(first-mid)<std::abs(initial-1)*1.1f/length,"Rapid toggle restarts with a click");
            for(int i=0;i<length+8;++i)previous=sample();
            check(std::abs(previous-1)<1e-5f,"Reversed crossfade failed to reach unity");
            p.releaseResources();
            std::cout<<"PASS: "<<(upward?"UP":"DOWN")<<" 10ms crossfade @"<<rate<<" Hz, max gain step "<<maxStep<<"; interrupted fade continuous.\n";
        }
}

void rangeCrossingAudioChecks()
{
    for(double rate : {44100.0,48000.0,96000.0})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"compressionMode",0);
        const float range=juce::Decibels::decibelsToGain(-6.0f);
        constexpr int block=257;
        p.setRateAndBufferSizeDetails(rate,block);p.prepareToPlay(rate,block);
        const int count=static_cast<int>(rate*2.0),latency=p.getLatencySamples();
        std::vector<float> input(static_cast<size_t>(count)),output;output.reserve(input.size());
        for(int i=0;i<count;++i)
            input[static_cast<size_t>(i)]=range*(1+0.06f*static_cast<float>(std::sin(2*pi*3*i/rate)))
                *static_cast<float>(std::sin(2*pi*600*i/rate));
        juce::MidiBuffer midi;
        for(int offset=0;offset<count;offset+=block)
        {
            const int n=std::min(block,count-offset);juce::AudioBuffer<float> audio(2,n);
            for(int i=0;i<n;++i)for(int ch=0;ch<2;++ch)audio.setSample(ch,i,input[static_cast<size_t>(offset+i)]);
            p.processBlock(audio,midi);
            for(int i=0;i<n;++i)output.push_back(audio.getSample(0,i));
        }
        float maxJump=0;
        for(int i=static_cast<int>(rate*.3);i<count;++i)
        {
            const float x=input[static_cast<size_t>(i-latency)],prior=input[static_cast<size_t>(i-latency-1)];
            if(std::abs(x)<0.1f || std::abs(prior)<0.1f)continue;
            maxJump=std::max(maxJump,std::abs(output[static_cast<size_t>(i)]/x-output[static_cast<size_t>(i-1)]/prior));
        }
        std::cout<<"RANGE DIAGNOSTIC @"<<rate<<": max gain change "<<maxJump<<"\n";
        // New finite dB reduction is much stronger than the legacy curve here.
        // Compare the actual waveform against an independent double-precision
        // future-peak + continuous-range reference rather than the old curve's
        // absolute per-sample gain-change limit.
        double maxError=0,maxReferenceJump=0,previousReference=1;
        for(int i=static_cast<int>(rate*.3)-1;i<count;++i)
        {
            double peak=0;
            for(int j=i-latency;j<=i;++j)
                peak=std::max(peak,std::abs(static_cast<double>(input[static_cast<size_t>(j)])));
            const double lower=std::pow(10.0,-40.0/20.0),upper=std::pow(10.0,-6.0/20.0);
            double reference=1;
            if(peak>lower && peak<upper)
            {
                const double t=std::clamp((upper-peak)/((upper-lower)*.5),0.0,1.0);
                reference=1+(std::pow(lower/peak,.875)-1)*t*t*(3-2*t);
            }
            const float x=input[static_cast<size_t>(i-latency)];
            maxError=std::max(maxError,std::abs(output[static_cast<size_t>(i)]-x*reference));
            if(i>=static_cast<int>(rate*.3))maxReferenceJump=std::max(maxReferenceJump,std::abs(reference-previousReference));
            previousReference=reference;
        }
        check(maxError<2e-6 && maxJump<=maxReferenceJump+2e-6,
              "Actual Range crossing differs from independent continuous-curve reference");
        std::cout<<"RANGE REFERENCE: max waveform error "<<maxError<<", max continuous reference gain change "<<maxReferenceJump<<"\n";
        check(latency==static_cast<int>(std::round(rate*0.026)),"Range fix added latency");
        std::cout<<"PASS: actual 600Hz carrier crossing Range at 3Hz @"<<rate<<"; max adjacent gain change "<<maxJump<<", unchanged 26ms latency.\n";
    }
}

void branchOversampledCrossfadeChecks()
{
    for(int os : {1,2})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);set(p,"compressionMode",1);
        set(p,"lookaheadMs",0);set(p,"oversampling",static_cast<float>(os));
        set(p,"upThresholdDb",-50);set(p,"downThresholdDb",-12);
        p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);
        juce::AudioBuffer<float> audio(2,1);juce::MidiBuffer midi;
        const auto sample=[&]()
        {audio.setSample(0,0,.05f);audio.setSample(1,0,.05f);p.processBlock(audio,midi);return audio.getSample(0,0)/.05f;};
        // Compare against this oversampling filter's own neutral DC response,
        // not ideal unity: its inherited passband response is not exactly 1.
        set(p,"upRatio",1);
        float neutral=1;
        for(int i=0;i<7000;++i)neutral=sample();
        set(p,"upRatio",.125f);
        float initial=1;
        for(int i=0;i<7000;++i)initial=sample();
        set(p,"upEnabled",0);
        float previous=initial,maxStep=0,finished=0;
        int ten=-1,ninety=-1;
        const int latency=p.getLatencySamples();
        for(int i=0;i<1200;++i)
        {
            const auto now=sample();maxStep=std::max(maxStep,std::abs(now-previous));previous=now;
            const auto progress=(initial-now)/(initial-neutral);
            if(ten<0 && progress>=.1f)ten=i;
            if(ninety<0 && progress>=.9f)ninety=i;
            if(i==480+latency+32)finished=now;
        }
        check(maxStep<std::abs(initial-neutral)/480*1.2f,"Oversampled enable switch has a hard jump");
        // The parameter fade enters AFTER upsampling, so input-to-output PDC
        // is not its onset delay. Measure its central slope after filtering.
        std::cout<<"OVERSAMPLED: factor "<<(os==1?8:16)<<", latency "<<latency
                 <<", 10%-90% samples "<<(ninety-ten)<<", final/neutral gain "<<finished<<"/"<<neutral<<".\n";
        check(ten>=0 && ninety>ten && std::abs((ninety-ten)-384)<=4 && std::abs(finished-neutral)<.0001f,
              "Oversampling changed the ten millisecond crossfade duration");
        std::cout<<"PASS: 0ms/"<<(os==1?8:16)<<"x switch crossfade retains 10ms duration; max gain step "<<maxStep<<".\n";
    }
}
