#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <limits>

namespace qqsc::params
{
    inline constexpr auto inputGainDb    = "inputGainDb";
    inline constexpr auto ratio          = "ratio";          // ST / legacy Ratio
    inline constexpr auto ratioL         = "ratioL";
    inline constexpr auto ratioR         = "ratioR";
    inline constexpr auto ratioM         = "ratioM";
    inline constexpr auto ratioS         = "ratioS";
    inline constexpr auto makeupGainDb   = "makeupGainDb";   // ST / legacy shared Makeup
    inline constexpr auto makeupGainLDb  = "makeupGainLDb";
    inline constexpr auto makeupGainRDb  = "makeupGainRDb";
    inline constexpr auto makeupGainMDb  = "makeupGainMDb";
    inline constexpr auto makeupGainSDb  = "makeupGainSDb";
    inline constexpr auto mix            = "mix";            // ST / legacy Mix
    inline constexpr auto mixL           = "mixL";
    inline constexpr auto mixR           = "mixR";
    inline constexpr auto mixM           = "mixM";
    inline constexpr auto mixS           = "mixS";
    inline constexpr auto outputGainDb   = "outputGainDb";
    inline constexpr auto lookaheadMs    = "lookaheadMs";
    inline constexpr auto oversampling   = "oversampling";
    inline constexpr auto processingMode = "processingMode";
    inline constexpr auto bypass         = "bypass";
    inline constexpr auto thresholdDb    = "thresholdDb";    // ST / legacy Threshold
    inline constexpr auto thresholdLDb   = "thresholdLDb";
    inline constexpr auto thresholdRDb   = "thresholdRDb";
    inline constexpr auto thresholdMDb   = "thresholdMDb";
    inline constexpr auto thresholdSDb   = "thresholdSDb";
    inline constexpr auto domainLink     = "domainLink";
    // v1.1.x: append Side Chain parameters after every v1.0.4 parameter so
    // existing host-facing IDs/order and old projects remain safe.
    inline constexpr auto keySource      = "keySource";
    inline constexpr auto keyGainDb      = "keyGainDb";
    inline constexpr auto keyHpfHz       = "keyHpfHz";


    // v1.2.0 parameters are appended after the complete v1.1.9 sequence.
    inline constexpr auto compressionMode = "compressionMode";
    inline constexpr auto rangeDb = "rangeDb";
    inline constexpr auto rangeLDb = "rangeLDb";
    inline constexpr auto rangeRDb = "rangeRDb";
    inline constexpr auto rangeMDb = "rangeMDb";
    inline constexpr auto rangeSDb = "rangeSDb";
    inline constexpr auto upThresholdDb = "upThresholdDb";
    inline constexpr auto upThresholdLDb = "upThresholdLDb";
    inline constexpr auto upThresholdRDb = "upThresholdRDb";
    inline constexpr auto upThresholdMDb = "upThresholdMDb";
    inline constexpr auto upThresholdSDb = "upThresholdSDb";
    inline constexpr auto downThresholdDb = "downThresholdDb";
    inline constexpr auto downThresholdLDb = "downThresholdLDb";
    inline constexpr auto downThresholdRDb = "downThresholdRDb";
    inline constexpr auto downThresholdMDb = "downThresholdMDb";
    inline constexpr auto downThresholdSDb = "downThresholdSDb";
    inline constexpr auto upRatio = "upRatio";
    inline constexpr auto dualRatioLink = "dualRatioLink";
    inline constexpr auto upRatioL = "upRatioL";
    inline constexpr auto upRatioR = "upRatioR";
    inline constexpr auto upRatioM = "upRatioM";
    inline constexpr auto upRatioS = "upRatioS";
    inline constexpr auto downRatio = "downRatio";
    inline constexpr auto downRatioL = "downRatioL";
    inline constexpr auto downRatioR = "downRatioR";
    inline constexpr auto downRatioM = "downRatioM";
    inline constexpr auto downRatioS = "downRatioS";

