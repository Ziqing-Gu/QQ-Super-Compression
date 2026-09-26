from pathlib import Path
p=Path(__file__).parent/'Source/PluginProcessor.cpp'
s=p.read_text(encoding='utf-8')
def replace(a,b):
    global s
    assert a in s,a[:100];s=s.replace(a,b)
replace('(6u);','(8u);')
replace('            6u, stageCount','            8u, stageCount')
replace('wetBaseBuffer.setSize (6,','wetBaseBuffer.setSize (8,')
replace('oversamplingInputBuffer.setSize (6,','oversamplingInputBuffer.setSize (8,')
replace('    oversamplingInputBuffer.setSize (8, configuredMaximumBlockSize, false, true, true);',
'''    oversamplingInputBuffer.setSize (8, configuredMaximumBlockSize, false, true, true);
    mixControlBuffer.setSize (11, configuredMaximumBlockSize, false, true, true);
    abActive = false;
    abTransferPending.store (false);
    lastAudioTransfer = makeABTransfer (captureCurrentSnapshot());''')
replace('    resetRatioSmoother (algorithmFade,', '''    abFade.reset (currentSampleRate * currentOversamplingFactor, 0.020);
    abFade.setCurrentAndTargetValue (1.0f);
    abActive = false;
    resetRatioSmoother (algorithmFade,''')
replace('>= 6u','>= 8u');replace('< 6u','< 8u');replace('.getSubsetChannelBlock (0, 6)','.getSubsetChannelBlock (0, 8)')
replace('        oversamplingInputBuffer.setSample (5, i, 0.0f);',
'''        oversamplingInputBuffer.setSample (5, i, 0.0f);
        oversamplingInputBuffer.setSample (6, i, 0.0f);
        oversamplingInputBuffer.setSample (7, i, 0.0f);''')
replace('    // Main and selected Key enter the same effective 1x/8x/16x internal domain.',
'''    // Cache the existing base-rate controls once. The original six paths
    // still use these exact values after downsampling. Two extra aligned
    // channels carry the complete A/B result, including Makeup and Mix.
    const std::array<juce::SmoothedValue<float>*,5> makeups { &makeupSTSmoother, &makeupLSmoother, &makeupRSmoother, &makeupMSmoother, &makeupSSmoother };
    const std::array<juce::SmoothedValue<float>*,5> mixes { &mixSmoother, &mixLSmoother, &mixRSmoother, &mixMSmoother, &mixSSmoother };
    for (int i=0;i<numSamples;++i)
    {
        for (size_t d=0;d<5;++d)
        {
            mixControlBuffer.setSample (int(d),i,juce::Decibels::decibelsToGain (makeups[d]->getNextValue()));
            mixControlBuffer.setSample (int(d)+5,i,mixes[d]->getNextValue());
        }
        mixControlBuffer.setSample (10,i,outputGainSmoother.getNextValue());
    }
    if (abTransferPending.load (std::memory_order_acquire))
    {
        const juce::SpinLock::ScopedTryLockType lock (abTransferLock);
        if (lock.isLocked())
        {
            if (! requestedABCompatible) abActive = false;
            else if (abActive && requestedABTo == abFrom && ! abFromFrozen)
                abFade.setTargetValue (0.0f);
            else if (abActive && requestedABTo == abTo)
                abFade.setTargetValue (1.0f);
            else
            {
                abFromFrozen = abActive;
                abFrozen = abLastMatrix;
                abFrom = lastAudioTransfer;
                abTo = requestedABTo;
                abFade.setCurrentAndTargetValue (0.0f);
                abFade.setTargetValue (1.0f);
                abActive = true;
            }
            abTailSamples = (currentTotalLatencySamples + 64) * currentOversamplingFactor;
            abTransferPending.store (false,std::memory_order_release);
        }
    }
    const bool abOutputForBlock = abActive;

    // Main and selected Key enter the same effective 1x/8x/16x internal domain.''')
replace('        oversampledBlock.setSample (5, i, wetS);',
'''        oversampledBlock.setSample (5, i, wetS);

        const int baseSample = i / currentOversamplingFactor;
        std::array<float,5> totalGains;
        for (size_t d=0;d<5;++d)
        {
            const auto amount=mixControlBuffer.getSample (int(d)+5,baseSample);
            totalGains[d]=((1.0f-amount)+gains[d]*mixControlBuffer.getSample (int(d),baseSample)*amount)
                       * mixControlBuffer.getSample (10,baseSample);
        }
        auto complete = qqsc::ABTransfer::matrix (totalGains,mode);
        if (abOutputForBlock)
        {
            const auto from = abFromFrozen ? abFrozen : abFrom.evaluate (levels,useExternalKey);
            const auto to = abTo.evaluate (levels,useExternalKey);
            const auto t = abFade.getNextValue();
            const auto w = t*t*(3.0f-2.0f*t); // zero slope at either endpoint
            for (size_t c=0;c<4;++c) complete[c]=(1.0f-w)*from[c]+w*to[c];
            if (abFade.isSmoothing())
                abTailSamples=(currentTotalLatencySamples+64)*currentOversamplingFactor;
            else if (--abTailSamples<=0 && ! restoringDynamicsState.load (std::memory_order_acquire))
                abActive=false;
        }
        abLastMatrix=complete;
        oversampledBlock.setSample (6,i,complete[0]*dryLInternal+complete[1]*dryRInternal);
        oversampledBlock.setSample (7,i,complete[2]*dryLInternal+complete[3]*dryRInternal);''')
