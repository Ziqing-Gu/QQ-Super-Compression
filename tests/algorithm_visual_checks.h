void QQSCVisualCheck::windowReplayCheck(QQSuperCompressionAudioProcessorEditor& e,
                                       QQSuperCompressionAudioProcessor& p)
{
    struct Worker final : juce::Thread { Worker():juce::Thread("test-only replay"){} void run() override{} } worker;
    auto& display=e.display;
    constexpr int blockSize=257, blocks=32;
    int cases=0;
    for(int look:{0,26})for(float window:{0.f,8.f,26.f})for(int detector:{0,1})
    {
        p.releaseResources();p.setEcoMode(false);p.setEditorOpen(true);
        parameter(p,"processingMode",0);parameter(p,"keySource",0);
        parameter(p,"inputGainDb",0);parameter(p,"keyGainDb",0);parameter(p,"keyHpfHz",qqsc::params::keyHpfOffHz);
        parameter(p,"lookaheadMs",float(look));parameter(p,"detectorWindowMs",window);parameter(p,"oversampling",0);
        parameter(p,"detectorMode",float(detector));
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

void QQSCVisualCheck::zeroLookaheadDetectorCheck(QQSuperCompressionAudioProcessorEditor& e,
                                               QQSuperCompressionAudioProcessor& p)
{
    auto& display = e.display;
    juce::MidiBuffer midi;
    constexpr int blockSize = 257;
    float worstAudioDelta = 0.0f;
    for (int os : {0, 1, 2, 3}) for (int mode : {0, 1, 2})
    {
        QQSuperCompressionAudioProcessor reference;
        for (auto* processor : { &p, &reference })
        {
            processor->releaseResources();
            processor->setEcoMode (false);
            processor->setEditorOpen (true);
            parameter (*processor, "processingMode", float (mode));
            parameter (*processor, "compressionMode", 0);
            parameter (*processor, "algorithmMode", 1);
            parameter (*processor, "inputGainDb", 0);
            parameter (*processor, "outputGainDb", 0);
            parameter (*processor, "keyHpfHz", qqsc::params::keyHpfOffHz);
            parameter (*processor, "lookaheadMs", 0);
            parameter (*processor, "oversampling", float (os));
            parameter (*processor, "detectorMode", 0);
            for (int domain = 0; domain < 5; ++domain)
            {
                parameter (*processor, qqsc::params::ratioIds[size_t(domain)], 5.38f);
                processor->setBoundaryForDomainDb (false, false, domain, qqsc::params::thresholdOffDb);
                processor->setBoundaryForDomainDb (false, true, domain, qqsc::params::rangeOffDb);
            }
            processor->setRateAndBufferSizeDetails (48000, blockSize);
            processor->prepareToPlay (48000, blockSize);
        }
        display.clearHistories();
        display.lastObservedLookaheadMs = 0;
        display.lastObservedDetectorWindowMs = 0;
        display.lastObservedDetectorMode = 0;
        display.lastObservedOversampling = os;
        display.lastObservedHpfHz = qqsc::params::keyHpfOffHz;
        display.hpfRefreshPending = false;
        display.timerCallback();
        const auto initialRequest = display.replayRequestGeneration->load();
        for (int block = 0; block < 64; ++block)
        {
            if (block >= 32 && block % 8 == 0)
            {
                parameter (p, "detectorMode", float ((block / 8) % 2));
                parameter (p, "detectorWindowMs", float(block)); // ineffective at 0 ms
            }
            juce::AudioBuffer<float> audio (2, blockSize), expected (2, blockSize);
            for (int i = 0; i < blockSize; ++i)
            {
                const double t = double (block * blockSize + i) / 48000.0;
                const float left = float (.31 * std::sin (t * 2711.0) + .12 * std::sin (t * 5157.0));
                const float right = float (.27 * std::sin (t * 2873.0 + .3) + .09 * std::sin (t * 4981.0));
                audio.setSample (0, i, left); audio.setSample (1, i, right);
            }
            expected.makeCopyOf (audio);
            p.processBlock (audio, midi); reference.processBlock (expected, midi);
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < blockSize; ++i)
                worstAudioDelta = juce::jmax (worstAudioDelta, std::abs (audio.getSample(ch,i) - expected.getSample(ch,i)));
            for (int tick = 0; tick < 4; ++tick) display.timerCallback();
        }
        const auto extraRequests = display.replayRequestGeneration->load() - initialRequest;
        std::cout << "ZERO_DETECTOR os=" << os << " mode=" << mode
                  << " audio_delta=" << worstAudioDelta << " redundant_replays=" << extraRequests << '\n';
        if (worstAudioDelta != 0.0f) throw std::runtime_error ("0 ms detector switching changed audio");
        if (extraRequests != 0) throw std::runtime_error ("0 ms detector switching unnecessarily rebuilt Display history");

        // No new audio: toggling must preserve every already-visible point.
        display.refreshRenderCaches (mode);
        const auto before0 = display.renderCaches[0].projected;
        const auto before1 = display.renderCaches[1].projected;
        for (int detector : {0, 1, 0, 1})
        {
            parameter (p, "detectorMode", float(detector));
            for (int tick = 0; tick < 8; ++tick) display.timerCallback();
            display.refreshRenderCaches (mode);
            for (int domain = 0; domain < (mode == 0 ? 1 : 2); ++domain)
            {
                const auto& before = domain == 0 ? before0 : before1;
                const auto& after = display.renderCaches[size_t(domain)].projected;
                if (before.size != after.size) throw std::runtime_error ("0 ms switch changed history length");
                for (size_t i = 0; i < before.size; ++i)
                    if (before.input[i] != after.input[i] || before.output[i] != after.output[i]
                        || before.gainReductionBoundary[i] != after.gainReductionBoundary[i])
                        throw std::runtime_error ("0 ms switch changed a retained Display curve");
            }
        }
        if (display.replayRequestGeneration->load() != initialRequest)
            throw std::runtime_error ("Stopped 0 ms switching queued a replay");
        reference.releaseResources();
    }
    // With real lookahead, the detector choice still has to re-project history.
    parameter (p, "lookaheadMs", 26);
    parameter (p, "detectorWindowMs", 100);
    for (int tick = 0; tick < 8; ++tick) display.timerCallback();
    const auto beforeNonzeroSwitch = display.replayRequestGeneration->load();
    parameter (p, "detectorMode", 0);
    for (int tick = 0; tick < 8; ++tick) display.timerCallback();
    if (display.replayRequestGeneration->load() <= beforeNonzeroSwitch)
        throw std::runtime_error ("Nonzero lookahead lost detector history refresh");
    std::cout << "PASS: 0 ms Min/Max audio and live/stopped Display invariant at 1x/4x/8x/16x, ST/LR/MS; 26 ms refresh retained.\n";
}

void QQSCVisualCheck::unityDisplayCheck(QQSuperCompressionAudioProcessorEditor& e,
                                        QQSuperCompressionAudioProcessor& p)
{
    // SAFE is interactive only with a nonzero Lookahead in 1.3.3.
    parameter(p,"lookaheadMs",26);parameter(p,"detectorMode",0);
    e.timerCallback();
    if (e.detectorModeButton.getButtonText() != "SAFE" || e.detectorModeButton.getToggleState()
        || e.detectorModeButton.getBounds().intersects (e.title.getBounds())
        || e.detectorModeButton.getBounds().intersects (e.limiterButton.getBounds()))
        throw std::runtime_error ("Detector selector default label or header layout is wrong");
    e.detectorModeButton.onClick();
    if (p.readSoundParameter (qqsc::params::detectorMode) != 1.0f
        || e.detectorModeButton.getButtonText() != "SAFE" || !e.detectorModeButton.getToggleState())
        throw std::runtime_error ("SAFE did not light up");
    e.detectorModeButton.onClick();
    if (p.readSoundParameter (qqsc::params::detectorMode) != 0.0f
        || e.detectorModeButton.getButtonText() != "SAFE" || e.detectorModeButton.getToggleState())
        throw std::runtime_error ("SAFE did not switch off");
    // A lookahead detector peak may exceed the aligned carrier. At unity gain
    // the displayed Output must still trace the carrier, including fixed trims.
    parameter(p,"processingMode",0);parameter(p,"compressionMode",0);
    parameter(p,"keySource",0);parameter(p,"bypass",0);
    parameter(p,"inputGainDb",0);parameter(p,"outputGainDb",0);
    parameter(p,"ratio",1);parameter(p,"makeupGainDb",0);parameter(p,"mix",100);
    p.setBoundaryForDomainDb(false,false,0,-10);
    p.setBoundaryForDomainDb(false,true,0,1);
    auto& points=e.display.histories[0].points;points.clear();
    DynamicDisplay::HistoryPoint point;point.inputDb=-18.0f;point.detectorDb=-6.0f;
    points.push_back(point);
    e.display.refreshRenderCaches(0);
    const auto& unity=e.display.renderCaches[0].projected;
    if(unity.size!=1 || std::abs(unity.output[0]-unity.input[0])>.002f
       || std::abs(unity.gainReductionBoundary[0]-unity.input[0])>.002f
       || std::abs(unity.effectiveGainReduction[0])>.002f)
        throw std::runtime_error("Unity Display follows detector instead of aligned input");
    parameter(p,"makeupGainDb",6);e.display.refreshRenderCaches(0);
    if(std::abs(e.display.renderCaches[0].projected.output[0]-(-12.0f))>.002f)
        throw std::runtime_error("Unity Display lost fixed Makeup gain");
    parameter(p,"makeupGainDb",0);parameter(p,"ratio",5);parameter(p,"mix",0);
    e.display.refreshRenderCaches(0);
    if(std::abs(e.display.renderCaches[0].projected.output[0]-(-18.0f))>.002f)
        throw std::runtime_error("Dry Mix Display follows detector instead of aligned input");
    parameter(p,"ratio",1);parameter(p,"mix",100);
    std::cout<<"PASS: unity Display follows aligned carrier with different detector peak; fixed Makeup and dry Mix.\n";
}

void QQSCVisualCheck::algorithmChecks(QQSuperCompressionAudioProcessorEditor& e,
                                      QQSuperCompressionAudioProcessor& p,const juce::File& dir)
{
    parameter(p,"algorithmMode",0);parameter(p,"processingMode",0);parameter(p,"compressionMode",0);
    parameter(p,"ratio",5);parameter(p,"makeupGainDb",0);parameter(p,"mix",100);
    p.setBoundaryForDomainDb(false,false,0,-10);p.setBoundaryForDomainDb(false,true,0,1);
    e.timerCallback();p.getAPVTS().copyState();p.getUndoManager().clearUndoHistory();
    {
        GestureProbe touch(*p.getAPVTS().getParameter("algorithmMode"));e.algorithmButton.onClick();
        if(touch.events!=std::vector<bool>{true,false})throw std::runtime_error("Algorithm host gesture mismatch");
    }
    p.getAPVTS().copyState();
    if(e.algorithmButton.getButtonText()!="ALGO: SUPER")throw std::runtime_error("Algorithm click did not update label");
    p.getUndoManager().undo();e.timerCallback();
    if(e.algorithmButton.getButtonText()!="ALGO: CLASSIC")throw std::runtime_error("Algorithm undo failed");
    p.getUndoManager().redo();e.timerCallback();
    if(e.algorithmButton.getButtonText()!="ALGO: SUPER")throw std::runtime_error("Algorithm redo failed");
    auto& points=e.display.histories[0].points;points.clear();
    DynamicDisplay::HistoryPoint point;point.inputDb=point.detectorDb=0;points.push_back(point);
    for(int algorithm:{0,1})
    {
        parameter(p,"algorithmMode",float(algorithm));e.timerCallback();e.display.refreshRenderCaches(0);
        const auto expected=algorithm==0?-8.0:20*std::log10((1+4*std::pow(10.,-.5))/5);
        if(std::abs(e.display.renderCaches[0].projected.output[0]-expected)>.002)
            throw std::runtime_error("Algorithm Display did not follow automation");
    }
    unityDisplayCheck(e,p);
    points.clear();point.inputDb=point.detectorDb=0;points.push_back(point);

    for(auto skin:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        for(int size:{1008,1200,1800})for(int algorithm:{0,1})
        {
            parameter(p,"algorithmMode",float(algorithm));e.theme=skin;e.applyTheme();e.setSize(size,size*2/3);e.timerCallback();
            if(e.algorithmButton.getRight()+6!=e.sidechainButton.getX()
                ||e.algorithmButton.getBounds().intersects(e.title.getBounds())
                ||e.algorithmButton.getY()!=e.sidechainButton.getY())
                throw std::runtime_error("Algorithm toolbar overlap or alignment");
            const juce::String theme=skin==qqsc::ui::Theme::light?"Light":skin==qqsc::ui::Theme::dark?"Dark":"ClassicSkin";
            snapshot(e,dir.getChildFile("Algorithm-"+theme+"-"+juce::String(size)+(algorithm?"-Super.png":"-Classic.png")));
        }
    std::cout<<"PASS: algorithm toolbar click / automation / undo / redo / host gesture, Display both laws, 3 themes at3 sizes with no overlap.\n";
}