    // Rev4: appended branch enables; saved ratios and thresholds stay intact.
    inline constexpr auto upEnabled = "upEnabled";
    inline constexpr auto upEnabledL = "upEnabledL";
    inline constexpr auto upEnabledR = "upEnabledR";
    inline constexpr auto upEnabledM = "upEnabledM";
    inline constexpr auto upEnabledS = "upEnabledS";
    inline constexpr std::array<const char*, 5> upEnabledIds { upEnabled, upEnabledL, upEnabledR, upEnabledM, upEnabledS };
    inline constexpr auto downEnabled = "downEnabled";
    inline constexpr auto downEnabledL = "downEnabledL";
    inline constexpr auto downEnabledR = "downEnabledR";
    inline constexpr auto downEnabledM = "downEnabledM";
    inline constexpr auto downEnabledS = "downEnabledS";
    inline constexpr std::array<const char*, 5> downEnabledIds { downEnabled, downEnabledL, downEnabledR, downEnabledM, downEnabledS };

    enum CompressionMode { singleCompression = 0, dualCompression = 1 };
    inline constexpr std::array<const char*, 5> thresholdIds { thresholdDb, thresholdLDb, thresholdRDb, thresholdMDb, thresholdSDb };
    inline constexpr std::array<const char*, 5> rangeIds { rangeDb, rangeLDb, rangeRDb, rangeMDb, rangeSDb };
    inline constexpr std::array<const char*, 5> upThresholdIds { upThresholdDb, upThresholdLDb, upThresholdRDb, upThresholdMDb, upThresholdSDb };
    inline constexpr std::array<const char*, 5> downThresholdIds { downThresholdDb, downThresholdLDb, downThresholdRDb, downThresholdMDb, downThresholdSDb };
    inline constexpr std::array<const char*, 5> ratioIds { ratio, ratioL, ratioR, ratioM, ratioS };
    inline constexpr std::array<const char*, 5> upRatioIds { upRatio, upRatioL, upRatioR, upRatioM, upRatioS };
    inline constexpr std::array<const char*, 5> downRatioIds { downRatio, downRatioL, downRatioR, downRatioM, downRatioS };

    inline juce::NormalisableRange<float> dynamicsRatioRange (float minimum = 1.0f / 32.0f, float maximum = 32.0f)
    {
        return { minimum, maximum,
                 [] (float lo, float hi, float normalised) { return lo * std::pow (hi / lo, normalised); },
                 [] (float lo, float hi, float value) { return std::log (juce::jlimit (lo, hi, value) / lo) / std::log (hi / lo); },
                 [] (float lo, float hi, float value) { return juce::jlimit (lo, hi, value); } };
    }

    inline juce::String dynamicsRatioText (float value)
    {
        if (value < 0.99999f)
            return "1:" + juce::String (1.0f / value, 2);
        return juce::String (value, value < 10.0f ? 2 : 1) + ":1";
    }

    inline float dynamicsRatioFromText (const juce::String& text)
    {
        if (text.containsChar (':'))
        {
            const auto left = text.upToFirstOccurrenceOf (":", false, false).getFloatValue();
            const auto right = text.fromFirstOccurrenceOf (":", false, false).getFloatValue();
            return right > 0.0f ? left / right : 1.0f;
        }
        if (text.containsChar ('/'))
        {
            const auto left = text.upToFirstOccurrenceOf ("/", false, false).getFloatValue();
            const auto right = text.fromFirstOccurrenceOf ("/", false, false).getFloatValue();
            return right > 0.0f ? left / right : 1.0f;
        }
        return text.getFloatValue();
    }

    // Threshold OFF is represented by the bottom endpoint. DSP maps this sentinel
    // to a true zero-linear threshold, i.e. the exact pre-Threshold (-inf) law.
    inline constexpr float thresholdOffDb = -120.0f;
    // Virtual Range endpoint: +1 is a stored OFF sentinel, never physical dB.
    // Finite 0 dB remains a distinct strict upper cutoff in saved projects.
    inline constexpr float rangeOffDb = 1.0f;
    inline constexpr float keyHpfOffHz = 0.0f;
    inline constexpr float keyHpfMinHz = 20.0f;
    inline constexpr float keyHpfMaxHz = 500.0f;

