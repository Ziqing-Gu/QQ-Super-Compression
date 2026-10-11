static void replayReferenceChecks(QQSuperCompressionAudioProcessorEditor& e,
                                       QQSuperCompressionAudioProcessor& p)
{
    struct Worker final : juce::Thread { Worker():juce::Thread("test-only replay"){} void run() override{} } worker;
    auto& display=e.display;
    constexpr int blockSize=257, blocks=32;
    int cases=0;
    for(int look:{0,26})for(float window:{0.f,8.f,26.f})for(int detector:{0,1})
    {
        p.releaseResources();p.setEcoMode(false);p.setEditorOpen(true);
        set(p,"processingMode",0);set(p,"keySource",0);
        set(p,"inputGainDb",0);set(p,"keyGainDb",0);set(p,"keyHpfHz",qqsc::params::keyHpfOffHz);
        set(p,"lookaheadMs",float(look));set(p,"detectorWindowMs",window);set(p,"oversampling",0);
        set(p,"detectorMode",float(detector));
        p.setRateAndBufferSizeDetails(48000,blockSize);p.prepareToPlay(48000,blockSize);
        display.clearHistories();
        DynamicDisplay::ReplayRequest request;
        request.requestGeneration=display.replayRequestGeneration->load();
        request.lookaheadMs=float(look);request.detectorWindowMs=std::min(float(look),window);
        request.detectorMode=request.detectorWindowMs>0?detector:0;
        request.mode=0;request.keySource=0;request.hpfHz=qqsc::params::keyHpfOffHz;
        const int n=look*48,w=int(std::round(request.detectorWindowMs*48));
        juce::MidiBuffer midi;std::vector<float> input;
        for(int block=0;block<blocks;++block)
        {
            juce::AudioBuffer<float> audio(2,blockSize);
            for(int i=0;i<blockSize;++i)
            {
                // Peak is inside the interval, while the marker sample is zero.
                const float x=i==blockSize-1?0.f:float(.08+.003*(block%9));input.push_back(x);
                audio.setSample(0,i,x);audio.setSample(1,i,x);
            }
            p.processBlock(audio,midi);
            const auto pos=p.getDisplayKeyHistoryPosition();
            request.captureGeneration=pos.generation;request.markers.push_back(pos.counter);
        }
        DynamicDisplay::ReplayResult result;
        if(!display.buildHpfReplay(request,result,worker))throw std::runtime_error("WINDOW replay failed");
        for(int block=0;block<blocks;++block)
        {
            float peak=0;
            for(int i=block*blockSize;i<(block+1)*blockSize;++i)
            {
                const int t=i-n;float past=0,future=0;
                for(int j=std::max(0,t-w);j<=t && j<int(input.size());++j)past=std::max(past,input[size_t(j)]);
                for(int j=std::max(0,t);j<=t+w && j<int(input.size());++j)future=std::max(future,input[size_t(j)]);
                peak=std::max(peak,detector?std::max(past,future):std::min(past,future));
            }
            const float expected=juce::Decibels::gainToDecibels(peak,-120.f);
            if(std::abs(result.detectorDb0[size_t(block)]-expected)>.002f)
                throw std::runtime_error("Replay is not aligned interval-peak detection");
        }
        // Reproject the SAME retained carrier after switching Lookahead.
        // A pure delay change must not move the matching detection interval.
        const auto retained=request;
        for(float newLook:{0.f,26.f,100.f})
        {
            request=retained;request.lookaheadMs=newLook;request.detectorWindowMs=0;
            request.detectorMode=0;request.carrierDelays.assign(request.markers.size(),float(look)*.001f);
            if(!display.buildHpfReplay(request,result,worker))throw std::runtime_error("Lookahead replay alignment failed");
            for(int block=0;block<blocks;++block)
            {
                float peak=0;
                for(int i=block*blockSize;i<(block+1)*blockSize;++i)
                {const int t=i-n;if(t>=0 && t<int(input.size()))peak=std::max(peak,input[size_t(t)]);}
                if(!std::isfinite(result.detectorDb0[size_t(block)]) ||
                    std::abs(result.detectorDb0[size_t(block)]-juce::Decibels::gainToDecibels(peak,-120.f))>.002f)
                    throw std::runtime_error("Lookahead change displaced retained Display carrier");
            }
        }
        ++cases;
    }
    std::cout<<"PASS: "<<cases<<" WINDOW/0 ms interval-peak oracles and "<<cases*3<<" retained-carrier Lookahead alignment cases.\n";
}
