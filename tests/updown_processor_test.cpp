#include "PluginProcessor.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <functional>
#include <vector>
#include <algorithm>

namespace
{
constexpr double testSampleRate = 48000.0;
constexpr double pi = 3.14159265358979323846;
void check (bool condition, const char* message)
{
    if (! condition) throw std::runtime_error (message);
}
void set (QQSuperCompressionAudioProcessor& p, const char* id, float value)
{
    auto* parameter = p.getAPVTS().getParameter (id);
    if (parameter == nullptr) throw std::runtime_error (id);
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}
float get (QQSuperCompressionAudioProcessor& p, const char* id)
{
    auto* value = p.getAPVTS().getRawParameterValue (id);
    if (value == nullptr) throw std::runtime_error (id);
    return value->load();
}
void baseline (QQSuperCompressionAudioProcessor& p)
{
    set (p, "lookaheadMs", 26.0f);
    set (p, "oversampling", 0.0f);
    set (p, "inputGainDb", 0.0f); set (p, "outputGainDb", 0.0f);
    set (p, "makeupGainDb", 0.0f); set (p, "mix", 100.0f);
    set (p, "thresholdDb", -40.0f); set (p, "rangeDb", -6.0f);
    set (p, "upThresholdDb", -24.0f); set (p, "downThresholdDb", -12.0f);
    set (p, "ratio", 8.0f); set (p, "upRatio", 0.125f); set (p, "downRatio", 8.0f);
}
struct AudioResult
{
    std::vector<float> output;
    std::vector<float> right;
    int latency = 0;
    float meter = 0.0f;
};
AudioResult render (QQSuperCompressionAudioProcessor& p, float amplitude, int blockSize = 256,
                    bool bypass = false, double frequency = 400.0, double rate = testSampleRate,
                    float rightScale = 1.0f)
{
    p.setRateAndBufferSizeDetails (rate, blockSize);
    p.prepareToPlay (rate, blockSize);
    AudioResult result;
    const auto count = static_cast<int> (rate * 0.8);
    result.output.reserve (static_cast<size_t> (count));
    juce::MidiBuffer midi;
    for (int offset = 0; offset < count; offset += blockSize)
    {
        const int size = std::min (blockSize, count - offset);
        juce::AudioBuffer<float> audio (2, size);
        for (int i = 0; i < size; ++i)
        {
            const auto x = amplitude * static_cast<float> (std::sin (2.0*pi*frequency*(offset+i)/rate));
            audio.setSample (0, i, x); audio.setSample (1, i, x * rightScale);
        }
        if (bypass) p.processBlockBypassed (audio, midi); else p.processBlock (audio, midi);
        for (int i = 0; i < size; ++i)
        {
            const float value = audio.getSample (0, i);
            check (std::isfinite (value), "Non-finite actual processor output");
            result.output.push_back (value);
            result.right.push_back (audio.getSample (1, i));
        }
    }
    result.latency = p.getLatencySamples();
    result.meter = p.getMeterState().gainReductionDb0.load();
    p.releaseResources();
    return result;
}
double rmsGain (const AudioResult& result, double amplitude, double rate = testSampleRate)
{
    const int start = static_cast<int> (rate * 0.3);
    const int count = static_cast<int> (rate * 0.25);
    double energy = 0.0;
    for (int i = start; i < start + count; ++i) energy += result.output[static_cast<size_t> (i)] * result.output[static_cast<size_t> (i)];
    return std::sqrt (energy / count) / (amplitude / std::sqrt (2.0));
}
double harmonicDb (const AudioResult& result, double frequency = 400.0, double rate = testSampleRate)
{
    const int period = static_cast<int> (std::round (rate / frequency));
    const int start = static_cast<int> (rate * 0.3);
    const int count = (static_cast<int> (rate * 0.3) / period) * period;
    double harmonicEnergy = 0.0, fundamentalEnergy = 0.0;
    for (int harmonic = 1; harmonic <= 7; ++harmonic)
    {
        double re = 0.0, im = 0.0;
        for (int i = 0; i < count; ++i)
        {
            const double phase = 2.0*pi*frequency*harmonic*i/rate;
            const double x = result.output[static_cast<size_t> (start+i)];
            re += x*std::cos (phase); im += x*std::sin (phase);
        }
        const double energy = re*re + im*im;
        if (harmonic == 1) fundamentalEnergy = energy; else harmonicEnergy += energy;
    }
    return 10.0 * std::log10 (std::max (1e-30, harmonicEnergy / fundamentalEnergy));
}
void collisionChecks()
{
    const char* lower[] { "thresholdDb", "thresholdLDb", "thresholdRDb", "thresholdMDb", "thresholdSDb",
                         "upThresholdDb", "upThresholdLDb", "upThresholdRDb", "upThresholdMDb", "upThresholdSDb" };
    const char* upper[] { "rangeDb", "rangeLDb", "rangeRDb", "rangeMDb", "rangeSDb",
                         "downThresholdDb", "downThresholdLDb", "downThresholdRDb", "downThresholdMDb", "downThresholdSDb" };
    QQSuperCompressionAudioProcessor p;
    for (int i = 0; i < 10; ++i)
    {
        set (p, lower[i], -50.0f); set (p, upper[i], -20.0f);
        set (p, lower[i], -10.0f);
        check (std::abs (p.getBoundaryForDomainDb (i >= 5, false, i % 5) + 10.0f) < 0.01f && std::abs (p.getBoundaryForDomainDb (i >= 5, true, i % 5) + 10.0f) < 0.01f,
               "Lower boundary does not push upper under host parameter edits");
        set (p, lower[i], -40.0f);
        check (std::abs (p.getBoundaryForDomainDb (i >= 5, true, i % 5) + 10.0f) < 0.01f, "Reversal moved unselected boundary");
        set (p, upper[i], -60.0f);
        check (std::abs (p.getBoundaryForDomainDb (i >= 5, false, i % 5) + 60.0f) < 0.01f, "Upper does not push lower");
        set (p, upper[i], 0.0f);
        set (p, lower[i], 0.0f);
        check (get (p, upper[i]) == 0.0f, "Boundary endpoint mismatch");
    }
    std::cout << "PASS: all ten ST/LR/MS boundary pairs push, separate and reach endpoints through host edits.\n";
}
void audioChecks()
{
    QQSuperCompressionAudioProcessor p;
    baseline (p);
    for (int mode : {0,1})
    {
        set (p, "compressionMode", static_cast<float> (mode));
        if (mode == 0) { set (p, "thresholdDb", -20.0f); set (p, "rangeDb", -20.0f); }
        else { set (p, "upThresholdDb", -20.0f); set (p, "downThresholdDb", -20.0f); }
        for (float amplitude : {0.001f, 0.05f, 0.5f})
            check (std::abs (rmsGain (render (p, amplitude), amplitude)-1.0) < 1e-5,
                   "Collapsed boundaries must disable the entire dynamic stage");
    }
    baseline (p); set (p, "compressionMode", 0.0f);
    for (float amplitude : {0.001f, 0.75f})
        check (std::abs (rmsGain (render (p, amplitude), amplitude)-1.0) < 1e-5, "Single outside range is not unity");
    auto down = render (p, 0.1f);
    check (rmsGain (down, 0.1) < 0.95, "Single downward did not reduce");
    check (down.meter > 0.0f, "Downward meter sign");
    set (p, "ratio", 0.125f);
    auto up = render (p, 0.1f);
    check (rmsGain (up, 0.1) > 1.05, "Single upward did not boost");
    check (rmsGain (render (p, 0.03f), 0.03) > rmsGain (up, 0.1), "Single upward must lift softer sounds more");
    check (up.meter < 0.0f, "Upward meter sign");
    const auto singleUpThd = harmonicDb (up);
    check (singleUpThd < -110.0, "Steady single upward harmonic regression");
    set (p, "ratio", 1.0f);
    check (std::abs (rmsGain (render (p, 0.1f), 0.1)-1.0) < 1e-5, "Ratio 1 not unity");
    set (p, "compressionMode", 1.0f);
    auto dualUp = render (p, 0.125f);
    auto dualBelow = render (p, 0.02f);
    auto dualDown = render (p, 0.5f);
    check (rmsGain (dualUp, 0.125) > 1.01, "Dual signal above UP and below DOWN not raised");
    check (std::abs (rmsGain (dualBelow, 0.02)-1.0) < 1e-5, "Dual UP gate raises material below threshold");
    check (rmsGain (dualDown, 0.5) < 0.99, "Dual strong signal not reduced");
    check (harmonicDb (dualUp) < -110.0 && harmonicDb (dualDown) < -110.0, "Dual harmonic regression");
    const auto bypass = render (p, 0.1f, 256, true);
    double error = 0.0;
    for (size_t i = 15000; i < bypass.output.size(); ++i)
    {
        const auto expected = 0.1f * static_cast<float> (std::sin (2.0*pi*400.0*(static_cast<double> (i)-bypass.latency)/testSampleRate));
        error = std::max (error, std::abs (static_cast<double> (bypass.output[i]-expected)));
    }
    check (bypass.latency == 1248 && error < 1e-6, "Bypass/PDC delayed dry mismatch");
    set (p, "mix", 0.0f);
    auto dry = render (p, 0.02f);
    check (std::abs (rmsGain (dry, 0.02)-1.0) < 1e-5 && std::abs (dry.meter) < 1e-5, "Zero Mix not unity/zero dynamics meter");
    set (p,"mix",100.0f); set(p,"keySource",1.0f);
    check(std::abs(rmsGain(render(p,0.02f),0.02)-1.0)<1e-5,"Missing EXT key must preserve unity carrier");
    std::cout << "PASS: compiled processor single/dual, collapsed/off regions, gain signs, ratio1, Mix0 and bypass/PDC.\n";
    std::cout << "THD @400Hz/26ms, H2-H7: single down " << harmonicDb (down) << " dB; single up " << singleUpThd
              << " dB; dual up " << harmonicDb (dualUp) << " dB; dual down " << harmonicDb (dualDown) << " dB.\n";
}
void stateChecks()
{
    QQSuperCompressionAudioProcessor p;
    baseline (p);
    set (p, "ratio", 0.125f); set (p, "rangeLDb", -7.0f);
    set (p, "upThresholdMDb", -33.0f); set (p, "downThresholdMDb", -9.0f);
    set (p, "upRatioM", 0.25f); set (p, "downRatioS", 21.0f);
    set (p, "compressionMode", 1.0f);
    p.copyAToB();
    set (p, "compressionMode", 0.0f); set (p, "upRatioM", 0.5f);
    p.selectABSlot (1);
    check (get (p,"compressionMode") == 1.0f && std::abs (get(p,"upRatioM")-0.25f)<0.001f, "A/B lost dual settings");
    juce::MemoryBlock data;
    p.getStateInformation (data);
    QQSuperCompressionAudioProcessor restored;
    restored.setStateInformation (data.getData(), static_cast<int> (data.getSize()));
    for (const char* id : {"ratio", "rangeLDb", "upThresholdMDb", "downThresholdMDb", "upRatioM", "downRatioS", "compressionMode"})
        check (std::abs (get(p,id)-get(restored,id))<0.001f, "State roundtrip lost new parameter");
    std::cout << "PASS: separate single/dual banks, A/B and host state roundtrip.\n";
}
void blockChecks()
{
    QQSuperCompressionAudioProcessor p, q;
    baseline(p); baseline(q); set(p,"compressionMode",1); set(q,"compressionMode",1);
    set(p,"upThresholdDb",-60); set(q,"upThresholdDb",-60);
    const auto a = render(p,0.02f,64), b = render(q,0.02f,511);
    double error=0;
    for (size_t i=0;i<a.output.size();++i) error=std::max(error,std::abs(static_cast<double>(a.output[i]-b.output[i])));
    check(error<1e-6,"Output depends on host block size");
    std::cout << "PASS: 64/511-sample host block output parity; peak error " << error << ".\n";
}
void domainChecks()
{
    QQSuperCompressionAudioProcessor p;
    baseline(p);
    set(p,"compressionMode",1);
    for(const char* id : {"upThresholdLDb","upThresholdRDb","upThresholdMDb","upThresholdSDb"}) set(p,id,-60);
    for(const char* id : {"downThresholdLDb","downThresholdRDb","downThresholdMDb","downThresholdSDb"}) set(p,id,-12);
    for(const char* id : {"upRatioL","upRatioR","upRatioM","upRatioS"}) set(p,id,0.125f);
    set(p,"processingMode",2);
    set(p,"upRatioR",1.0f);
    auto lr=render(p,0.02f);
    check(rmsGain(lr,0.02)>1.01,"LR left boost missing");
    AudioResult right; right.output=lr.right;
    check(std::abs(rmsGain(right,0.02)-1.0)<1e-5,"LR ratio independence lost");
    set(p,"processingMode",1);
    const auto mid=render(p,0.02f);
    const auto side=render(p,0.02f,256,false,400,testSampleRate,-1.0f);
    check(rmsGain(mid,0.02)>1.01 && rmsGain(side,0.02)>1.01,"MS independent boosts missing");
    for(size_t i=15000;i<side.output.size();++i)
        check(std::abs(side.output[i]+side.right[i])<1e-6,"Side decode stereo polarity changed");
    std::cout<<"PASS: LR independent ratios, pure Mid/Side processing and decode polarity.\n";
}
void signedMixPeakCheck()
{
    QQSuperCompressionAudioProcessor p;
    baseline(p); set(p,"compressionMode",1); set(p,"upThresholdDb",-50); set(p,"downThresholdDb",-12);
    set(p,"upRatio",1.0f/32.0f); set(p,"downRatio",32); set(p,"mix",50);
    set(p,"lookaheadMs",0); set(p,"oversampling",0);
    p.setRateAndBufferSizeDetails(testSampleRate,512); p.prepareToPlay(testSampleRate,512);
    juce::AudioBuffer<float> audio(2,512); juce::MidiBuffer midi;
    for(int i=0;i<512;++i) for(int channel=0;channel<2;++channel) audio.setSample(channel,i,i<256 ? 0.02f : 0.9f);
    p.processBlock(audio,midi);
    float expected=0;
    for(int i=0;i<512;++i)
    {
        const auto gain=audio.getSample(0,i)/(i<256 ? 0.02f : 0.9f);
        const auto signedGr=-juce::Decibels::gainToDecibels(gain);
        if(std::abs(signedGr)>std::abs(expected)) expected=signedGr;
    }
    check(expected<0,"Mixed extrema fixture does not favour boost");
    check(std::abs(p.getMeterState().gainReductionDb0.load()-expected)<0.001f,"Signed peak must be selected AFTER Mix");
    check(std::abs(p.getMeterState().gainReductionHoldDb0.load()-expected)<0.001f,"Signed Hold differs from effective peak");
    p.releaseResources();
    std::cout<<"PASS: same-block upward/downward extrema and Hold selected after Mix (signed GR "<<expected<<" dB).\n";
}
void additionalToneChecks()
{
    QQSuperCompressionAudioProcessor p; baseline(p); set(p,"compressionMode",1); set(p,"upRatio",1.0f/32.0f);
    set(p,"upThresholdDb",-60);
    for(const auto rate : {44100.0,48000.0,96000.0})
    {
        const auto frequency=rate/100.0;
        auto result=render(p,0.02f,256,false,frequency,rate);
        check(harmonicDb(result,frequency,rate)<-110.0,"Sample rate harmonic regression");
    }
    set(p,"lookaheadMs",80);
    auto low=render(p,0.02f,256,false,20);
    const auto lowThd=harmonicDb(low,20);
    check(lowThd<-110,"20 Hz lookahead harmonic regression");
    std::cout<<"PASS: 44.1/48/96kHz steady upward; 20Hz/80ms H2-H7 THD "<<lowThd<<" dB.\n";
    baseline(p); set(p,"compressionMode",0);
    const auto upper=juce::Decibels::decibelsToGain(-6.0f);
    const auto downInside=p.getDynamicsGainForDomain(upper-1e-6f,0);
    check(p.getDynamicsGainForDomain(upper,0)==1.0f,"Exact Range boundary not unity");
    set(p,"ratio",0.125f);
    const auto lower=juce::Decibels::decibelsToGain(-40.0f);
    const auto upInside=p.getDynamicsGainForDomain(lower+1e-6f,0);
    check(p.getDynamicsGainForDomain(lower,0)==1.0f,"Exact Threshold boundary not unity");
    check(std::abs(downInside-1)<1e-6f && std::abs(upInside-1)<1e-5f,"Boundary gain jump regression");
    std::cout<<"PASS: continuous boundary residual with test settings: down at Range "
             <<-juce::Decibels::gainToDecibels(downInside)<<" dB; up at Threshold "
             <<juce::Decibels::gainToDecibels(upInside)<<" dB. These are not steady-tone THD results.\n";
}
void revisionTwoDefaultsAndRangeOff()
{
    QQSuperCompressionAudioProcessor p;
    for(int domain=0;domain<5;++domain)
    {
        check(p.getBoundaryForDomainDb(true,false,domain)==qqsc::params::thresholdOffDb,"Dual UP default must be -inf");
        check(p.getBoundaryForDomainDb(true,true,domain)==0.0f,"Dual DOWN default must be 0 dB");
        check(p.getBoundaryForDomainDb(false,true,domain)==qqsc::params::rangeOffDb,"Single Range default must be OFF");
    }
    // APVTS initialises raw values through the parameter's logarithmic
    // normalised range. Print the actual round-trip error before accepting only
    // a few float ULPs; a changed product default remains a hard failure.
    const auto priorPrecision = std::cout.precision();
    std::cout << std::setprecision (std::numeric_limits<float>::max_digits10);
    const char* defaultRatioIds[] { "ratio", "upRatio", "downRatio" };
    const float expectedDefaultRatios[] { 1.0f, 1.0f, 1.0f };
    bool defaultsWithinFloatPrecision = true;
    for (int i = 0; i < 3; ++i)
    {
        const auto actual = get (p, defaultRatioIds[i]);
        const auto expected = expectedDefaultRatios[i];
        const auto tolerance = 8.0f * std::numeric_limits<float>::epsilon() * expected;
        auto* parameter = p.getAPVTS().getParameter (defaultRatioIds[i]);
        const auto normalisedDefault = parameter->getDefaultValue();
        const auto decodedDefault = parameter->convertFrom0to1 (normalisedDefault);
        std::cout << "DEFAULT: " << defaultRatioIds[i] << " actual=" << actual
                  << " expected=" << expected << " abs_error=" << std::abs (actual - expected)
                  << " tolerance=" << tolerance << " normalised=" << normalisedDefault
                  << " decoded=" << decodedDefault << "\n";
        defaultsWithinFloatPrecision = defaultsWithinFloatPrecision
            && std::isfinite (actual) && std::abs (actual - expected) <= tolerance
            && std::abs (decodedDefault - expected) <= tolerance;
    }
    std::cout.precision (priorPrecision);
    check (defaultsWithinFloatPrecision, "Ratio defaults changed beyond logarithmic float round-trip precision");
    set(p,"compressionMode",1);
    for(float level : {0.0f,0.001f,0.1f,0.9f,1.0f})
        check(p.getDynamicsGainForDomain(level,0)==1.0f,"Default Dual must stay inactive until thresholds move");
    set(p,"compressionMode",0); set(p,"ratio",2); set(p,"thresholdDb",-120);
    set(p,"rangeDb",qqsc::params::rangeOffDb);
    check(std::abs(p.getDynamicsGainForDomain(1.0f,0)-0.5f)<1e-6,"OFF Range still cuts off at 0dB");
    for(const auto rate : {44100.0,48000.0,96000.0})
    for(const float preview : {10.0f,26.0f,80.0f})
    {
        set(p,"lookaheadMs",preview); set(p,"oversampling",0);
        auto result=render(p,1.0f,256,false,599.9,rate);
        qqsc::StaticCompressionEngine reference;
        reference.prepare(static_cast<int>(std::ceil(rate*0.1)));
        reference.setLookaheadSamples(result.latency);
        std::vector<float> input(result.output.size());
        double maximumError=0;
        int unityBursts=0;
        for(size_t i=0;i<input.size();++i)
        {
            input[i]=static_cast<float>(std::sin(2*pi*599.9*static_cast<double>(i)/rate));
            const float legacyGain=reference.processSample(input[i],2.0f,0.0f,static_cast<int64_t>(i));
            const float dry=i>=static_cast<size_t>(result.latency) ? input[i-static_cast<size_t>(result.latency)] : 0.0f;
            const float expected=dry*legacyGain;
            maximumError=std::max(maximumError,std::abs(static_cast<double>(result.output[i]-expected)));
            if(i>static_cast<size_t>(rate*0.2) && std::abs(dry)>0.25f && std::abs(result.output[i]/dry)>0.9f) ++unityBursts;
        }
        check(maximumError<1e-6,"599.9Hz RangeOFF differs from retained 1.1.9 down law");
        check(unityBursts==0,"599.9Hz fullscale still has intermittent unity bursts");
        std::cout<<"PASS: 599.9Hz fullscale, "<<rate<<"Hz/"<<preview<<"ms RangeOFF: zero unity bursts, legacy peak error "<<maximumError<<".\n";
    }
    // Finite zero and OFF must remain distinct across state restore.
    for(float upper : {0.0f,qqsc::params::rangeOffDb})
    {
        set(p,"rangeDb",upper); juce::MemoryBlock data; p.getStateInformation(data);
        QQSuperCompressionAudioProcessor restored;
        restored.setStateInformation(data.getData(),static_cast<int>(data.getSize()));
        check(restored.getBoundaryForDomainDb(false,true,0)==upper,"State conflates finite0 with RangeOFF");
        const auto gain=restored.getDynamicsGainForDomain(1.0f,0);
        check(std::abs(gain-(upper==0.0f ? 1.0f : 0.5f))<1e-6,"Restored Range gate differs");
    }
    std::cout<<"PASS: corrected Dual -inf/0 defaults, Ratio defaults and distinct finite0/OFF state.\n";
}
void legacySchemaMigrationChecks()
{
    const auto captureTree = [] (QQSuperCompressionAudioProcessor& p)
    {
        juce::MemoryBlock binary;
        p.getStateInformation (binary);
        auto xml = juce::AudioProcessor::getXmlFromBinary (binary.getData(), static_cast<int> (binary.getSize()));
        check (xml != nullptr, "Migration fixture could not decode native state");
        return juce::ValueTree::fromXml (*xml);
    };
    const auto restoreTree = [] (QQSuperCompressionAudioProcessor& p, const juce::ValueTree& state)
    {
        auto xml = state.createXml();
        check (xml != nullptr, "Migration fixture could not encode state");
        juce::MemoryBlock binary;
        juce::AudioProcessor::copyXmlToBinary (*xml, binary);
        p.setStateInformation (binary.getData(), static_cast<int> (binary.getSize()));
    };
    const auto isNewParameter = [] (const juce::String& id)
    {
        if (id == qqsc::params::compressionMode || id == qqsc::params::dualRatioLink) return true;
        for (const auto& ids : { qqsc::params::rangeIds, qqsc::params::upThresholdIds,
                                 qqsc::params::downThresholdIds, qqsc::params::upRatioIds,
                                 qqsc::params::downRatioIds })
            for (const auto* candidate : ids)
                if (id == candidate) return true;
        return false;
    };
    QQSuperCompressionAudioProcessor legacySource;
    for (size_t d = 0; d < 5; ++d)
    {
        set (legacySource, qqsc::params::ratioIds[d], 2.0f);
        legacySource.setBoundaryForDomainDb (false, false, static_cast<int> (d), -120.0f);
    }
    legacySource.copyAToB();
    auto schema10 = captureTree (legacySource);
    schema10.setProperty ("qqscStateSchemaVersion", 10, nullptr);
    int removedParameters = 0, removedABProperties = 0;
    for (int i = schema10.getNumChildren(); --i >= 0;)
        if (isNewParameter (schema10.getChild (i).getProperty ("id").toString()))
        {
            schema10.removeChild (i, nullptr);
            ++removedParameters;
        }
    for (int i = schema10.getNumProperties(); --i >= 0;)
    {
        const auto name = schema10.getPropertyName (i);
        const auto text = name.toString();
        if ((text.startsWith ("qqscAB_A_") || text.startsWith ("qqscAB_B_"))
            && isNewParameter (text.substring (9)))
        {
            schema10.removeProperty (name, nullptr);
            ++removedABProperties;
        }
    }
    check (removedParameters == 27 && removedABProperties == 52,
           "Schema10 fixture did not remove all newly introduced sound state");
    QQSuperCompressionAudioProcessor target10;
    // Restore into a deliberately dirty current instance: missing parameters
    // must receive migration defaults rather than retain these previous edits.
    set (target10, qqsc::params::compressionMode, 1.0f);
    for (size_t d = 0; d < 5; ++d)
    {
        target10.setBoundaryForDomainDb (false, true, static_cast<int> (d), -6.0f);
        target10.setBoundaryForDomainDb (true, false, static_cast<int> (d), -30.0f);
        target10.setBoundaryForDomainDb (true, true, static_cast<int> (d), -10.0f);
        set (target10, qqsc::params::upRatioIds[d], 0.25f);
        set (target10, qqsc::params::downRatioIds[d], 16.0f);
    }
    restoreTree (target10, schema10);
    for (int slot : { 0, 1, 0 })
    {
        target10.selectABSlot (slot);
        check (get (target10, qqsc::params::compressionMode) == 0.0f,
               "Schema10 mode migration failed in project or A/B state");
        for (size_t d = 0; d < 5; ++d)
        {
            const auto domain = static_cast<int> (d);
            check (target10.getBoundaryForDomainDb (false, true, domain) == qqsc::params::rangeOffDb,
                   "Schema10 missing Range must migrate to OFF in every domain and A/B slot");
            check (target10.getBoundaryForDomainDb (true, false, domain) == qqsc::params::thresholdOffDb
                   && target10.getBoundaryForDomainDb (true, true, domain) == 0.0f,
                   "Schema10 missing dual thresholds retained dirty values");
            check (std::abs (get (target10, qqsc::params::upRatioIds[d]) - 1.0f) < 1e-6f
                   && std::abs (get (target10, qqsc::params::downRatioIds[d]) - 1.0f) < 1e-6f,
                   "Schema10 missing dual Ratio defaults were not restored");
            check (std::abs (get (target10, qqsc::params::ratioIds[d]) - 2.0f) < 1e-6f
                   && std::abs (target10.getDynamicsGainForDomain (1.0f, domain) - 0.5f) < 1e-6f,
                   "Schema10 existing Ratio or legacy fullscale downward gain changed");
        }
    }
    // A real schema11 state already contains finite Range=0 and independent
    // dual values. All of those stored values must survive schema12 defaults.
    QQSuperCompressionAudioProcessor source11;
    for (size_t d = 0; d < 5; ++d)
    {
        source11.setBoundaryForDomainDb (false, true, static_cast<int> (d), 0.0f);
        source11.setBoundaryForDomainDb (true, false, static_cast<int> (d), -24.0f);
        source11.setBoundaryForDomainDb (true, true, static_cast<int> (d), -12.0f);
        set (source11, qqsc::params::ratioIds[d], 2.0f);
        set (source11, qqsc::params::upRatioIds[d], 0.25f);
        set (source11, qqsc::params::downRatioIds[d], 16.0f);
    }
    source11.copyAToB();
    auto schema11 = captureTree (source11);
    schema11.setProperty ("qqscStateSchemaVersion", 11, nullptr);
    QQSuperCompressionAudioProcessor target11;
    restoreTree (target11, schema11);
    for (int slot : { 0, 1, 0 })
    {
        target11.selectABSlot (slot);
        for (size_t d = 0; d < 5; ++d)
        {
            const auto domain = static_cast<int> (d);
            check (target11.getBoundaryForDomainDb (false, true, domain) == 0.0f,
                   "Schema11 finite Range0 was silently migrated to OFF");
            check (target11.getDynamicsGainForDomain (1.0f, domain) == 1.0f
                   && target11.getDynamicsGainForDomain (0.5f, domain) < 0.99f,
                   "Schema11 finite Range0 lost its strict upper boundary");
            check (std::abs (target11.getBoundaryForDomainDb (true, false, domain) + 24.0f) < 0.001f
                   && std::abs (target11.getBoundaryForDomainDb (true, true, domain) + 12.0f) < 0.001f
                   && std::abs (get (target11, qqsc::params::upRatioIds[d]) - 0.25f) < 1e-6f
                   && std::abs (get (target11, qqsc::params::downRatioIds[d]) - 16.0f) < 1e-6f,
                   "Schema11 saved dual values were replaced by new defaults");
        }
    }
    std::cout << "PASS: schema10 missing parameters migrate on dirty instance and A/B; schema11 finite0 and saved dual banks survive schema12.\n";
}
void upwardThresholdStrength()
{
    QQSuperCompressionAudioProcessor p; baseline(p); set(p,"compressionMode",1); set(p,"downThresholdDb",0);
    set(p,"upRatio",0.125f);
    for(float threshold : {-20.0f,-40.0f,-60.0f})
    {
        set(p,"upThresholdDb",threshold-20.0f);
        set(p,"downThresholdDb",threshold);
        const auto amplitude=juce::Decibels::decibelsToGain(threshold-10.0f);
        const auto result=render(p,amplitude);
        const auto lift=20.0*std::log10(rmsGain(result,amplitude));
        check(std::abs(lift-7.9219786)<0.005,"Upward strength incorrectly shrinks with absolute threshold");
        check(harmonicDb(result)<-110,"Threshold-relative upward harmonic regression");
        std::cout<<"PASS: UP gate "<<threshold-20.0f<<"dB, DOWN "<<threshold<<"dB, input10dBbelowDOWN, Ratio1:8 -> "<<lift<<"dB lift.\n";
    }
}
void upwardGateAndUnityDefaults()
{
    QQSuperCompressionAudioProcessor p;
    for (const auto& ids : {qqsc::params::ratioIds, qqsc::params::upRatioIds, qqsc::params::downRatioIds})
        for (const auto* id : ids)
            check(std::abs(get(p,id)-1.0f)<1e-6f,"Every Ratio domain must default to 1:1");
    check(get(p,qqsc::params::dualRatioLink)==1.0f,"First-use Dual Ratio LINK default must be ON");
    set(p,"compressionMode",1);
    set(p,"upRatio",0.125f);
    check(p.getDynamicsGainForDomain(0.1f,0)>1.0f,"UP=-inf is an open gate, not upward disable");
    for(int d=0;d<5;++d)
    {
        p.setBoundaryForDomainDb(true,false,d,-40);
        p.setBoundaryForDomainDb(true,true,d,-10);
        set(p,qqsc::params::upRatioIds[static_cast<size_t>(d)],0.125f);
        set(p,qqsc::params::downRatioIds[static_cast<size_t>(d)],8);
        const auto lower=juce::Decibels::decibelsToGain(-40.0f);
        const auto upper=juce::Decibels::decibelsToGain(-10.0f);
        check(p.getDynamicsGainForDomain(lower*0.5f,d)==1 && p.getDynamicsGainForDomain(lower,d)==1,
              "Dual UP gate must leave below/equal threshold untouched");
        check(p.getDynamicsGainForDomain(lower*2,d)>1,"Dual UP must raise material ABOVE its threshold");
        check(std::abs(p.getDynamicsGainForDomain(upper,d)-1)<1e-6,"UP/DOWN handoff must be unity at DOWN");
        const auto down=p.getDynamicsGainForDomain(0.8f,d);
        set(p,qqsc::params::upRatioIds[static_cast<size_t>(d)],1.0f/32.0f);
        check(down<1 && std::abs(p.getDynamicsGainForDomain(0.8f,d)-down)<1e-6,
              "Above DOWN, Up Ratio must not affect downward gain");
    }
    std::cout<<"PASS: all 15 unity defaults; UP is a lower gate; exact DOWN handoff and independent downward branch across five domains.\n";
}
#include "branch_revision4_checks.h"
#include "up1000_checks.h"
}
int main()
{
    juce::ScopedJuceInitialiser_GUI initialiser;
    try { collisionChecks(); audioChecks(); stateChecks(); blockChecks(); domainChecks(); signedMixPeakCheck(); additionalToneChecks(); revisionTwoDefaultsAndRangeOff(); legacySchemaMigrationChecks(); upwardThresholdStrength(); upwardGateAndUnityDefaults(); boundaryContinuityChecks(); branchEnableStateChecks(); branchCrossfadeChecks(); rangeCrossingAudioChecks(); branchOversampledCrossfadeChecks(); up1000Checks(); return 0; }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
