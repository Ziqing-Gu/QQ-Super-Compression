    static void headroomChecks(const juce::File& root)
    {
        using Engine = qqsc::StaticCompressionEngine;
        const auto db=[](float x){return juce::Decibels::gainToDecibels(x,-180.f);};
        const auto gain=[](float x){return juce::Decibels::decibelsToGain(x);};
        const auto put=[](QQSuperCompressionAudioProcessor& p,const char* id,float v)
            {set(p,p.soundParameterID(id),v);};
        const auto expected=[](float amplitude,float threshold,float ratio,int algorithm)
        {
            if(amplitude<=threshold)return amplitude;
            return algorithm==0 ? threshold*std::pow(amplitude/threshold,1.f/ratio)
                : amplitude*(1.f+(ratio-1.f)*threshold)/(1.f+(ratio-1.f)*amplitude);
        };
        struct Worker final : juce::Thread
        { Worker():juce::Thread("headroom replay test"){} void run() override{} } worker;
        juce::String csv="family,algorithm,dual,transparent,lookahead,input_db,output_db,expected_db\n";
        int cases=0,projections=0,replays=0;
        float worstAudio=0,worstDisplay=0;
        for(int family:{0,1,2})for(int algorithm:{0,1})for(int dual:{0,1})
        {
            if(family==1 && dual)continue;
            for(int transparent:{0,1})for(float look:{0.f,25.3f,100.f})for(float inputDb:{-2.f,0.f,2.f,6.f,30.f})
            {
                QQSuperCompressionAudioProcessor p;
                p.setProcessorModeFromEditor(family);p.setEcoMode(false);
                put(p,"limiterLink",0);put(p,"domainLink",0);
                put(p,"processingMode",0);put(p,"compressionMode",float(dual));
                put(p,"algorithmMode",float(algorithm));put(p,"upAlgorithmMode",float(algorithm));
                put(p,"downAlgorithmMode",float(algorithm));
                put(p,"lookaheadMs",look);put(p,"distort",0);put(p,"detectorMode",float(transparent));
                put(p,"oversampling",0);put(p,"keySource",0);put(p,"keyHpfHz",0);
                put(p,"inputGainDb",0);put(p,"outputGainDb",0);put(p,"limiterStereoLink",0);
                set(p,"ceilingDb",0);set(p,"truePeakLimiting",0);
                const float ratio=family==0?200.f:1000.f, thresholdDb=-12.f,threshold=gain(thresholdDb);
                for(size_t d=0;d<5;++d)
                {
                    put(p,qqsc::params::ratioIds[d],ratio);
                    put(p,qqsc::params::downRatioIds[d],ratio);put(p,qqsc::params::upRatioIds[d],1);
                    put(p,qqsc::params::upEnabledIds[d],1);put(p,qqsc::params::downEnabledIds[d],1);
                    p.setBoundaryForDomainDb(false,true,int(d),qqsc::params::rangeOffDb);
                    p.setBoundaryForDomainDb(false,false,int(d),thresholdDb);
                    p.setBoundaryForDomainDb(true,false,int(d),-90);
                    p.setBoundaryForDomainDb(true,true,int(d),thresholdDb);
                }
                for(const char* id:{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"})put(p,id,0);
                for(const char* id:{"mix","mixL","mixR","mixM","mixS"})put(p,id,100);
                p.setDisplayKeyHistoryCaptureEnabled(true);
                p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);
                const float amplitude=gain(inputDb),right=amplitude*.5f;
                juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,256);
                for(int block=0;block<65;++block)
                {
                    for(int i=0;i<256;++i){audio.setSample(0,i,amplitude);audio.setSample(1,i,right);}
                    p.processBlock(audio,midi);
                }
                const float want=expected(amplitude,threshold,ratio,algorithm),actual=audio.getSample(0,255);
                const float error=std::abs(db(actual)-db(want));worstAudio=std::max(worstAudio,error);
                csv+=juce::String(family)+","+juce::String(algorithm)+","+juce::String(dual)+","+juce::String(transparent)
                    +","+juce::String(look)+","+juce::String(inputDb)+","+juce::String(db(actual),6)+","+juce::String(db(want),6)+"\n";
                check(error<.015f,"Actual processor stops increasing reduction above 0 dB");
                const float wantR=family==0?right*want/amplitude:expected(right,threshold,ratio,algorithm);
                check(std::abs(db(audio.getSample(1,255))-db(wantR))<.015f,"Independent R headroom processing");
                // A/B uses a separate cached transfer path: it must obey the same law.
                qqsc::ABTransfer transfer;
                transfer.mode=family==0?0:2;transfer.dual=dual!=0;
                transfer.algorithm=transfer.upAlgorithm=transfer.downAlgorithm=algorithm==0
                    ? qqsc::CompressionAlgorithm::classic:qqsc::CompressionAlgorithm::super;
                transfer.lower.fill(dual?gain(-90):threshold);
                transfer.upper.fill(dual?threshold:std::numeric_limits<float>::infinity());
                transfer.ratio.fill(ratio);transfer.downRatio.fill(ratio);transfer.upRatio.fill(1);
                transfer.makeup.fill(1);transfer.mix.fill(1);
                const auto matrix=transfer.evaluate({amplitude,amplitude,right,.75f*amplitude,.25f*amplitude},false,true);
                check(std::abs(db(matrix[0]*amplitude+matrix[1]*right)-db(want))<.015f,"A/B transfer headroom");
                ++cases;
                if(inputDb>0 && look==100.f)
                {
                    DynamicDisplay display(p);display.stopTimer();display.setSize(1000,500);display.clearHistories();
                    const int mode=family==0?0:2;
                    for(int channel=0;channel<(family==0?1:2);++channel)
                    {
                        DynamicDisplay::HistoryPoint point;
                        point.inputDb=db(channel==0?amplitude:right);
                        point.detectorDb=point.inputDb;
                        point.capturedInputGainDb=0;point.capturedKeyGainDb=0;
                        point.captureCounter=100;point.captureGeneration=1;
                        display.pushHistory(display.histories[size_t(channel)],point);
                    }
                    display.refreshRenderCaches(mode);
                    const auto projected=display.renderCaches[0].projected.output[0];
                    worstDisplay=std::max(worstDisplay,std::abs(projected-db(want)));
                    if(std::abs(projected-db(want))>=.02f)
                        std::cerr<<"Display mismatch family="<<family<<" algorithm="<<algorithm<<" dual="<<dual
                                 <<" input="<<inputDb<<" actual="<<projected<<" expected="<<db(want)
                                 <<" points="<<display.renderCaches[0].projected.size
                                 <<" lut="<<display.dynamicsGainFromLut(display.renderCaches[0],inputDb)<<"\n";
                    check(std::abs(projected-db(want))<.02f,"Display projects positive input using a capped detector");
                    for(float probeDb:{.01f,2.f,30.f,60.f,72.f})
                    {
                        const float g=display.dynamicsGainFromLut(display.renderCaches[0],probeDb);
                        const float target=expected(gain(probeDb),threshold,ratio,algorithm)/gain(probeDb);
                        check(std::abs(db(g)-db(target))<.02f,"Positive LUT/fallback retains a hidden ceiling");
                    }
                    ++projections;
                    // Replay real retained samples, including full Window and Transparent.
                    DynamicDisplay::ReplayRequest request;
                    const auto& meters=p.getMeterState();
                    request.requestGeneration=display.replayRequestGeneration->load();
                    request.captureGeneration=meters.displayCaptureGeneration.load();
                    request.requestedStartCounter=0;request.mode=mode;
                    request.lookaheadMs=look;request.detectorWindowMs=look;request.detectorMode=transparent;
                    request.limiter=family==2;request.clipper=family==1;
                    request.markers={meters.displayCaptureCounter.load()};
                    request.carrierDelays={p.getDisplayCoreDelaySeconds()};request.captureBlockSamples={256};
                    DynamicDisplay::ReplayResult replay;
                    check(display.buildHpfReplay(request,replay,worker),"Headroom replay unavailable");
                    check(std::abs(replay.detectorDb0[0]-inputDb)<.01f,"Replay detector lost positive-dB evidence");
                    ++replays;
                }
                p.releaseResources();
            }
        }
        // Finite Range and upward gates remain effective, including finite 0 dB.
        for(auto algorithm:{qqsc::CompressionAlgorithm::classic,qqsc::CompressionAlgorithm::super})
            for(float ratio:{.125f,8.f,200.f})for(float amplitude:{1.f,2.f,32.f})
            {
                check(Engine::singleGainForLevel(amplitude,ratio,gain(-20),1.f,algorithm)==1.f,"Finite Range bypass changed");
                check(Engine::singleGainForLevel(amplitude,ratio,1.f,1.f,algorithm)==1.f,"Equal boundaries changed");
                if(ratio<1)check(Engine::singleGainForLevel(amplitude,ratio,gain(-20),INFINITY,algorithm)==1.f,"Upward anchor changed");
            }
        root.getChildFile("headroom-audio.csv").replaceWithText(csv);
        std::cout<<"PASS: "<<cases<<" real audio/A-B headroom cases, "<<projections<<" Display cases, "<<replays
                 <<" real history replays; finite Range/upward boundaries preserved. Max audio error="<<worstAudio
                 <<" dB, display error="<<worstDisplay<<" dB\n";
    }
