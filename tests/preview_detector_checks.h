namespace
{
void previewDetectorChecks()
{
    // Independent brute-force windows exercise inclusivity, ring wrap, reset,
    // negative/over-unity samples, and the original zero-lookahead path.
    std::vector<float> signal(7000);
    uint32_t state=0x12345678;
    for(auto& value:signal)
    {
        state=state*1664525u+1013904223u;
        value=(float(state>>8)/16777216.f-.5f)*2.4f;
    }
    qqsc::StaticCompressionEngine engine;engine.prepare(2400);
    for(int delay:{0,1,17,480,1248,2400,17})
    {
        engine.setLookaheadSamples(delay);
        for(int n=0;n<int(signal.size());++n)
        {
            engine.processSample(signal[size_t(n)],8,.01f,10000+n);
            float expected=0;
            if(n>=delay)
            {
                float past=0,future=0;
                for(int i=std::max(0,n-2*delay);i<=n-delay;++i)past=std::max(past,std::min(1.f,std::abs(signal[size_t(i)])));
                for(int i=n-delay;i<=n;++i)future=std::max(future,std::min(1.f,std::abs(signal[size_t(i)])));
                expected=std::min(past,future);
            }
            check(engine.getCurrentLevel()==expected,"Aligned peak differs from independent two-window oracle");
        }
    }
    // Reconfiguration must warm both windows from retained audio history.
    QQSuperCompressionAudioProcessor p;setupDb(p,0,0,-20,1,8);
    p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);
    juce::MidiBuffer midi;juce::AudioBuffer<float> b(2,256);
    const float expected=.5f*std::pow(.1f/.5f,.875f);
    for(int iteration=0;iteration<100;++iteration)
    {
        if(iteration==40)set(p,"lookaheadMs",40);
        if(iteration==70)set(p,"lookaheadMs",100);
        for(int c=0;c<2;++c)for(int i=0;i<256;++i)b.setSample(c,i,.5f);
        p.processBlock(b,midi);
        if(iteration>30)for(int i=0;i<256;++i)
            check(std::abs(b.getSample(0,i)-expected)<1e-6,"Lookahead change loses detector history");
    }
    p.releaseResources();
    std::cout<<"PASS: independent aligned-window oracle, ring wrap/reset/0ms; 26/40/100ms live detector rebuild.\n";
}
}
