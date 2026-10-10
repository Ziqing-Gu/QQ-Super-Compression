    static void cumulativeMatchChecks()
    {
        struct Head final:juce::AudioPlayHead
        {
            int64_t sample=0;bool playing=true;
            juce::Optional<PositionInfo> getPosition() const override
            {PositionInfo p;p.setIsPlaying(playing);p.setTimeInSamples(sample);return p;}
        };
        const auto require=[](bool ok,const char* text){if(!ok)throw std::runtime_error(text);};
        for(int kind:{0,1,2,3,4,5})
        {
            const bool limiter=kind>=3,unity=kind>=4;
            QQSuperCompressionAudioProcessor p,reference;Head hp,hr;
            const auto set=[](auto& x,const char* id,float value){auto* a=x.getAPVTS().getParameter(id);if(!a)throw std::runtime_error(id);a->setValueNotifyingHost(a->convertTo0to1(value));};
            for(auto* x:{&p,&reference})
            {
                x->setEcoMode(false);if(limiter)x->enterLimiterMode();
                set(*x,x->soundParameterID("processingMode"),float(limiter?2:kind));
                set(*x,x->soundParameterID("lookaheadMs"),26);set(*x,x->soundParameterID("oversampling"),0);
                set(*x,x->soundParameterID("algorithmMode"),0);set(*x,"limiterLink",kind==5?1.f:0.f);set(*x,"truePeakLimiting",0);set(*x,"ceilingDb",0);
                for(int d=0;d<5;++d){set(*x,x->soundParameterID(qqsc::params::ratioIds[size_t(d)]),4);set(*x,x->soundParameterID(qqsc::params::thresholdIds[size_t(d)]),-20);}
                x->setUnityMonitorEnabled(unity);x->setRateAndBufferSizeDetails(48000,257);x->prepareToPlay(48000,257);
            }
            p.setPlayHead(&hp);reference.setPlayHead(&hr);juce::MidiBuffer midi;
            const auto run=[&](int samples,float amplitude)
            {
                for(int n=0;n<samples;n+=257)
                {
                    int count=std::min(257,samples-n);juce::AudioBuffer<float> a(2,count),b(2,count);
                    for(int i=0;i<count;++i){float time=float(hp.sample+i);a.setSample(0,i,amplitude*std::sin(time*.071f));a.setSample(1,i,amplitude*.67f*std::sin(time*.113f));}
                    b.makeCopyOf(a);p.processBlock(a,midi);reference.processBlock(b,midi);hp.sample+=count;hr.sample+=count;
                    p.refreshMatchResults();reference.refreshMatchResults();
                }
            };
            run(48000*2,.24f);
            require(p.hasMatchData(),"Match unavailable during active playback");
            const auto firstCount=p.loudnessMatch.getBlockCount();require(firstCount==17,"Capture omitted start of playback");const auto firstGeneration=p.matchGeneration.load();
            require(p.applyMatchForCurrentMode(),"First active-playback Match failed");
            require(p.loudnessMatch.getBlockCount()==firstCount && p.matchGeneration.load()==firstGeneration
                && !p.resetMatchOnNextPlaybackBlock.load(),"Match consumed or scheduled reset of capture");
            const auto id=limiter?"makeupGainLDb":kind==0?"makeupGainDb":kind==1?"makeupGainMDb":"makeupGainLDb";
            float first=p.readSoundParameter(id);
            require(p.applyMatchForCurrentMode() && std::abs(p.readSoundParameter(id)-first)<.001f,"Immediate second click adds compensation");
            if(unity)set(p,"limiterMakeupGainLDb",first+1.f);
            run(48000*2,.07f);
            require(p.loudnessMatch.getBlockCount()>firstCount && p.matchGeneration.load()==firstGeneration,"Early playback was discarded after Match");
            require(reference.applyMatchForCurrentMode() && p.applyMatchForCurrentMode(),"Second cumulative Match unavailable");
            float target=p.readSoundParameter(id),expected=reference.readSoundParameter(id);
            std::cout<<"CUMULATIVE_MATCH kind="<<kind<<" blocks="<<p.loudnessMatch.getBlockCount()<<" target="<<target<<" uninterrupted="<<expected<<"\n";
            require(std::abs(target-expected)<.025f,"Repeated Match disagrees with uninterrupted full-pass reference");
            require(p.applyMatchForCurrentMode() && std::abs(p.readSoundParameter(id)-target)<.001f,"Retained result applied twice");
            // A loop within an uninterrupted playback does not create a new pass.
            hp.sample-=24000;hr.sample-=24000;run(4800,.07f);
            require(p.matchGeneration.load()==firstGeneration,"Seek/loop reset cumulative pass");
            hp.playing=hr.playing=false;run(257,0);
            require(p.hasMatchData(),"Stop discarded completed capture");
            hp.playing=hr.playing=true;run(28800,.07f);
            require(p.matchGeneration.load()!=firstGeneration && p.loudnessMatch.getBlockCount()<firstCount,"New playback did not start a new capture");
            if(kind==0)
            {
                // ECO must not offer a partial UI-open segment as a full pass.
                p.setEditorOpen(true);p.setEcoMode(true);run(4800,.07f);
                p.setEditorOpen(false);run(4800,.07f);
                p.setEditorOpen(true);run(48000,.07f);
                require(!p.hasMatchData(),"ECO skipped capture presented as complete playback");
                hp.playing=hr.playing=false;run(257,0);
                hp.playing=hr.playing=true;run(48000,.07f);
                require(p.hasMatchData(),"ECO complete new pass remained invalid");
            }
            p.setPlayHead(nullptr);reference.setPlayHead(nullptr);
        }
        std::cout<<"PASS: cumulative Match during playback, repeat-click idempotence, full-pass reference, loops, stop retention and restart across Normal ST/MS/LR and Limiter ordinary/unity including Output LINK; ECO incomplete-pass guard.\n";
    }