for i,name in enumerate(['ST','L','R','M','S']):
    # Match exact original whitespace by finding the unique expression only.
    replace('juce::Decibels::decibelsToGain (makeup'+('ST' if i==0 else name)+'Smoother.getNextValue())',f'mixControlBuffer.getSample ({i},i)')
for i,name in enumerate(['','L','R','M','S']):
    replace('mix'+name+'Smoother.getNextValue()',f'mixControlBuffer.getSample ({i+5},i)')
replace('const auto outputGain = outputGainSmoother.getNextValue();','const auto outputGain = mixControlBuffer.getSample (10,i);')
replace('const float activeOutL = mixedL * outputGain;','const float activeOutL = abOutputForBlock ? wetBaseBuffer.getSample (6,i) : mixedL * outputGain;')
replace('const float activeOutR = mixedR * outputGain;','const float activeOutR = abOutputForBlock && stereoBus ? wetBaseBuffer.getSample (7,i) : mixedR * outputGain;')
replace('    auto wetBlock = juce::dsp::AudioBlock<float> (wetBaseBuffer)',
'''    if (! restoringDynamicsState.load (std::memory_order_acquire))
        lastAudioTransfer = makeABTransfer (captureCurrentSnapshot());
    auto wetBlock = juce::dsp::AudioBlock<float> (wetBaseBuffer)''')
replace('void QQSuperCompressionAudioProcessor::applySnapshot (const ParameterSnapshot& snapshot)\n{',
'''qqsc::ABTransfer QQSuperCompressionAudioProcessor::makeABTransfer (const ParameterSnapshot& s) noexcept
{
    qqsc::ABTransfer t;
    t.dual=s.compressionMode==1; t.mode=s.mode;
    t.algorithm=s.algorithmMode==0 ? qqsc::CompressionAlgorithm::classic : qqsc::CompressionAlgorithm::super;
    t.ratio={s.ratio,s.ratioL,s.ratioR,s.ratioM,s.ratioS};
    t.lower={s.thresholdDb,s.thresholdLDb,s.thresholdRDb,s.thresholdMDb,s.thresholdSDb};
    t.makeup={s.makeupST,s.makeupL,s.makeupR,s.makeupM,s.makeupS};
    t.mix={s.mix,s.mixL,s.mixR,s.mixM,s.mixS};
    t.output=juce::Decibels::decibelsToGain(s.outputGainDb);
    for(size_t d=0;d<5;++d)
    {
        t.lower[d]=qqsc::params::thresholdLinear(t.dual?s.upThreshold[d]:t.lower[d]);
        t.upper[d]=t.dual?qqsc::params::thresholdLinear(s.downThreshold[d]):qqsc::params::rangeLinear(s.range[d]);
        t.upRatio[d]=s.upEnabled[d]?s.upRatio[d]:1.f;
        t.downRatio[d]=s.downEnabled[d]?s.downRatio[d]:1.f;
        t.makeup[d]=juce::Decibels::decibelsToGain(t.makeup[d]); t.mix[d]*=.01f;
    }
    return t;
}

void QQSuperCompressionAudioProcessor::queueABTransfer (const ParameterSnapshot& from, const ParameterSnapshot& to)
{
    const juce::SpinLock::ScopedLockType lock(abTransferLock);
    // A gain-only result crossfade is valid when both banks share the carrier
    // and detector timing/source. Latency/source changes retain their existing
    // reconfiguration path; never blend differently delayed signals here.
    requestedABCompatible=from.inputGainDb==to.inputGainDb && from.lookaheadMs==to.lookaheadMs
        && from.oversampling==to.oversampling && from.keySource==to.keySource
        && from.keyGainDb==to.keyGainDb && from.keyHpfHz==to.keyHpfHz;
    requestedABFrom=makeABTransfer(from); requestedABTo=makeABTransfer(to);
    abTransferPending.store(true,std::memory_order_release);
}

void QQSuperCompressionAudioProcessor::applySnapshot (const ParameterSnapshot& snapshot)
{
    queueABTransfer (captureCurrentSnapshot(), snapshot);''')
p.write_text(s,encoding='utf-8',newline='\n')
print('Added complete-result A/B gain-domain crossfade with aligned oversampling channels.')
