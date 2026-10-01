static void eco1238()
{
    // Direct window reference checks the detector independently of processor DSP.
    for (int look : {0,1,31,1248})
    {
        qqsc::StaticCompressionEngine e; e.prepare(1248); e.setLookaheadSamples(look);
        std::vector<float> x(4200);
        for (int i=0;i<int(x.size());++i)
        {
            x[size_t(i)]=std::min(1.f,float(std::abs(1.4*std::sin(i*.013)*std::sin(i*.31))));
            float future=0,past=0;
            for (int j=std::max(0,i-look);j<=i;++j)future=std::max(future,x[size_t(j)]);
            if(i>=look)for(int j=std::max(0,i-2*look);j<=i-look;++j)past=std::max(past,x[size_t(j)]);
            check(e.processLevelSample(x[size_t(i)],i)==std::min(future,past),"detector window differs from reference");
        }
    }
    struct C {int core,ceiling;bool limiter,mono,external;float look;double rate;};
    const C configs[]={{0,0,false,false,false,0,48000},{3,0,false,false,false,0,44100},
        {1,0,false,false,false,0,48000},{2,0,false,false,false,0,96000},
        {2,2,true,false,false,0,48000},{3,3,true,false,true,0,48000},
        {1,2,true,false,true,0,96000},{2,3,true,true,true,0,44100},
        {0,2,true,false,false,26,48000},{0,0,false,true,false,52,48000}};
    double worst=0;uint64_t transitions=0;
    for (const auto& c:configs)
    {
        QQSuperCompressionAudioProcessor full,eco;Transport host;host.playing=true;
        for(auto* q:{&full,&eco})
        {
            auto layout=q->getBusesLayout();
            if(c.mono){layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());}
            layout.inputBuses.set(1,juce::AudioChannelSet::stereo());check(q->setBusesLayout(layout),"ECO test buses");
            configure(*q,0,c.core,c.ceiling,c.limiter);
            set(*q,q->soundParameterID("lookaheadMs"),c.look);
            set(*q,q->soundParameterID("keySource"),c.external?1.f:0.f);
            set(*q,q->soundParameterID("keyHpfHz"),79);
            set(*q,"truePeakLimiting",1);set(*q,"ceilingDb",-3);
            for(size_t d=0;d<5;++d)
            {
                set(*q,q->soundParameterID(qqsc::params::ratioIds[d]),float(4+7*d));
                set(*q,q->soundParameterID(qqsc::params::upRatioIds[d]),float(1.1+.2*d));
                set(*q,q->soundParameterID(qqsc::params::downRatioIds[d]),float(5+3*d));
                q->setBoundaryForDomainDb(false,false,int(d),float(-24+3*d));
                q->setBoundaryForDomainDb(true,false,int(d),-60);
                q->setBoundaryForDomainDb(true,true,int(d),float(-18+2*d));
            }
            q->setPlayHead(&host);q->prepareToPlay(c.rate,17);
        }
        full.setEcoMode(false);eco.setEcoMode(true);eco.setEditorOpen(false);
        juce::MidiBuffer midi;const int sizes[]={2048,512,1,198,199,256,17,8192,63,1024};
        for(int b=0;b<110;++b)
        {
            if(b%8==0)
            {
                for(auto* q:{&full,&eco})
                {
                    set(*q,q->soundParameterID("processingMode"),float((b/8)%3));
                    set(*q,q->soundParameterID("compressionMode"),float((b/16)%2));
                    set(*q,q->soundParameterID("algorithmMode"),float((b/8)%2));
                    set(*q,q->soundParameterID("ratioS"),float(4+b));
                    set(*q,q->soundParameterID("mixM"),float(30+b%70));
                }
                ++transitions;
            }
            if(c.limiter && (b==24 || b==36))for(auto* q:{&full,&eco})set(*q,"truePeakLimiting",b==24?0.f:1.f);
            if(b==41)for(auto* q:{&full,&eco})q->copyAToB();
            if(b==42)for(auto* q:{&full,&eco})q->selectABSlot(1);
            if(b==53 || b==55)for(auto* q:{&full,&eco})q->selectABSlot(0);
            if(b==54 || b==56)for(auto* q:{&full,&eco})q->selectABSlot(1);
            if(b==67)eco.setEditorOpen(true);
            if(b==71)eco.setEditorOpen(false);
            if(b==81)eco.setEcoMode(false);
            if(b==85)eco.setEcoMode(true);
            if(b==93)for(auto* q:{&full,&eco})q->setNonRealtime(true);
            const int n=sizes[b%10],channels=c.mono?1:2;
            juce::AudioBuffer<float> a(channels+2,n),z(channels+2,n);
            for(int ch=0;ch<channels+2;++ch)for(int i=0;i<n;++i)
            {
                const double t=double(host.sample+i)/c.rate;
                const auto sample=float((.5+.49*std::sin(2*pi*1.7*t))*std::sin(2*pi*(ch%2?371:997)*t));
                a.setSample(ch,i,sample);
            }
            z.makeCopyOf(a,true);full.processBlock(a,midi);eco.processBlock(z,midi);
            for(int ch=0;ch<channels;++ch)for(int i=0;i<n;++i)
            {
                check(std::isfinite(z.getSample(ch,i)),"ECO nonfinite");
                const double delta=std::abs(a.getSample(ch,i)-z.getSample(ch,i));
                worst=std::max(worst,delta);
                if(delta>2.e-6)std::cout<<"MISMATCH core="<<c.core<<" block="<<b<<" i="<<i<<" delta="<<delta<<"\n";
                check(delta<=2.e-6,"ECO mode/UI/AB transition changes sound");
            }
            if(!eco.shouldRunUiAnalysis())
                check(eco.outputCeiling.gainForDisplayLinear()==1.f && eco.outputCeiling.inputTruePeakForDisplayLinear()==0.f,
                      "hidden ECO still accumulates Ceiling display statistics");
            check(full.getLatencySamples()==eco.getLatencySamples(),"ECO changes PDC");host.sample+=n;
        }
        check(eco.performanceWindows[2].blocks.load()>0 && eco.performanceWindows[3].blocks.load()>0,"missing closed/open diagnostics");
        const auto before=eco.performanceWindows[2].blocks.load();eco.setEditorOpen(true);stream(eco,c.rate,{128},128,c.mono?3:4);
        check(eco.performanceWindows[2].blocks.load()==before,"opening UI erased closed measurements");
        full.setPlayHead(nullptr);eco.setPlayHead(nullptr);
        std::cout<<"ECO_PARITY core="<<c.core<<" ceiling="<<c.ceiling<<" limiter="<<c.limiter<<" look="<<c.look<<" max_delta="<<worst<<"\n";
    }
    {
        QQSuperCompressionAudioProcessor p;configure(p,0,0,0,false);p.setEcoMode(false);Transport host;p.setPlayHead(&host);p.prepareToPlay(48000,512);
        juce::AudioBuffer<float> b(2,512);juce::MidiBuffer midi;
        const auto render=[&](){p.processBlock(b,midi);};
        for(int i=0;i<120;++i){b.clear();render();}
        check(p.performanceWindows[0].idleGate.load()==9,"sleep diagnostic");
        host.playing=true;b.clear();render();check(p.performanceWindows[0].idleGate.load()==2,"playing diagnostic");
        host.available=false;render();check(p.performanceWindows[0].idleGate.load()==1,"unknown transport diagnostic");
        host.available=true;host.playing=false;set(p,"ratio",13);render();check(p.performanceWindows[0].idleGate.load()==3,"parameter wake diagnostic");
        b.clear();b.setSample(0,0,1.e-8f);render();check(p.performanceWindows[0].idleGate.load()==4,"above-floor wake diagnostic");
        p.setPlayHead(nullptr);
    }
    std::cout<<"PASS detector reference; ECO mono/stereo, internal/external SC, dual/single, algorithm smoothing, "<<transitions<<" mode transitions, UI, A/B, offline, PDC; max_delta="<<worst<<"\n";
}
