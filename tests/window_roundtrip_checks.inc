    static void windowRoundtripChecks(const juce::File&)
    {
        struct Worker final:juce::Thread { Worker():Thread("roundtrip oracle"){} void run() override{} } worker;
        float worst=0, rawNativeWorst=0, audioError=0; int count=0;
        for(int family:{0,1,2})for(int cadence:{1,3})for(int os:{0,1})for(double rate:{48000.,96000.})
        {
            QQSuperCompressionAudioProcessor p,reference;
            for(auto* x:{&p,&reference})
            {
                x->setEcoMode(false);x->setEditorOpen(true);
                if(family==1)x->enterClipperMode();if(family==2)x->enterLimiterMode();
                value(*x,"algorithmMode",1);value(*x,"ratio",1000);value(*x,"ratioL",1000);
                value(*x,"thresholdDb",-16);value(*x,"thresholdLDb",-16);value(*x,"rangeDb",1);
                value(*x,"lookaheadMs",26);value(*x,"detectorWindowMs",0);value(*x,"oversampling",float(os));
                x->setRateAndBufferSizeDetails(rate,257);x->prepareToPlay(rate,257);
            }
            DynamicDisplay d(p);d.stopTimer();d.setSize(1000,530);
            juce::MidiBuffer midi;
            for(int b=0;b<160;++b)
            {
                juce::AudioBuffer<float> a(2,257),r(2,257);
                for(int i=0;i<257;++i)for(int ch=0;ch<2;++ch)
                {const int n=b*257+i; const float x=signal(n,rate,ch,false)*std::sin(float(n)*.213f);a.setSample(ch,i,x);}
                r.makeCopyOf(a);p.processBlock(a,midi);reference.processBlock(r,midi);
                for(int ch=0;ch<2;++ch)for(int i=0;i<257;++i)audioError=std::max(audioError,std::abs(a.getSample(ch,i)-r.getSample(ch,i)));
                if(b%cadence==0)d.timerCallback();
            }
            ++*d.replayRequestGeneration;d.hpfReplayBusy=false;
            DynamicDisplay::ReplayRequest request;
            request.requestGeneration=d.replayRequestGeneration->load();
            request.captureGeneration=p.getDisplayKeyHistoryPosition().generation;
            request.mode=d.lastMode;request.limiter=family!=0;request.clipper=family==1;
            request.lookaheadMs=26;request.detectorWindowMs=0;
            request.oversampling=os;
            for(const auto& pt:d.histories[0].points)
            {request.markers.push_back(pt.captureCounter);request.carrierDelays.push_back(pt.carrierDelaySeconds);request.captureBlockSamples.push_back(pt.captureBlockSamples);}
            const auto original=d.histories;
            const auto oldCount=d.histories[0].points.size();
            ++p.getMeterState().displayCaptureRevision;d.timerCallback();
            require(d.histories[0].points.size()==oldCount,"Torn audio-block publication was captured");
            ++p.getMeterState().displayCaptureRevision;
            for(float window:{10.f,0.f})
            {
                value(p,"detectorWindowMs",window);request.detectorWindowMs=window;
                DynamicDisplay::ReplayResult result;
                require(d.buildHpfReplay(request,result,worker),"Window roundtrip replay");
                if(window==0 && os==0 && rate==48000)for(size_t ch=0;ch<(family?2u:1u);++ch)
                {
                    const auto& values=ch?result.detectorDb1:result.detectorDb0;
                    for(size_t i=1;i<original[ch].points.size();++i)
                        if(std::isfinite(values[i]))rawNativeWorst=std::max(rawNativeWorst,std::abs(values[i]-original[ch].points[i].detectorDb));
                }
                d.applyHpfReplay(std::move(result));
                if(window==0)for(size_t ch=0;ch<(family?2u:1u);++ch)
                    for(size_t i=1;i<original[ch].points.size();++i)
                    {const auto& pt=d.histories[ch].points[i];const auto actual=pt.hasReplayedDetector?pt.replayedDetectorDb:pt.detectorDb;
                     worst=std::max(worst,std::abs(actual-original[ch].points[i].detectorDb));}
            }
            ++count;
        }
        std::cout<<"Window roundtrip cases="<<count<<" max restored detector difference="<<worst<<" dB; native raw-replay difference="<<rawNativeWorst<<" dB; audio delta="<<audioError<<std::endl;
        require(rawNativeWorst<.003f,"Native replay uses the wrong audio-block interval");
        require(worst<.003f,"Returning Window to zero changed the captured detector evidence");
        require(audioError==0,"Display capture changed audio");
    }