    inline bool isThresholdEnabled (float db) noexcept
    {
        return db > thresholdOffDb + 0.0001f;
    }

    inline float thresholdLinear (float db) noexcept
    {
        if (! isThresholdEnabled (db))
            return 0.0f;

        return juce::Decibels::decibelsToGain (juce::jlimit (thresholdOffDb, 0.0f, db));
    }

    inline bool isRangeEnabled (float db) noexcept
    {
        return db <= 0.0f;
    }

    inline float clampRangeDb (float db) noexcept
    {
        return isRangeEnabled (db) ? juce::jlimit (thresholdOffDb, 0.0f, db) : rangeOffDb;
    }

    inline float rangeLinear (float db) noexcept
    {
        return isRangeEnabled (db) ? thresholdLinear (db) : std::numeric_limits<float>::infinity();
    }

    inline juce::String rangeText (float db)
    {
        if (! isRangeEnabled (db)) return "OFF";
        if (! isThresholdEnabled (db)) return "-inf dB";
        return juce::String (db, 2) + " dB";
    }

    inline float rangeFromText (const juce::String& text)
    {
        if (text.containsIgnoreCase ("off") || text.containsIgnoreCase ("+inf")) return rangeOffDb;
        if (text.containsIgnoreCase ("-inf")) return thresholdOffDb;
        return juce::jlimit (thresholdOffDb, 0.0f, text.getFloatValue());
    }

    inline juce::NormalisableRange<float> rangeParameterRange()
    {
        // The final 2% of travel is an explicit OFF detent; finite 0 dB remains
        // reachable immediately below it. No values between 0 and OFF are dB.
        constexpr float finiteEnd = 0.98f;
        return { thresholdOffDb, rangeOffDb,
                 [finiteEnd] (float, float, float normalised)
                 {
                     return normalised > finiteEnd ? rangeOffDb
                         : thresholdOffDb * (1.0f - normalised / finiteEnd);
                 },
                 [finiteEnd] (float, float, float value)
                 {
                     return isRangeEnabled (value)
                         ? finiteEnd * (1.0f - juce::jlimit (thresholdOffDb, 0.0f, value) / thresholdOffDb)
                         : 1.0f;
                 },
                 [] (float, float, float value)
                 {
                     return isRangeEnabled (value)
                         ? std::round (clampRangeDb (value) * 100.0f) * 0.01f
                         : rangeOffDb;
                 } };
    }

    inline bool isKeyHpfEnabled (float hz) noexcept
    {
        return hz >= keyHpfMinHz;
    }

    inline float clampKeyHpfHz (float hz) noexcept
    {
        return juce::jlimit (keyHpfMinHz, keyHpfMaxHz, hz);
    }

    inline juce::NormalisableRange<float> keyHpfRange()
    {
        // Reserve the first 2% of normalised travel for a true OFF position;
        // the active 20-500 Hz section then follows a perceptually useful log law.
        constexpr float activeStart = 0.02f;
        const auto logSpan = std::log (keyHpfMaxHz / keyHpfMinHz);

        return { keyHpfOffHz, keyHpfMaxHz,
                 [logSpan, activeStart] (float, float, float normalised)
                 {
                     if (normalised < activeStart)
                         return keyHpfOffHz;

                     const auto activeNormalised = (normalised - activeStart) / (1.0f - activeStart);
                     return keyHpfMinHz * std::exp (logSpan * activeNormalised);
                 },
                 [logSpan, activeStart] (float, float, float value)
                 {
                     if (! isKeyHpfEnabled (value))
                         return 0.0f;

                     const auto activeNormalised = std::log (clampKeyHpfHz (value) / keyHpfMinHz) / logSpan;
                     return activeStart + activeNormalised * (1.0f - activeStart);
                 },
                 [] (float, float, float value)
                 {
                     if (! isKeyHpfEnabled (value))
                         return keyHpfOffHz;

                     return std::round (clampKeyHpfHz (value));
                 } };
    }

