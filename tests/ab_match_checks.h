namespace
{
void matchedABChecks()
{
    double worstNull=0,worstExcursion=0;
    const char* ratios[]={"ratio","ratioL","ratioR","ratioM","ratioS"};
    const char* trims[]={"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"};
    const char* mixes[]={"mix","mixL","mixR","mixM","mixS"};
    for(double rate:{44100.,48000.,96000.})for(int os:{-1,0,1,2})for(int mode:{0,1,2})for(bool exact:{false,true})for(float mix:{0.f,50.f,100.f})
    {
        QQSuperCompressionAudioProcessor p,a,b;
        const int block=rate==44100?17:rate==48000?256:1024;
        const auto bank=[&](auto& proc,bool up)
        {
            setupDb(proc,0,mode,-120,1,up?.1f:10.f);set(proc,"algorithmMode",1);
            set(proc,"lookaheadMs",os==-1?26.f:0.f);set(proc,"oversampling",float(std::max(0,os)));
            for(auto id:ratios)set(proc,id,up?.1f:10.f);
            for(auto id:trims)set(proc,id,up?(exact?-14.93f:-14.88f):5.07f);
            for(auto id:mixes)set(proc,id,mix);
        };
        bank(p,false);p.copyAToB();p.selectABSlot(1);bank(p,true);p.selectABSlot(0);
        bank(a,false);bank(b,true);
        for(auto* proc:{&p,&a,&b}){proc->setRateAndBufferSizeDetails(rate,block);proc->prepareToPlay(rate,block);}
        juce::MidiBuffer midi;
        const int total=int(rate*.65);
        const int switches[]={int(rate*.15),int(rate*.153),int(rate*.18),int(rate*.30),int(rate*.45)};
        int nextSwitch=0;
        double error=0,excursion=0,peak=0;
        for(int n=0;n<total;)
        {
            if(nextSwitch<5&&n==switches[nextSwitch]){p.selectABSlot(nextSwitch%2==0?1:0);++nextSwitch;}
            int count=std::min(block,total-n);
            if(nextSwitch<5)count=std::min(count,switches[nextSwitch]-n);
            juce::AudioBuffer<float> source(2,count),outA(2,count),outB(2,count);
            for(int i=0;i<count;++i)
            {
                const double t=(n+i)/rate;
                const float envelope=float(.2+.15*std::sin(2*pi*7*t));
                source.setSample(0,i,envelope*float(std::sin(2*pi*317*t)+.2*std::sin(2*pi*1103*t)));
                source.setSample(1,i,envelope*.7f*float(std::sin(2*pi*503*t)));
            }
            outA.makeCopyOf(source);outB.makeCopyOf(source);
            p.processBlock(source,midi);a.processBlock(outA,midi);b.processBlock(outB,midi);
            if(n>int(rate*.10))for(int c=0;c<2;++c)for(int i=0;i<count;++i)
            {
                const double x=source.getSample(c,i),lo=std::min(outA.getSample(c,i),outB.getSample(c,i)),hi=std::max(outA.getSample(c,i),outB.getSample(c,i));
                check(std::isfinite(x),"A/B nonfinite output");
                peak=std::max(peak,std::abs(double(outA.getSample(c,i))));
                error=std::max(error,std::abs(x-outA.getSample(c,i)));
                excursion=std::max(excursion,std::max(lo-x,x-hi));
            }
            n+=count;
        }
        if(exact)worstNull=std::max(worstNull,error/peak);
        worstExcursion=std::max(worstExcursion,excursion/peak);
        if((exact&&error/peak>2e-5)||excursion/peak>2e-5)
            std::cerr<<"AB rate="<<rate<<" os="<<os<<" mode="<<mode<<" exact="<<exact<<" residual="<<error/peak<<" excursion="<<excursion/peak<<'\n';
        check(!exact||error/peak<2e-5,"Matched reciprocal A/B adds transient dip or boost");
        check(excursion/peak<2e-5,"Screenshot A/B leaves the range of its complete outputs");
        check(p.getLatencySamples()==a.getLatencySamples(),"A/B changed latency");
    }
    std::cout<<"PASS: matched reciprocal A/B dynamic stereo audio, screenshot and exact compensation, both directions / rapid reversal / final handoff; 216 cases, ST/LR/MS,44.1/48/96k,17/256/1024 blocks,26ms and0ms1x/8x/16x,0/50/100% Mix; worst relative null="<<worstNull<<", excursion="<<worstExcursion<<".\n";
}

struct MatchPlayHead : juce::AudioPlayHead
{
    int64_t sample=0; bool playing=true;
    juce::Optional<PositionInfo> getPosition() const override
    {PositionInfo info;info.setIsPlaying(playing);info.setTimeInSamples(sample);return info;}
};
void deepMatchChecks()
{
    for(float db:{-80.f,-120.f,-180.f,-240.f})
    {
        qqsc::BS1770LoudnessMatch m;m.prepare(48000);
        const float amplitude=std::pow(10.f,db/20);
        for(int n=0;n<48000;++n)
        {
            const float dry=amplitude*float(std::sin(2*pi*1000*n/48000.)),wet=dry*.1f;
            m.processSample(dry,dry,wet,wet,wet,wet,dry,dry,wet,wet);
        }
        m.servicePending();
        check(m.getLatestMatch().validST && std::abs(m.getLatestMatch().st-20)<.003,"MATCH still has an absolute low-level gate");
    }
    // Isolated K-weighting / gate checks: both huge attenuation and boost,
    // ordinary matching, quiet gaps, and truly missing Dry/Wet data.
    for(float difference:{6.f,40.f,65.f,100.f,119.f,-60.f,-119.f})
    {
        qqsc::BS1770LoudnessMatch m;m.prepare(48000);
        const float gain=std::pow(10.f,-difference/20);
        for(int n=0;n<144000;++n)
        {
            const float dry=n>48000&&n<72000?0.f:.1f*float(std::sin(2*pi*1000*n/48000.));
            const float wet=dry*gain;
            m.processSample(dry,dry,wet,wet,wet,wet,dry,dry,wet,wet);
        }
        m.servicePending();
        const auto r=m.getLatestMatch();
        check(r.validST&&r.validL&&r.validR&&r.validM&&r.validS,"Deep MATCH unavailable");
        for(float result:{r.st,r.l,r.r,r.m,r.s})check(std::abs(result-difference)<.003,"Deep MATCH clipped or mismeasured");
    }
    for(int absent:{0,1,2})
    {
        qqsc::BS1770LoudnessMatch m;m.prepare(48000);
        for(int n=0;n<48000;++n)
        {
            const float tone=.1f*float(std::sin(2*pi*1000*n/48000.));
            const float dry=absent==1?tone:0,wet=absent==2?tone:0;
            m.processSample(dry,dry,wet,wet,wet,wet,dry,dry,wet,wet);
        }
        m.servicePending();
        check(!m.hasAnyResult(),"MATCH invents a gain for silence / absent Wet");
    }
    for(int mode:{0,1,2})for(bool sides:{false,true})
    {
        QQSuperCompressionAudioProcessor p;setupDb(p,0,mode,-120,1,qqsc::normalMaximumDownRatio);
        MatchPlayHead head;p.setPlayHead(&head);p.prepareToPlay(48000,256);
        juce::MidiBuffer midi;
        const auto run=[&](int count)
        {
            double energy=0;
            for(int n=0;n<count;n+=256)
            {
                const int size=std::min(256,count-n);juce::AudioBuffer<float> block(2,size);
                for(int i=0;i<size;++i){const float x=.1f*float(std::sin(2*pi*400*(head.sample+i)/48000.));block.setSample(0,i,x);block.setSample(1,i,sides?-x:x);}
                p.processBlock(block,midi);head.sample+=size;
                if(n>count/2)for(int i=0;i<size;++i)energy+=double(block.getSample(0,i))*block.getSample(0,i);
            }
            return energy;
        };
        run(144000);p.refreshMatchResults();check(p.hasMatchData()&&p.applyMatchForCurrentMode(),"Processor MATCH did not apply after deep compression");
        const char* id=mode==0?"makeupGainDb":mode==2?"makeupGainLDb":sides?"makeupGainSDb":"makeupGainMDb";
        check(std::abs(get(p,id)-qqsc::normalMaximumMakeupDb)<.02,"Deep Normal MATCH must respect the current Makeup range");
        const double energy=run(96000);
        // Count samples actually accumulated in the last half.
        const int measured=96000-48128;
        const double expectedDb=-70*(1.0-1.0/qqsc::normalMaximumDownRatio)+qqsc::normalMaximumMakeupDb;
        check(std::abs(10*std::log10(energy/measured/.005)-expectedDb)<.03,"Normal MATCH residual disagrees with its Makeup cap");
        juce::MemoryBlock state;p.getStateInformation(state);QQSuperCompressionAudioProcessor restored;
        restored.setStateInformation(state.getData(),int(state.getSize()));
        check(std::abs(get(restored,id)-get(p,id))<.01,"Extended Makeup lost in project state");
        p.setPlayHead(nullptr);
    }
    QQSuperCompressionAudioProcessor trim;setupDb(trim,0,0,-120,1,1);set(trim,"makeupGainDb",-qqsc::normalMaximumMakeupDb);
    check(std::abs(rmsGain(render(trim,.1f),.1)/std::pow(10.,-qqsc::normalMaximumMakeupDb/20.)-1)<.001,"Negative deep Makeup is accidentally treated as silence");
    std::cout<<"PASS: MATCH has no absolute gate (-80/-120/-180/-240dB source); +/-119dB K-weighted differences, silence/absent-Wet rejection, actual Normal69.65dB attenuation with capped30dB MATCH and verified39.65dB residual in ST/LR/M/S; project state and -30dB Makeup.\n";
}
}
