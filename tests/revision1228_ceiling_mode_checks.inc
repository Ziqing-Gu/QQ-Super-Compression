    static void ceilingModeChecks(const juce::File&)
    {
        constexpr double rate=48000.0;
        constexpr int blockSize=128;
        const auto preparedLatency=[&](bool limiter,bool tp,float lookahead,int oversampling)
        {
            QQSuperCompressionAudioProcessor p; baseline(p);
            set(p,"lookaheadMs",lookahead);
            set(p,"oversampling",float(oversampling));
            set(p,"limiterLookaheadMs",lookahead);
            set(p,"limiterOversampling",float(oversampling));
            set(p,"truePeakLimiting",tp ? 1.0f : 0.0f);
            set(p,"limiterMode",limiter ? 1.0f : 0.0f);
            p.setRateAndBufferSizeDetails(rate,blockSize);
            p.prepareToPlay(rate,blockSize);
            return p.getLatencySamples();
        };

        // Normal mode must pay only for the user's own Lookahead / 0 ms
        // Oversampling choice. Hidden Limiter Ceiling state contributes zero PDC.
        const int normal26_1x=preparedLatency(false,false,26.0f,0);
        const int normal26_8x=preparedLatency(false,true,26.0f,1);
        const int normal26_16x=preparedLatency(false,false,26.0f,2);
        check(normal26_1x==int(std::llround(rate*.026)),"Normal 26 ms includes hidden Ceiling latency");
        check(normal26_8x==normal26_1x && normal26_16x==normal26_1x,
              "Non-zero Lookahead incorrectly inherited 0 ms Oversampling/Ceiling latency");

        const int normal0_1x=preparedLatency(false,false,0.0f,0);
        const int normal0_8x=preparedLatency(false,false,0.0f,1);
        const int normal0_16x=preparedLatency(false,true,0.0f,2);
        check(normal0_1x==0,"Normal 0 ms / 1x must report zero latency");
        check(normal0_8x>normal0_1x && normal0_16x>=normal0_8x,
              "Normal 0 ms user Oversampling does not control latency");
        check(preparedLatency(false,true,0.0f,0)==normal0_1x,
              "Normal TP preference changed latency while Limiter is OFF");

        // Limiter adds one fixed Ceiling latency. TP is now only an algorithm
        // selection inside that prepared 8x pipeline, so PDC must be identical.
        const int limiter26Hard=preparedLatency(true,false,26.0f,0);
        const int limiter26Tp=preparedLatency(true,true,26.0f,0);
        check(limiter26Hard==limiter26Tp,"Limiter TP toggle changed reported latency");
        check(limiter26Hard>normal26_1x,"Limiter did not add Ceiling pipeline latency");

        const int limiter0Hard=preparedLatency(true,false,0.0f,0);
        const int limiter0Tp=preparedLatency(true,true,0.0f,0);
        check(limiter0Hard==limiter0Tp,"0 ms Limiter TP toggle changed reported latency");
        check(limiter0Hard>normal0_1x,"0 ms Limiter Ceiling latency missing");

        // Exercise a live mode/TP sequence. Limiter boundaries may change PDC;
        // TP boundaries must not. One audio callback is enough to publish mode
        // automation that occurs outside the editor.
        QQSuperCompressionAudioProcessor live; baseline(live);
        set(live,"lookaheadMs",26.0f);set(live,"oversampling",0.0f);
        set(live,"limiterLookaheadMs",26.0f);set(live,"limiterOversampling",0.0f);
        set(live,"limiterMode",0.0f);set(live,"truePeakLimiting",0.0f);
        live.setRateAndBufferSizeDetails(rate,blockSize);live.prepareToPlay(rate,blockSize);
        juce::AudioBuffer<float> silence(2,blockSize);silence.clear();juce::MidiBuffer midi;
        const int liveNormal=live.getLatencySamples();
        set(live,"limiterMode",1.0f); live.processBlock(silence,midi);
        const int liveHard=live.getLatencySamples();
        set(live,"truePeakLimiting",1.0f); live.processBlock(silence,midi);
        const int liveTp=live.getLatencySamples();
        set(live,"truePeakLimiting",0.0f); live.processBlock(silence,midi);
        const int liveHardAgain=live.getLatencySamples();
        set(live,"limiterMode",0.0f); live.processBlock(silence,midi);
        const int liveNormalAgain=live.getLatencySamples();
        check(liveHard>liveNormal,"Entering Limiter did not publish Ceiling PDC");
        check(liveTp==liveHard && liveHardAgain==liveHard,"TP automation changed PDC");
        check(liveNormalAgain==liveNormal,"Leaving Limiter did not restore Normal PDC");

        std::cout<<"PASS: 1.2.28 Normal latency excludes Limiter Ceiling; Limiter Hard/TP share fixed 8x PDC and live TP toggles leave latency unchanged.\n";
    }
