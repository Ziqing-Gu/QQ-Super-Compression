// Compare complete idle processing against an always-running audio reference.
static void stoppedFastPath1237()
{
    struct C{bool limiter;float look;int core,ceiling;};
    const C configs[]={{false,0,0,0},{false,26,0,0},{false,0,2,0},
                       {true,0,0,3},{true,26,0,2},{true,0,2,3},{true,0,2,2}};
    double worst=0;
    for(const auto& c:configs)
    {
        QQSuperCompressionAudioProcessor p,reference;Transport host;
        for(auto* q:{&p,&reference})
        {
            auto layout=q->getBusesLayout();layout.inputBuses.set(1,juce::AudioChannelSet::stereo());
            check(q->setBusesLayout(layout),"idle sidechain bus");configure(*q,0,c.core,c.ceiling,c.limiter);
            set(*q,q->soundParameterID("lookaheadMs"),c.look);
            set(*q,q->soundParameterID("ratio"),9.81f);
            set(*q,q->soundParameterID("thresholdDb"),-16.27f);
            set(*q,q->soundParameterID("makeupGainDb"),1.36f);
            set(*q,"limiterOutputDb",14.91f);set(*q,"truePeakLimiting",1);set(*q,"tpRecoveryMode",2);
            q->setEcoMode(false);q->setEditorOpen(true);q->setDisplayKeyHistoryCaptureEnabled(true);
            q->setPlayHead(&host);q->prepareToPlay(48000,256);
        }
        juce::MidiBuffer midi;int counter=0;
        const auto render=[&](int samples,int signal,bool hostBypass=false)
        {
            for(int done=0;done<samples;)
            {
                const int sizes[]={17,256,2048,1,63};const int n=std::min(samples-done,sizes[counter++%5]);
                juce::AudioBuffer<float> a(4,n),b(4,n);a.clear();
                for(int i=0;i<n;++i)
                {
                    if(signal==1)for(int ch=0;ch<2;++ch)a.setSample(ch,i,float(.8*std::sin(2*pi*(ch?73:997)*(host.sample+i)/48000.)));
                    if(signal==2)a.setSample(0,i,1.e-8f); // Above the stopped FULL residual floor.
                    if(signal==3)for(int ch=2;ch<4;++ch)a.setSample(ch,i,.25f);
                }
                b.makeCopyOf(a,true);reference.stoppedSilentSamples=0;reference.stoppedDspSleeping=false;
                const auto fastBefore=p.stoppedFastPathBlocks;
                if(hostBypass){p.processBlockBypassed(a,midi);reference.processBlockBypassed(b,midi);}
                else {p.processBlock(a,midi);reference.processBlock(b,midi);}
                for(int ch=0;ch<2;++ch)for(int i=0;i<n;++i)
                {check(std::isfinite(a.getSample(ch,i)),"idle wake nonfinite");worst=std::max(worst,double(std::abs(a.getSample(ch,i)-b.getSample(ch,i))));}
                if(signal==1 || signal==2 || host.playing || !host.available || (signal==3 && p.readSoundParameter("keySource")>=.5f))
                    check(p.stoppedFastPathBlocks==fastBefore,"active block incorrectly took zero fast path");
                if(counter%6==0){p.timerCallback();reference.timerCallback();}
                check(p.getLatencySamples()==reference.getLatencySamples(),"idle changed PDC");
                check(std::abs(p.getMeterState().truePeakHoldDb.load()-reference.getMeterState().truePeakHoldDb.load())<1.e-4f,"idle TP hold changed");
                // Deliberately leave the host sample position fixed while stopped.
                if(host.playing)host.sample+=n;
                done+=n;
            }
        };
        host.playing=true;render(9600,1);host.playing=false;render(288000,0);
        check(p.stoppedFastPathBlocks>0,"no complete stopped fast path");
        check(p.getDisplayKeyHistoryPosition().counter==reference.getDisplayKeyHistoryPosition().counter,"FULL history stopped advancing");
        const auto before=p.stoppedFastPathBlocks;render(12000,3);
        check(p.stoppedFastPathBlocks>before,"unused INT sidechain prevents sleep");
        p.resetTruePeakHold();reference.resetTruePeakHold();render(37,0);
        p.setUnityMonitorEnabled(true);reference.setUnityMonitorEnabled(true);render(3000,0,true);
        render(600,2);render(288000,0);
        for(auto* q:{&p,&reference})set(*q,q->soundParameterID("mix"),37);
        render(17,0);check(!p.stoppedDspSleeping,"idle parameter edit did not wake");render(288000,0);
        for(auto* q:{&p,&reference})set(*q,q->soundParameterID("keySource"),1);
        render(288000,0);render(12000,3);
        for(auto* q:{&p,&reference})set(*q,q->soundParameterID("keySource"),0);
        render(288000,0);host.available=false;render(17,0);host.available=true;
        host.playing=true;render(12000,1);host.playing=false;render(288000,0);
        for(auto* q:{&p,&reference})q->setNonRealtime(true);
        host.playing=true;render(12000,1);
        p.setPlayHead(nullptr);reference.setPlayHead(nullptr);
        std::cout<<"FAST_IDLE limiter="<<c.limiter<<" look="<<c.look<<" core="<<c.core<<" ceiling="<<c.ceiling<<" fast_blocks="<<p.stoppedFastPathBlocks<<" max_delta="<<worst<<"\n";
    }
    check(worst<1.e-6,"complete idle path changes resumed audio");
    std::cout<<"PASS full idle: Normal native/OS, Limiter shared/separate Ceiling, fixed stopped clock, FULL history/holds, unused/selected SC, tiny input, edits, bypass, Unity, playback and offline wake.\n";
}
static void idleInstanceProfile1237()
{
    struct C{const char* name;bool limiter;float look;int core,ceiling;};
    const C configs[]={{"Normal1x",false,0,0,0},{"Normal26",false,26,0,0},{"Limiter16x",true,0,2,2}};
    std::cout<<"IDLE_PROFILE config,instances,eco,legacy_partial,us_per_chain,one_core_percent\n";
    for(const auto& c:configs)for(int count:{1,10})for(bool eco:{false,true})
    {
        Transport host;std::vector<std::unique_ptr<QQSuperCompressionAudioProcessor>> chain;
        juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;
        for(int n=0;n<count;++n)
        {
            auto p=std::make_unique<QQSuperCompressionAudioProcessor>();configure(*p,0,c.core,c.ceiling,c.limiter);
            set(*p,p->soundParameterID("lookaheadMs"),c.look);set(*p,"truePeakLimiting",1);
            p->setEcoMode(eco);p->setEditorOpen(false);p->setPlayHead(&host);p->prepareToPlay(48000,256);
            for(int i=0;i<300;++i){b.clear();p->processBlock(b,midi);}
            check(p->stoppedFastPathBlocks>0,"profile failed to enter full idle");chain.push_back(std::move(p));
        }
        for(bool legacy:{true,false})
        {
            constexpr int repeats=512;
            const auto start=std::chrono::steady_clock::now();
            for(int n=0;n<repeats;++n)for(auto& p:chain)
            {
                // Reproduce 1.2.36's partial sleep: the block still performs
                // all host-rate loops, while its later DSP guard remains asleep.
                if(legacy)p->stoppedDspSleeping=false;
                b.clear();p->processBlock(b,midi);
            }
            const auto sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            std::cout<<c.name<<','<<count<<','<<eco<<','<<legacy<<','<<sec*1.e6/repeats<<','<<sec/(repeats*256./48000.)*100<<"\n";
        }
        for(auto& p:chain)p->setPlayHead(nullptr);
    }
}
