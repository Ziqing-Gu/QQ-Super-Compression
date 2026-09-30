static void ecoTransport1241()
{
    struct Config{bool limiter;int core,ceil;float look;double rate;bool mono,external;};
    const Config configs[]={{false,0,0,0,48000,false,false},{false,3,3,0,48000,false,false},
        {false,0,0,26,44100,true,true},{true,3,3,0,48000,false,false},
        {true,1,1,0,48000,false,true},{true,2,3,0,96000,true,true},
        {true,3,1,0,48000,false,false},{true,0,3,26,48000,false,false}};
    double worst=0;
    for(const auto& c:configs)
    {
        QQSuperCompressionAudioProcessor processor;Transport host;host.playing=true;
        auto layout=processor.getBusesLayout();
        if(c.mono){layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());}
        layout.inputBuses.set(1,juce::AudioChannelSet::stereo());check(processor.setBusesLayout(layout),"transport test buses");
        configure(processor,0,0,0,c.limiter);
        set(processor,processor.soundParameterID("oversampling"),float(c.core));set(processor,"ceilingOversampling",float(c.ceil));
        set(processor,processor.soundParameterID("lookaheadMs"),c.look);set(processor,processor.soundParameterID("keySource"),c.external?1.f:0.f);
        set(processor,processor.soundParameterID("ratio"),47);set(processor,processor.soundParameterID("thresholdDb"),-18);
        set(processor,processor.soundParameterID("keyHpfHz"),79);set(processor,"truePeakLimiting",1);set(processor,"ceilingDb",-1);
        set(processor,"limiterOutputDb",18);set(processor,"tpRecoveryMode",2);
        processor.setEcoMode(true);processor.setEditorOpen(false);processor.setPlayHead(&host);processor.prepareToPlay(c.rate,1024);
        const int channels=c.mono?1:2;juce::MidiBuffer midi;juce::AudioBuffer<float> audio(channels+2,1024);
        const auto fill=[&](float level){for(int ch=0;ch<audio.getNumChannels();++ch)for(int i=0;i<audio.getNumSamples();++i)audio.setSample(ch,i,level*(i%2?1.f:-1.f));};
        for(int i=0;i<12;++i){fill(.7f);processor.processBlock(audio,midi);host.sample+=1024;}
        host.playing=false;
        for(float level:{1.e-10f,3.16227766e-5f,.7f})for(bool open:{false,true})
        {
            processor.setEditorOpen(open);fill(level);const auto before=processor.detectorSampleCounter;
            processor.processBlock(audio,midi);
            check(processor.ecoTransportSuspended && processor.detectorSampleCounter==before,"stopped ECO runs detector");
            for(int ch=0;ch<channels;++ch)check(audio.getMagnitude(ch,0,audio.getNumSamples())==0.f,"stopped ECO not muted");
            check(processor.performanceWindows[open?3:2].idleGate.load()==10,"ECO stop reason missing");
        }
        processor.setEditorOpen(false);
        for(int wake=0;wake<5;++wake)
        {
            host.available=true;host.playing=false;host.recording=false;processor.setNonRealtime(false);processor.setEcoMode(true);
            audio.setSize(channels+2,1024,false,false,true);fill(.2f);processor.processBlock(audio,midi);
            check(processor.ecoTransportSuspended,"stop before restart");
            set(processor,processor.soundParameterID("inputGainDb"),float(wake-2));
            set(processor,processor.soundParameterID("mix"),float(55+7*wake));
            if(wake==2){processor.copyAToB();set(processor,processor.soundParameterID("ratio"),900);processor.selectABSlot(1);}
            fill(1.e-10f);processor.processBlock(audio,midi);check(processor.ecoTransportSuspended,"edits woke stopped ECO");
            host.playing=wake==0;host.recording=wake==1;host.available=wake!=3;
            processor.setNonRealtime(wake==2);if(wake==4)processor.setEcoMode(false);
            juce::MemoryBlock state;processor.getStateInformation(state);
            QQSuperCompressionAudioProcessor reference;check(reference.setBusesLayout(layout),"reference buses");
            reference.setStateInformation(state.getData(),int(state.getSize()));reference.setEcoMode(false);
            reference.setPlayHead(&host);reference.setNonRealtime(wake==2);reference.prepareToPlay(c.rate,1024);
            const bool bypass=wake==3;double peak=0;
            for(int block=0;block<24;++block)
            {
                const int sizes[]={1,17,1024,256};const int n=sizes[block%4];audio.setSize(channels+2,n,false,false,true);
                for(int ch=0;ch<audio.getNumChannels();++ch)for(int i=0;i<n;++i)
                {
                    float x=block<3 ? 0.f : float(.63*std::sin((host.sample+i)*.13+ch*.7))+1.e-5f;
                    if(block==3 && i==0)x=1.f;audio.setSample(ch,i,x);
                }
                juce::AudioBuffer<float> ref;ref.makeCopyOf(audio,true);
                if(bypass){processor.processBlockBypassed(audio,midi);reference.processBlockBypassed(ref,midi);}
                else{processor.processBlock(audio,midi);reference.processBlock(ref,midi);}
                check(!processor.ecoTransportSuspended,"first callback did not resume");
                check(processor.getLatencySamples()==reference.getLatencySamples(),"resume changed PDC");
                for(int ch=0;ch<channels;++ch)for(int i=0;i<n;++i)
                {
                    const auto x=audio.getSample(ch,i),y=ref.getSample(ch,i);check(std::isfinite(x),"nonfinite resumed audio");
                    worst=std::max(worst,double(std::abs(x-y)));peak=std::max(peak,double(std::abs(x)));
                    check(std::abs(x-y)<=2.e-6f,"resumed output differs from clean reference");
                    if(block<3)check(x==0.f,"pre-stop cached audio leaked");
                }
                host.sample+=n;
            }
            check(peak>1.e-5,"active/record/offline/FULL output muted");reference.setPlayHead(nullptr);
        }
        std::cout<<"ECO_TRANSPORT limiter="<<c.limiter<<" core="<<c.core<<" ceiling="<<c.ceil<<" look="<<c.look<<" rate="<<c.rate<<" max_delta="<<worst<<"\n";
        processor.setPlayHead(nullptr);
    }
    // No transport crossfade may attenuate the first sample, including exports.
    {
        QQSuperCompressionAudioProcessor processor;Transport host;configure(processor,0,0,0,false);
        processor.setEcoMode(true);processor.setPlayHead(&host);processor.prepareToPlay(48000,1);
        juce::AudioBuffer<float> one(2,1);juce::MidiBuffer midi;
        for(bool offline:{false,true})
        {
            host.playing=false;processor.setNonRealtime(false);one.clear();processor.processBlock(one,midi);
            host.playing=!offline;processor.setNonRealtime(offline);one.setSample(0,0,.25f);one.setSample(1,0,-.25f);
            processor.processBlock(one,midi);
            check(one.getSample(0,0)==.25f && one.getSample(1,0)==-.25f,"transport crossfade altered first sample");
        }
        processor.setPlayHead(nullptr);
    }
    // Quantify repeated stopped callbacks including raw diagnostics; leave real
    // Cubase acceptance to the host rather than equating this with its meter.
    QQSuperCompressionAudioProcessor p;Transport host;configure(p,0,0,0,true);
    set(p,p.soundParameterID("oversampling"),3);set(p,"ceilingOversampling",3);set(p,"truePeakLimiting",1);
    p.setPlayHead(&host);p.setEcoMode(true);p.setEditorOpen(false);p.prepareToPlay(48000,1024);
    juce::AudioBuffer<float> b(2,1024);juce::MidiBuffer midi;
    double elapsed=0;
    for(int k=0;k<320;++k)
    {
        for(int ch=0;ch<2;++ch)juce::FloatVectorOperations::fill(b.getWritePointer(ch),1.e-10f,1024);
        const auto start=std::chrono::steady_clock::now();p.processBlock(b,midi);
        if(k>=64)elapsed+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    }
    std::cout<<"ECO_STOP_16x_16x_1024 mean_us="<<elapsed/256<<"\n"<<p.getPerformanceDiagnostics()<<"\n";
    p.setPlayHead(nullptr);
    std::cout<<"PASS ECO transport stop: nonzero main/SC, open/closed, edits, A/B, clean restart, recording, stopped offline, unknown transport, FULL live monitoring, bypass, variable blocks, PDC; max_delta="<<worst<<"\n";
}
