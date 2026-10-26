    static void inputDisplayChecks(const juce::File& root)
    {
        int cases=0;
        for(int family:{0,1,2})for(int mode:{0,1,2})
        {
            if(family!=0 && mode!=2)continue;
            QQSuperCompressionAudioProcessor p;p.setProcessorModeFromEditor(family);
            set(p,"ceilingEnabled",0);
            auto put=[&](const char* id,float v){set(p,p.soundParameterID(id),v);};
            put("processingMode",float(mode));put("keySource",0);put("algorithmMode",0);
            put("compressionMode",0);put("limiterLink",0);put("inputOutputLink",0);
            put("limiterStereoLink",0);put("outputGainDb",4);
            for(const auto* id:qqsc::params::ratioIds)put(id,family==0?4.f:10.f);
            for(int d=0;d<5;++d){p.setBoundaryForDomainDb(false,true,d,qqsc::params::rangeOffDb);p.setBoundaryForDomainDb(false,false,d,-20);}
            for(const auto* id:{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"})put(id,3);
            for(const auto* id:{"mix","mixL","mixR","mixM","mixS"})put(id,75);
            QQSuperCompressionAudioProcessorEditor e(p,settings(root));e.stopTimer();e.display.stopTimer();
            auto& display=e.display;display.setSize(1000,500);display.clearHistories();
            for(int ch=0;ch<2;++ch)
            {
                DynamicDisplay::HistoryPoint point;point.inputDb=-36;point.detectorDb=-12;
                point.capturedInputGainDb=0;point.externalKeyDb=-17;point.capturedKeyGainDb=0;
                point.captureCounter=100;point.captureGeneration=1;display.pushHistory(display.histories[size_t(ch)],point);
            }
            for(float input:{-12.06f,0.f,6.f})for(bool external:{false,true})for(bool replay:{false,true})
            {
                put("inputGainDb",input);put("keySource",external?1.f:0.f);
                for(auto& h:display.histories)
                {
                    h.points[0].hasReplayedDetector=replay;
                    h.points[0].replayedDetectorDb=-12.f;
                    h.points[0].detectorDb=-12.f;
                }
                display.refreshRenderCaches(mode);
                const int domain=mode==0?0:mode==2?1:3;
                const auto plot=display.getBoundaryPlotForDomain(domain);
                check(std::abs(display.getBoundaryYForDomainDb(domain,-20)-display.dbToY(-20,plot))<.001f,
                      "Input must not move threshold line away from its fixed dB coordinate");
                check(std::abs(display.getBoundaryDbForY(domain,display.dbToY(-20,plot))+20)<.001f,"Threshold inverse mapping includes Input");
                const auto& v=display.renderCaches[0].projected;
                check(v.size==1,"Input Display missing retained point");
                check(std::abs(v.input[0]-(-36))<.002f,"Retired Input cannot move the pre-compression trace");
                const float detector=-12;
                const float gain=p.getDynamicsGainForDomain(juce::Decibels::decibelsToGain(detector),domain);
                const float mixed=.25f+.75f*gain;
                const float wet=.25f+.75f*gain*juce::Decibels::decibelsToGain(3.f);
                const float blue=-36+juce::Decibels::gainToDecibels(mixed);
                const float output=-36+juce::Decibels::gainToDecibels(wet)+4;
                if(std::abs(v.gainReductionBoundary[0]-blue)>=.025f)
                    std::cerr<<"Blue mismatch family="<<family<<" mode="<<mode<<" input="<<input<<" external="<<external<<" replay="<<replay<<" got="<<v.gainReductionBoundary[0]<<" expected="<<blue<<" gain="<<gain<<" detector="<<detector<<"\n";
                check(std::abs(v.gainReductionBoundary[0]-blue)<.025f,"Blue trace has wrong Input reference");
                check(std::abs(v.output[0]-output)<.025f,"Output includes Input more than once");
                check(std::abs(v.externalKey[0]+17)<.002f,"Input moved external sidechain waveform");
                ++cases;
            }
            check(display.getLoudnessReadoutBounds().isEmpty()==(family!=2),"LUFS box must only appear in Limiter");
            if(family==1){put("inputGainDb",-12.06f);sync(p,e);image(e,root.getChildFile("clipper-input.png"));}
        }
        std::cout<<"PASS: "<<cases<<" Input Display / fixed threshold / EXT / retained replay cases.\n";
    }

    static void inputMonitorChecks(const juce::File& root)
    {
        int cases=0;float worst=0;juce::String csv="ceiling,ratio,makeup,mix,input,peak,reference_error\n";
        for(bool ceiling:{false,true})for(float ratio:{1.f,4.f,1000.f})for(float makeup:{0.f,6.f})for(float mix:{0.f,50.f,100.f})
        {
            float unityPeak=0;
            for(float input:{0.f,-6.f})
            {
                QQSuperCompressionAudioProcessor p,reference;
                for(auto* x:{&p,&reference})
                {
                    x->enterLimiterMode();x->setEcoMode(false);x->setUnityMonitorEnabled(true);
                    auto put=[&](const char* id,float v){set(*x,x->soundParameterID(id),v);};
                    put("limiterLink",0);put("lookaheadMs",25.3f);put("oversampling",0);put("keySource",0);put("keyHpfHz",0);
                    put("compressionMode",0);put("algorithmMode",0);put("ratioL",ratio);put("inputGainDb",x==&p?input:0.f);
                    put("outputGainDb",12);put("makeupGainLDb",makeup);put("mixL",mix);put("limiterStereoLink",0);
                    x->setBoundaryForDomainDb(false,false,1,-24);
                    set(*x,"ceilingEnabled",ceiling?1.f:0.f);set(*x,"truePeakLimiting",1);set(*x,"ceilingDb",-1);
                    x->setRateAndBufferSizeDetails(48000,256);x->prepareToPlay(48000,256);
                }
                juce::MidiBuffer midi;juce::AudioBuffer<float> a(2,256),b(2,256);
                float error=0,peak=0;const float inputGain=1.f;
                for(int block=0;block<240;++block)
                {
                    for(int i=0;i<256;++i)
                    {float t=float(block*256+i);float left=.25f*std::sin(t*.057f),right=.17f*std::sin(t*.083f);
                     a.setSample(0,i,left);a.setSample(1,i,right);b.setSample(0,i,left*inputGain);b.setSample(1,i,right*inputGain);}
                    if(block%17==0)
                    {set(p,p.soundParameterID("inputGainDb"),(block%34==0)?-12.f:12.f);p.setDisplayAdjustDb(float(block%37)-18.f);}
                    p.processBlock(a,midi);reference.processBlock(b,midi);
                    if(block>100)for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)
                    {error=std::max(error,std::abs(a.getSample(ch,i)-b.getSample(ch,i)/inputGain));peak=std::max(peak,std::abs(a.getSample(ch,i)));}
                }
                // Retired Input automation and display adjustment must leave audio identical.
                check(error<.00001f,"Adjust and retired Input automation must not change real monitor samples");
                if(!ceiling && (ratio==1 || mix==0))
                {
                    if(input==0)unityPeak=peak;
                    else check(std::abs(juce::Decibels::gainToDecibels(peak/unityPeak))<.002f,"Unity monitor must restore pre-Input level at ratio 1:1 / all Dry");
                }
                worst=std::max(worst,error);++cases;
                csv+=juce::String(int(ceiling))+","+juce::String(ratio)+","+juce::String(makeup)+","+juce::String(mix)+","+juce::String(input)+","+juce::String(peak,7)+","+juce::String(error,9)+"\n";
                p.releaseResources();reference.releaseResources();
            }
        }
        root.getChildFile("input-monitor.csv").replaceWithText(csv);
        std::cout<<"PASS: "<<cases<<" actual monitor cases: Adjust and retired Input are audio-invariant; max error="<<worst<<"\n";
    }

    static void inputMatchChecks(const juce::File& root)
    {
        struct Head final:juce::AudioPlayHead
        { int64_t sample=0;juce::Optional<PositionInfo> getPosition() const override
            {PositionInfo p;p.setIsPlaying(true);p.setTimeInSamples(sample);return p;} };
        int cases=0;juce::String csv="family,mode,unity,input,output,makeup\n";
        for(int family:{0,1,2})for(int mode:{0,1,2})
        {
            if(family!=0 && mode!=2)continue;
            float reference=0;bool first=true;
            for(bool unity:{false,true})for(float input:{0.f,-6.f,6.f})for(float output:{-12.f,18.f})
            {
                QQSuperCompressionAudioProcessor p;Head head;p.setProcessorModeFromEditor(family);p.setEcoMode(false);
                auto put=[&](const char* id,float v){set(p,p.soundParameterID(id),v);};
                put("processingMode",float(mode));put("compressionMode",0);put("lookaheadMs",25.3f);put("oversampling",0);
                put("algorithmMode",0);put("keySource",0);put("keyHpfHz",0);put("limiterLink",0);put("inputOutputLink",0);
                put("inputGainDb",input);put("outputGainDb",output);put("limiterStereoLink",0);
                for(const auto* id:qqsc::params::ratioIds)put(id,family==1?10.f:4.f);
                for(int d=0;d<5;++d){p.setBoundaryForDomainDb(false,true,d,qqsc::params::rangeOffDb);p.setBoundaryForDomainDb(false,false,d,-30);}
                for(const auto* id:{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"})put(id,0);
                for(const auto* id:{"mix","mixL","mixR","mixM","mixS"})put(id,100);
                set(p,"ceilingDb",-6);set(p,"truePeakLimiting",1);p.setUnityMonitorEnabled(unity);
                p.setPlayHead(&head);p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);
                juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,256);
                // Same original source: Adjust and retired Input cannot affect Match.
                const float source=.15f;
                p.setDisplayAdjustDb(input*3.f);
                for(int b=0;b<600;++b)
                {
                    for(int i=0;i<256;++i){float t=float(head.sample+i);audio.setSample(0,i,source*std::sin(t*.071f));audio.setSample(1,i,source*.67f*std::sin(t*.113f));}
                    p.processBlock(audio,midi);head.sample+=256;p.refreshMatchResults();
                }
                check(p.hasMatchData() && p.applyMatchForCurrentMode(),"Pre-Input Match unavailable");
                const auto id=family!=0||mode==2?"makeupGainLDb":mode==1?"makeupGainMDb":"makeupGainDb";
                const float target=p.readSoundParameter(id);
                if(first){reference=target;first=false;}
                const float expected=reference;
                if(std::abs(target-expected)>=.025f)std::cerr<<"Match mismatch family="<<family<<" mode="<<mode<<" unity="<<unity<<" input="<<input<<" output="<<output<<" got="<<target<<" ref="<<expected<<"\n";
                check(std::abs(target-expected)<.025f,"Match must use original input, independent of Adjust and retired Input");
                check(p.applyMatchForCurrentMode() && std::abs(p.readSoundParameter(id)-target)<.001f,"Repeated Match added compensation");
                check(std::abs(p.readSoundParameter("outputGainDb")-output)<.001f,"Unlinked Match altered Output Gain");
                csv+=juce::String(family)+","+juce::String(mode)+","+juce::String(int(unity))+","+juce::String(input)+","+juce::String(output)+","+juce::String(target,6)+"\n";
                ++cases;p.setPlayHead(nullptr);p.releaseResources();
            }
        }
        root.getChildFile("match-reference.csv").replaceWithText(csv);
        std::cout<<"PASS: "<<cases<<" real-audio Match reference / Output independence / monitor cases.\n";
    }
