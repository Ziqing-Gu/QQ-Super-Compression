    static void performanceArchitectureChecks(const juce::File&)
    {
        constexpr double rate=48000.0;
        constexpr int blockSize=128;
        const auto latency=[&](bool limiter,bool tp,float lookahead,int coreOs,int ceilingOs)
        {
            QQSuperCompressionAudioProcessor p; baseline(p);
            set(p,"lookaheadMs",lookahead);set(p,"oversampling",float(qqsc::params::migrateLegacyOversamplingChoice(coreOs)));
            set(p,"limiterLookaheadMs",lookahead);set(p,"limiterOversampling",float(qqsc::params::migrateLegacyOversamplingChoice(coreOs)));
            set(p,qqsc::params::ceilingOversampling,float(qqsc::params::migrateLegacyOversamplingChoice(ceilingOs)));
            set(p,qqsc::params::truePeakLimiting,tp ? 1.0f : 0.0f);
            set(p,qqsc::params::limiterMode,limiter ? 1.0f : 0.0f);
            p.setRateAndBufferSizeDetails(rate,blockSize);p.prepareToPlay(rate,blockSize);
            return p.getLatencySamples();
        };

        const int normal26=latency(false,false,26.0f,0,0);
        check(normal26==latency(false,false,26.0f,0,1)
           && normal26==latency(false,false,26.0f,0,2),
              "Normal latency changed with hidden Ceiling OS");

        const int lim26_1=latency(true,false,26.0f,0,0);
        const int lim26_8=latency(true,false,26.0f,0,1);
        const int lim26_16=latency(true,false,26.0f,0,2);
        check(lim26_1==normal26,"Limiter Ceiling 1x must add no oversampling/PDC latency");
        check(lim26_8>lim26_1 && lim26_16>=lim26_8,
              "Ceiling 1x/8x/16x latency ordering is invalid");
        check(latency(true,true,26.0f,0,0)==latency(true,true,26.0f,0,1),
              "TP+stored 1x was not promoted to effective 8x");
        check(latency(true,true,26.0f,0,1)==lim26_8,
              "TP toggle changed 8x Ceiling PDC");
        check(latency(true,true,26.0f,0,2)==lim26_16,
              "TP toggle changed 16x Ceiling PDC");

        const int normal0_8=latency(false,false,0.0f,1,0);
        const int normal0_16=latency(false,false,0.0f,2,0);
        const int shared8=latency(true,false,0.0f,1,1);
        const int shared16=latency(true,false,0.0f,2,2);
        const int fullCeiling8=lim26_8-normal26;
        const int fullCeiling16=lim26_16-normal26;
        check(shared8>normal0_8 && shared8<normal0_8+fullCeiling8,
              "Matched 0 ms 8x Core/Ceiling did not remove duplicate Ceiling audio OS latency");
        check(shared16>normal0_16 && shared16<normal0_16+fullCeiling16,
              "Matched 0 ms 16x Core/Ceiling did not remove duplicate Ceiling audio OS latency");
        check(latency(true,true,0.0f,1,1)==shared8,
              "Shared 8x TP toggle changed PDC");
        check(latency(true,true,0.0f,2,2)==shared16,
              "Shared 16x TP toggle changed PDC");

        // Mismatched factors currently preserve the exact requested quality via
        // the safe standalone Ceiling path rather than silently changing rate.
        check(latency(true,false,0.0f,1,2)==normal0_8+fullCeiling16,
              "8x Core + 16x Ceiling did not preserve exact 16x fallback PDC");
        check(latency(true,false,0.0f,2,1)==normal0_16+fullCeiling8,
              "16x Core + 8x Ceiling did not preserve exact 8x fallback PDC");

        // FULL/ECO is analysis-only. With no editor open, ECO sleeps the
        // analysis workload, but the audible path and PDC must remain exactly
        // identical. Each instance owns its independent performance choice.
        QQSuperCompressionAudioProcessor full; baseline(full);
        const auto fullAudio=render(full,0.45f,blockSize,false,731.0,rate,0.83f);
        QQSuperCompressionAudioProcessor eco; baseline(eco);eco.setEcoMode(true);
        const auto ecoAudio=render(eco,0.45f,blockSize,false,731.0,rate,0.83f);
        check(fullAudio.latency==ecoAudio.latency,"FULL/ECO changed PDC");
        check(fullAudio.output.size()==ecoAudio.output.size(),"FULL/ECO render length mismatch");
        float ecoDelta=0.0f;
        for(size_t i=0;i<fullAudio.output.size();++i)
        {
            ecoDelta=juce::jmax(ecoDelta,std::abs(fullAudio.output[i]-ecoAudio.output[i]));
            ecoDelta=juce::jmax(ecoDelta,std::abs(fullAudio.right[i]-ecoAudio.right[i]));
        }
        check(ecoDelta<=1.0e-7f,"FULL/ECO changed audible Normal output");

        qqsc::OutputCeiling native;
        native.prepare(rate,false,-6.0f,qqsc::params::tpAuto,0,blockSize);
        const auto nativeOut=native.process(0.95f,-0.95f);
        const auto target=juce::Decibels::decibelsToGain(-6.0f);
        check(std::abs(nativeOut[0])<=target+1.0e-6f && std::abs(nativeOut[1])<=target+1.0e-6f,
              "Ceiling 1x is not a native Hard Clipper");
        check(native.latencySamples()==0,"Ceiling 1x unexpectedly reports latency");

        std::cout<<"PERFORMANCE-ARCHITECTURE normal26="<<normal26
                 <<" ceiling8="<<fullCeiling8<<" ceiling16="<<fullCeiling16
                 <<" shared8="<<shared8<<" shared16="<<shared16<<" ecoDelta="<<ecoDelta<<"\n";
        std::cout<<"PASS: 1.2.34 Ceiling OS 1x/8x/16x, TP promotion/fixed-PDC and matched 0 ms Core/Ceiling sharing are coherent.\n";
    }
