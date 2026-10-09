#include "PluginProcessor.h"
#include <JuceHeader.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

static void setParameter (QQSuperCompressionAudioProcessor& processor, const char* id, float value)
{
    auto* parameter = processor.getAPVTS().getParameter (id);
    if (parameter == nullptr) throw std::runtime_error (id);
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI initialiser;
    try
    {
        if (argc < 9 || argc > 12)
            throw std::runtime_error ("Usage: QQSCClickReplay input.wav output.f32 block ratio makeup lookahead oversampling mode [mixPercent] [detectorMode] [windowMs]");
        const juce::File input (juce::String::fromUTF8 (argv[1]));
        juce::WavAudioFormat format;
        auto reader = std::unique_ptr<juce::AudioFormatReader> (format.createReaderFor (input.createInputStream().release(), true));
        if (reader == nullptr || reader->numChannels != 2 || reader->sampleRate != 48000.0)
            throw std::runtime_error ("Expected a stereo 48 kHz WAV");
        std::ofstream output (argv[2], std::ios::binary);
        if (! output) throw std::runtime_error ("Cannot open output");
        const int blockSize = std::stoi (argv[3]);
        const float ratio = std::stof (argv[4]);
        const float makeup = std::stof (argv[5]);
        const float lookahead = std::stof (argv[6]);
        const float oversampling = std::stof (argv[7]);
        const float mode = std::stof (argv[8]);
        const float mix = argc >= 10 ? std::stof (argv[9]) : 100.0f;
        const float detector = argc >= 11 ? std::stof (argv[10]) : 0.0f;
        const float window = argc >= 12 ? std::stof (argv[11]) : 100.0f;
        QQSuperCompressionAudioProcessor processor;
        setParameter (processor, "algorithmMode", 1.0f);
        setParameter (processor, "compressionMode", 0.0f);
        setParameter (processor, "processingMode", mode);
        setParameter (processor, "ratio", ratio);
        setParameter (processor, "ratioL", ratio);
        setParameter (processor, "ratioR", ratio);
        setParameter (processor, "thresholdDb", -120.0f);
        setParameter (processor, "thresholdLDb", -120.0f);
        setParameter (processor, "thresholdRDb", -120.0f);
        setParameter (processor, "rangeDb", qqsc::params::rangeOffDb);
        setParameter (processor, "rangeLDb", qqsc::params::rangeOffDb);
        setParameter (processor, "rangeRDb", qqsc::params::rangeOffDb);
        setParameter (processor, "makeupGainDb", makeup);
        setParameter (processor, "inputGainDb", 0.0f);
        setParameter (processor, "outputGainDb", 0.0f);
        setParameter (processor, "mix", mix);
        setParameter (processor, qqsc::params::detectorMode, detector);
        setParameter (processor, qqsc::params::detectorWindowMs, window);
        setParameter (processor, "lookaheadMs", lookahead);
        setParameter (processor, "oversampling", oversampling);
        processor.setRateAndBufferSizeDetails (reader->sampleRate, blockSize);
        processor.prepareToPlay (reader->sampleRate, blockSize);
        juce::MidiBuffer midi;
        const int total = static_cast<int> (reader->lengthInSamples);
        double maxInput = 0.0, maxOutput = 0.0;
        for (int offset = 0; offset < total; offset += blockSize)
        {
            const int count = std::min (blockSize, total - offset);
            juce::AudioBuffer<float> audio (2, count);
            if (! reader->read (&audio, 0, count, offset, true, true))
                throw std::runtime_error ("WAV read failed");
            for (int i = 0; i < count; ++i)
                for (int c = 0; c < 2; ++c)
                    maxInput = std::max (maxInput, double (std::abs (audio.getSample (c, i))));
            processor.processBlock (audio, midi);
            for (int i = 0; i < count; ++i)
                for (int c = 0; c < 2; ++c)
                {
                    const float sample = audio.getSample (c, i);
                    if (! std::isfinite (sample)) throw std::runtime_error ("Non-finite output");
                    maxOutput = std::max (maxOutput, double (std::abs (sample)));
                    output.write (reinterpret_cast<const char*> (&sample), sizeof (sample));
                }
        }
        output.close();
        std::cout << "samples=" << total << " block=" << blockSize
                   << " ratio=" << ratio << " makeup=" << makeup << " mix=" << mix << " detector=" << detector
                   << " oversampling=" << oversampling << " mode=" << mode
                  << " latency=" << processor.getLatencySamples()
                  << " input_peak=" << maxInput << " output_peak=" << maxOutput << '\n';
        processor.releaseResources();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
