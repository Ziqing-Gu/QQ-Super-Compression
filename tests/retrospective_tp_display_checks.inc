    static void retrospectiveTpDisplayChecks(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p; baseline(p);
        QQSuperCompressionAudioProcessorEditor e(p, settings(root)); sync(p,e);

        auto* normalIn = p.getAPVTS().getParameter(qqsc::params::inputGainDb);
        auto* normalOut = p.getAPVTS().getParameter(qqsc::params::outputGainDb);
        check(normalIn->convertFrom0to1(0.0f) <= -23.999f && normalIn->convertFrom0to1(1.0f) >= 23.999f,
              "Normal Input must be +/-24 dB");
        check(normalOut->convertFrom0to1(0.0f) <= -23.999f && normalOut->convertFrom0to1(1.0f) >= 23.999f,
              "Normal Output must be +/-24 dB");

        e.limiterButton.onClick(); sync(p,e);
        check(e.outputGainSlider.getMinimum() <= -119.999 && e.outputGainSlider.getMaximum() >= 119.999,
              "Limiter Output must remain +/-120 dB");

        // Match the two mode banks to the same evidence source, then run one
        // real Display timer tick before injecting the synthetic history. In the
        // product the 60 Hz timer always establishes lastMode/lastKeySource/
        // captureGeneration before there can be an existing visible history.
        // The old regression seeded private history before the first tick, so
        // its first post-toggle tick performed the normal first-tick source
        // initialisation and falsely reported that Limiter itself cleared history.
        set(p,"processingMode",float(qqsc::params::stereoLinked));
        set(p,"keySource",float(qqsc::params::keyInternal));
        set(p,"limiterProcessingMode",float(qqsc::params::stereoLinked));
        set(p,"limiterKeySource",float(qqsc::params::keyInternal));
        QQSCVisualCheck::tickDisplay(e.display);

        // Seed one fixed historical evidence set. Every assertion below reuses
        // those same points; no new history is captured. That is the product's
        // defining retrospective Display contract.
        QQSCVisualCheck::seedTransferContractHistory(e.display);
        set(p,"limiterInputGainDb",0.0f);
        set(p,"limiterOutputDb",0.0f);
        set(p,"limiterMakeupGainDb",0.0f);
        set(p,"limiterMix",100.0f);
        set(p,"ceilingDb",0.0f);
        set(p,"truePeakLimiting",0.0f);
        set(p,"limiterCompressionMode",0.0f);
        set(p,"limiterRatio",200.0f);
        set(p,"limiterAlgorithmMode",0.0f);
        p.setBoundaryForDomainDb(false,false,0,-18.0f);
        p.setBoundaryForDomainDb(false,true,0,qqsc::params::rangeOffDb);

        const auto classic = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        check(classic.blue.size()==4,"Retrospective transfer history size");

        // The first two points deliberately contain unrelated captured carrier
        // peaks but identical detector evidence. INT projection must use the
        // detector-aligned transfer level, so those points cannot create fake
        // upward spikes.
        check(std::abs(classic.blue[0]-classic.blue[1])<.001f,
              "INT Display paired an unrelated carrier peak with detector GR");

        // With no Ceiling overshoot, TP OFF still shows only the ordinary QQ
        // compression transfer. Once the signal is driven above Ceiling later in
        // this test, TP OFF is allowed to move Blue further down because 1.2.28
        // makes it an actual 8x Hard Clipper rather than the old native guard.
        constexpr float visualThreshold=-18.0f;
        for(const auto blue:classic.blue)
            check(blue>=visualThreshold-.02f,
                  "Undriven 8x Hard Clip unexpectedly changed the QQ transfer");

        // Classic/Super are the original QQ transfer laws even in Limiter mode.
        // Changing the CURRENT algorithm must immediately reshape old history.
        set(p,"limiterAlgorithmMode",1.0f);
        const auto super = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        bool algorithmChanged=false;
        for(size_t i=0;i<super.blue.size();++i)
            if(std::abs(super.blue[i]-classic.blue[i])>.05f) algorithmChanged=true;
        check(algorithmChanged,"Classic/Super did not retrospectively reshape old Display history");

        set(p,"limiterRatio",800.0f);
        const auto ratio800 = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        check(std::abs(ratio800.blue[2]-super.blue[2])>.05f,
              "Ratio did not retrospectively reshape old Display history");

        p.setBoundaryForDomainDb(false,false,0,-12.0f);
        const auto threshold12 = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        check(std::abs(threshold12.blue[2]-ratio800.blue[2])>.05f,
              "Threshold did not retrospectively reshape old Display history");

        set(p,"limiterMix",50.0f);
        const auto mix50 = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        check(std::abs(mix50.blue[2]-threshold12.blue[2])>.05f,
              "Mix did not retrospectively reshape old Display history");

        // Input Gain changes the current internal detector level while the
        // pre-gain transfer reference stays anchored to the same evidence.
        set(p,"limiterMix",100.0f);
        set(p,"limiterInputGainDb",6.0f);
        const auto inputPlus6 = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        check(std::abs(inputPlus6.blue[2]-threshold12.blue[2])>.05f,
              "Input Gain did not retrospectively reshape old Display history");
        set(p,"limiterInputGainDb",0.0f);

        // 1.2.28: TP OFF is itself a post-dynamics 8x Hard Clipper. First drive
        // the current projected signal into Ceiling and confirm that the SAME old
        // history immediately acquires hard-clip attenuation. Then switch TP ON:
        // only the Ceiling algorithm changes; history and PDC stay intact.
        set(p,"limiterMix",100.0f);
        set(p,"limiterRatio",200.0f);
        p.setBoundaryForDomainDb(false,false,0,-18.0f);
        set(p,"limiterOutputDb",0.0f);
        set(p,"truePeakLimiting",0.0f);
        const auto hardClipUndriven = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        set(p,"limiterOutputDb",18.0f);
        const auto tpOff = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        bool hardClipChanged=false, hardClipBelowThreshold=false;
        for(size_t i=0;i<tpOff.blue.size();++i)
        {
            if(tpOff.blue[i] < hardClipUndriven.blue[i]-.05f) hardClipChanged=true;
            if(tpOff.blue[i] < visualThreshold-.05f) hardClipBelowThreshold=true;
        }
        check(hardClipChanged,"8x Hard Clip did not retrospectively affect existing blue history");
        check(hardClipBelowThreshold,"8x Hard Clip could not add post-threshold attenuation to blue");

        set(p,"truePeakLimiting",1.0f);
        const auto tpOn = QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        bool tpChanged=false;
        for(size_t i=0;i<tpOn.blue.size();++i)
        {
            check(tpOn.blue[i] <= tpOff.blue[i] + .02f,
                  "TP unexpectedly raised the calculated blue transfer result");
            if(std::abs(tpOn.blue[i]-tpOff.blue[i])>.05f) tpChanged=true;
        }
        check(tpChanged,"TP toggle did not retrospectively change Hard Clip to TP history");

        // Output/Makeup do not belong to base compressor GR, but with TP ON they
        // alter how hard the TP stage is driven, so their CURRENT values can also
        // reshape old blue history through the TP contribution.
        const auto beforeOutputChange=tpOn.blue;
        set(p,"limiterOutputDb",12.0f);
        const auto afterOutputChange=QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        bool outputChanged=false;
        for(size_t i=0;i<afterOutputChange.blue.size();++i)
            if(std::abs(afterOutputChange.blue[i]-beforeOutputChange[i])>.05f) outputChanged=true;
        check(outputChanged,"Output did not retrospectively update TP contribution to blue");

        set(p,"limiterOutputDb",18.0f);
        set(p,"limiterMakeupGainDb",6.0f);
        const auto makeupDrivenTp=QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        bool makeupChanged=false;
        for(size_t i=0;i<makeupDrivenTp.blue.size();++i)
            if(std::abs(makeupDrivenTp.blue[i]-tpOn.blue[i])>.05f) makeupChanged=true;
        check(makeupChanged,"Makeup did not retrospectively update TP contribution to blue");
        set(p,"limiterMakeupGainDb",0.0f);

        // Ceiling is a fixed output-level shift that is mirrored by the
        // OutputCeiling target in Limiter mode. Lowering Ceiling therefore
        // moves the projected final output but does NOT create extra TP GR by
        // itself: the signal feeding the guard and its target move together.
        // It must still invalidate/re-project the old history so Orange moves,
        // while Blue (Dynamics + Mix + actual TP attenuation) remains stable.
        const auto ceilingRevisionBefore=p.getDisplayProjectionRevision();
        set(p,"ceilingDb",-3.0f);
        check(p.getDisplayProjectionRevision()!=ceilingRevisionBefore,
              "Ceiling did not invalidate Display projection revision");
        const auto lowerCeiling=QQSCVisualCheck::projectRetrospectiveTpHistory(e.display,0);
        bool ceilingBlueStable=true, ceilingOutputShifted=false;
        for(size_t i=0;i<lowerCeiling.blue.size();++i)
        {
            if(std::abs(lowerCeiling.blue[i]-tpOn.blue[i])>.02f) ceilingBlueStable=false;
            if(std::abs(lowerCeiling.output[i]-tpOn.output[i])>2.90f) ceilingOutputShifted=true;
        }
        check(ceilingBlueStable,
              "Ceiling incorrectly changed blue TP GR even though drive and target shift together");
        check(ceilingOutputShifted,
              "Ceiling did not retrospectively shift projected final output");

        // Limiter ON/OFF is also only a current projection choice now; it must
        // not throw away the existing evidence window.
        const auto historyBeforeToggle=QQSCVisualCheck::retrospectiveHistorySize(e.display);
        e.limiterButton.onClick(); sync(p,e); QQSCVisualCheck::tickDisplay(e.display);
        e.limiterButton.onClick(); sync(p,e); QQSCVisualCheck::tickDisplay(e.display);
        check(QQSCVisualCheck::retrospectiveHistorySize(e.display)>=historyBeforeToggle,
              "Limiter toggle cleared retrospective Display history");

        std::cout<<"PASS: retrospective Display keeps evidence, reprojects Classic/Super/Ratio/Threshold/Mix, shows 8x Hard Clip at TP OFF, and replays TP recovery at TP ON.\n";
    }