    // Dynamic Display Dry is intentionally pre-Input-Gain, while the detector
    // sees post-Input-Gain audio. Shift the visual line into the Display reference.
    inline float effectiveDisplayThresholdDb (float threshold, float inputGain) noexcept
    {
        return threshold - inputGain;
    }

    enum ProcessingMode
    {
        stereoLinked = 0,
        midSide      = 1,
        leftRight    = 2
    };

    // LR/MS audition monitor is intentionally a local workflow state rather than
    // a host-automatable sound parameter. ALL preserves the normal stereo result;
    // FIRST/SECOND select L/R or M/S depending on the current processing mode.
    enum DomainMonitorSelection
    {
        monitorAll    = 0,
        monitorFirst  = 1,
        monitorSecond = 2
    };

    enum KeySource
    {
        keyInternal = 0,
        keyExternal = 1
    };


    inline constexpr std::array<float, 6> lookaheadPresetMs { 0.0f, 10.0f, 26.0f, 40.0f, 80.0f, 100.0f };
    inline constexpr std::array<int, 3> oversamplingFactors { 1, 8, 16 };
    inline constexpr std::array<int, 3> oversamplingStageCounts { 0, 3, 4 };

    inline juce::StringArray oversamplingChoices()
    {
        return { "1x", "8x", "16x" };
    }

    inline juce::String oversamplingNameForChoiceIndex (int index)
    {
        const auto choices = oversamplingChoices();
        index = juce::jlimit (0, choices.size() - 1, index);
        return choices[index];
    }

    inline int oversamplingFactorForChoiceIndex (int index) noexcept
    {
        index = juce::jlimit (0, static_cast<int> (oversamplingFactors.size()) - 1, index);
        return oversamplingFactors[static_cast<size_t> (index)];
    }

    inline int oversamplingStageCountForChoiceIndex (int index) noexcept
    {
        index = juce::jlimit (0, static_cast<int> (oversamplingStageCounts.size()) - 1, index);
        return oversamplingStageCounts[static_cast<size_t> (index)];
    }

    inline juce::StringArray lookaheadChoices()
    {
        return { "0 ms", "10 ms", "26 ms", "40 ms", "80 ms", "100 ms" };
    }

    inline int lookaheadChoiceIndexForMs (float ms) noexcept
    {
        int bestIndex = 0;
        auto bestDistance = std::abs (ms - lookaheadPresetMs[0]);

        for (int i = 1; i < static_cast<int> (lookaheadPresetMs.size()); ++i)
        {
            const auto distance = std::abs (ms - lookaheadPresetMs[static_cast<size_t> (i)]);
            // On an exact tie prefer the longer preset. This avoids migrating
            // a legacy non-zero Lookahead (notably 5 ms) down to the special
            // 0 ms distortion/flavour mode.
            if (distance <= bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return bestIndex;
    }

    inline float lookaheadMsForChoiceIndex (int index) noexcept
    {
        index = juce::jlimit (0, static_cast<int> (lookaheadPresetMs.size()) - 1, index);
        return lookaheadPresetMs[static_cast<size_t> (index)];
    }

    inline float snapLookaheadMs (float ms) noexcept
    {
        return lookaheadMsForChoiceIndex (lookaheadChoiceIndexForMs (ms));
    }

    inline int effectiveOversamplingChoiceIndex (float requestedLookaheadMs, int userChoiceIndex) noexcept
    {
        // Oversampling is intentionally a 0 ms flavour/anti-aliasing option only.
        // User PluginDoctor testing found no meaningful aliasing need at 10 ms
        // or longer, so every non-zero Lookahead always runs the Ratio core at 1x.
        if (snapLookaheadMs (requestedLookaheadMs) > 0.0001f)
            return 0;

        return juce::jlimit (0, static_cast<int> (oversamplingFactors.size()) - 1, userChoiceIndex);
    }

    inline juce::StringArray modeChoices()
    {
        return { "ST", "MS", "LR" };
    }

    inline juce::StringArray keySourceChoices()
    {
        return { "Internal", "External" };
    }

    inline juce::String modeName (int mode)
    {
        switch (mode)
        {
            case midSide:   return "MS";
            case leftRight: return "LR";
            default:        return "ST";
        }
    }
}
