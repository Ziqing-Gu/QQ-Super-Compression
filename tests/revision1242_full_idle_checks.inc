static void fullResidual1242()
{
    struct Config {bool limiter,mono,external;int core,ceil,mode;float look,hpf;double rate;int recovery=2;bool tp=true;};
    const Config configs[]={
        {false,false,false,0,0,0,0,0,48000}, {false,false,false,3,3,1,0,79,48000},
        {false,true,true,0,0,0,26,79,44100}, {false,false,false,1,0,2,0,0,96000},
        {true,false,false,3,3,0,0,0,48000}, {true,false,true,2,1,2,0,79,48000},
        {true,true,false,0,3,0,52,0,44100}, {true,false,false,1,2,1,0,0,48000},
        {true,false,false,3,3,0,0,0,48000,1,true}, {true,false,false,1,2,0,0,0,48000,0,false}};
    double allWorst=0;
    for(const auto& c:configs)
    {
        QQSuperCompressionAudioProcessor p,ref;Transport host;host.playing=true;
        for(auto* q:{&p,&ref})
        {
            auto layout=q->getBusesLayout();
            if(c.mono){layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());}
            layout.inputBuses.set(1,juce::AudioChannelSet::stereo());check(q->setBusesLayout(layout),"1242 buses");
            configure(*q,c.mode,0,0,c.limiter);
            set(*q,q->soundParameterID("oversampling"),float(c.core));set(*q,"ceilingOversampling",float(c.ceil));
            set(*q,q->soundParameterID("lookaheadMs"),c.look);set(*q,q->soundParameterID("keyHpfHz"),c.hpf);
            set(*q,q->soundParameterID("keySource"),c.external?1.f:0.f);set(*q,q->soundParameterID("algorithmMode"),1);
            set(*q,q->soundParameterID("ratio"),47);set(*q,q->soundParameterID("thresholdDb"),-18);
            set(*q,"limiterOutputDb",12);set(*q,"ceilingDb",-.3f);set(*q,"truePeakLimiting",c.tp?1.f:0.f);set(*q,"tpRecoveryMode",float(c.recovery));
            q->setEcoMode(false);q->setEditorOpen(false);q->setPlayHead(&host);q->prepareToPlay(c.rate,1024);
        }
        const int channels=c.mono?1:2;
        juce::MidiBuffer midi;double worst=0,maxSettlingSeconds=0;size_t block=0;uint32_t random=0x18c031a2;
        const auto render=[&](int samples,int signal,bool expectAwake)
        {
            const int sizes[]={1024,17,256,3,2048};
            for(int done=0;done<samples;)
            {
                const int n=std::min(samples-done,sizes[block++%5]);
                juce::AudioBuffer<float> a(channels+2,n),b(channels+2,n);a.clear();
                for(int ch=0;ch<channels+2;++ch)for(int i=0;i<n;++i)
                {
                    random=1664525u*random+1013904223u;
                    float x=1.e-10f;
                    if(signal==1)x=float(.7*std::sin(.13*(host.sample+i)+ch*.9));
                    if(signal==2)x=(i%2?1.f:-1.f)*1.e-10f;
                    if(signal==3)x=float(double(random)/2147483648.0-1.0)*1.e-10f;
                    if(signal==4)x=1.e-7f; // -140 dBFS must still wake live monitoring.
                    if(signal==5)x=ch>=channels ? .1f : 1.e-10f;
                    if(signal==6)x=i==0 && done==0 && ch==0 ? 1.f : 1.e-10f;
                    if(signal==7)x=block%2 ? 0.f : 1.e-10f;
                    if(signal==8)x=float(double(random)/2147483648.0-1.0)*1.e-5f;
                    a.setSample(ch,i,x);
                }
                b.makeCopyOf(a,true);ref.stoppedSilentSamples=0;ref.stoppedDspSleeping=false;
                p.processBlock(a,midi);ref.processBlock(b,midi);
                if(expectAwake || (signal==6 && done==0))check(!p.stoppedDspSleeping,"1242 did not wake in current block");
                for(int ch=0;ch<channels;++ch)for(int i=0;i<n;++i)
                {
                    const float x=a.getSample(ch,i),y=b.getSample(ch,i);
                    check(std::isfinite(x),"1242 nonfinite output");
                    worst=std::max(worst,double(std::abs(x-y)));
                }
                check(p.getLatencySamples()==ref.getLatencySamples(),"1242 PDC changed");
                host.sample+=n;done+=n;
            }
        };
        const auto settle=[&](int signal=0)
        {
            int samples=0;
            do {render(int(c.rate*.25),signal,false);samples+=int(c.rate*.25);}
            while(!p.stoppedDspSleeping && samples<int(c.rate*6));
            maxSettlingSeconds=std::max(maxSettlingSeconds,samples/c.rate);
            if(!p.stoppedDspSleeping)std::cout<<p.getPerformanceDiagnostics()<<"\n";
            check(p.stoppedDspSleeping,"1242 quiet input failed to settle within six seconds");
            render(2048,signal,false);
        };
        render(int(c.rate*.15),1,true);host.playing=false;
        settle();
        if(!p.stoppedDspSleeping)
            std::cout<<"FULL_RESIDUAL_DEBUG limiter="<<c.limiter<<" core="<<c.core<<" ceiling="<<c.ceil
                     <<" external="<<c.external<<" diagnostics="<<p.getPerformanceDiagnostics()<<"\n";
        check(p.stoppedDspSleeping && p.stoppedFastPathBlocks>0,"1242 -200 dB DC never slept");
        check(p.performanceWindows[0].idleGate.load()==13,"1242 residual sleep diagnostic missing");
        const auto counter=p.detectorSampleCounter;
        render(4096,2,false);render(4096,3,false);render(4096,7,false);
        check(p.detectorSampleCounter==counter,"1242 nearzero residual still runs oversampled detector");
        p.setEditorOpen(true);render(1024,0,false);p.setEditorOpen(false);
        check(p.stoppedDspSleeping,"1242 editor reopening wakes audio unnecessarily");
        render(int(c.rate*.12),8,true);settle(); // Ordinary stopped noise must remain audible.
        render(1024,4,true);settle(7);
        check(p.stoppedDspSleeping,"1242 did not resleep after live tiny signal");
        render(int(c.rate*.12),6,false);
        // First block contains an impulse while transport remains stopped.
        check(worst<2.e-6,"1242 stopped live wake/tails differ from continuous reference");
        settle();check(p.stoppedDspSleeping,"1242 wake repeat did not resleep");
        if(c.external){render(1024,5,true);settle();}
        for(int wake=0;wake<4;++wake)
        {
            if(wake==0)host.playing=true;
            if(wake==1)host.recording=true;
            if(wake==2){p.setNonRealtime(true);ref.setNonRealtime(true);}
            if(wake==3)host.available=false;
            render(1024,0,true);render(4096,6,true);
            host.playing=false;host.recording=false;host.available=true;p.setNonRealtime(false);ref.setNonRealtime(false);
            if(wake!=3){settle();check(p.stoppedDspSleeping,"1242 active transition resleep");}
        }
        allWorst=std::max(allWorst,worst);p.setPlayHead(nullptr);ref.setPlayHead(nullptr);
        std::cout<<"FULL_RESIDUAL limiter="<<c.limiter<<" core="<<c.core<<" ceiling="<<c.ceil<<" recovery="<<c.recovery<<" tp="<<c.tp<<" rate="<<c.rate<<" max_settling_s="<<maxSettlingSeconds<<" delta="<<worst<<"\n";
    }
    check(allWorst<2.e-6,"1242 aggregate wake/tail error");

    // Input alone must never decide silence when a legal gain raises it.
    for(bool excessiveGain:{false,true})
    {
        QQSuperCompressionAudioProcessor p;Transport host;configure(p,0,0,0,true);
        set(p,p.soundParameterID("oversampling"),0);set(p,"ceilingOversampling",0);
        set(p,"limiterOutputDb",excessiveGain?120.f:0.f);
        p.setEcoMode(false);p.setPlayHead(&host);p.prepareToPlay(48000,1024);
        juce::AudioBuffer<float> b(2,1024);juce::MidiBuffer midi;
        for(int block=0;block<120;++block)
        {for(int ch=0;ch<2;++ch)juce::FloatVectorOperations::fill(b.getWritePointer(ch),1.e-10f,1024);p.processBlock(b,midi);}
        check(p.stoppedDspSleeping!=excessiveGain,"1242 gain safety or low-gain sleep failed");
        if(excessiveGain)check(b.getMagnitude(0,1024)>1.e-5f,"1242 amplified quiet input was lost");
        else
        {
            set(p,"limiterOutputDb",120.f);
            for(int ch=0;ch<2;++ch)juce::FloatVectorOperations::fill(b.getWritePointer(ch),1.e-10f,1024);
            p.processBlock(b,midi);check(!p.stoppedDspSleeping,"1242 gain edit did not wake");
        }
        p.setPlayHead(nullptr);
    }

    // Profile identical stopped FULL input with/without permitting sleep.
    QQSuperCompressionAudioProcessor p,ref;Transport host;
    for(auto* q:{&p,&ref})
    {configure(*q,0,2,2,true);set(*q,"truePeakLimiting",1);q->setEcoMode(false);q->setEditorOpen(false);q->setPlayHead(&host);q->prepareToPlay(48000,1024);}
    juce::AudioBuffer<float> a(2,1024),b(2,1024);juce::MidiBuffer midi;double fastUs=0,activeUs=0;
    for(int k=0;k<240;++k)
    {
        for(int ch=0;ch<2;++ch)juce::FloatVectorOperations::fill(a.getWritePointer(ch),1.e-10f,1024);
        b.makeCopyOf(a,true);ref.stoppedSilentSamples=0;ref.stoppedDspSleeping=false;
        auto t=std::chrono::steady_clock::now();p.processBlock(a,midi);
        auto t2=std::chrono::steady_clock::now();ref.processBlock(b,midi);auto t3=std::chrono::steady_clock::now();
        if(k>=112){fastUs+=std::chrono::duration<double,std::micro>(t2-t).count();activeUs+=std::chrono::duration<double,std::micro>(t3-t2).count();}
    }
    check(p.stoppedDspSleeping,"1242 profile not asleep");
    std::cout<<"FULL_16x_16x_1024 active_us="<<activeUs/128<<" idle_us="<<fastUs/128<<"\n"<<p.getPerformanceDiagnostics()<<"\n";
    p.setPlayHead(nullptr);ref.setPlayHead(nullptr);
    std::cout<<"PASS FULL residual DC/noise, gain protection, current-block live/SC/play/record/offline/unknown wake, tails, UI and PDC; max_delta="<<allWorst<<"\n";
}
