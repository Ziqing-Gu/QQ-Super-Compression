#pragma once

#include <JuceHeader.h>
#include "DynamicsLimits.h"
#include <array>
#include <cmath>
#include <limits>

namespace qqsc::params
{
    inline constexpr auto limiterMode = "limiterMode";
    inline constexpr auto limiterStereoLink = "limiterStereoLink";
    // Gain-domain coupling only; never mix channel audio. A stronger cut can
    // attenuate the other channel, but a boost never drags its partner upward.
    inline void coupleLimiterGains(float& left, float& right, float amount) noexcept
    {
        const float shared = juce::jmin(1.0f, juce::jmin(left, right));
        if (shared >= 1.0f || amount <= 0.0f) return;
        const float a = juce::jlimit(0.0f, 1.0f, amount);
        left += a * (shared - left);
        right += a * (shared - right);
    }
    inline constexpr auto limiterLink = "limiterLink";
    inline constexpr auto ceilingDb = "ceilingDb";
    inline constexpr auto truePeakLimiting = "truePeakLimiting";
    inline constexpr auto tpRecoveryMode = "tpRecoveryMode";
    inline constexpr auto ceilingOversampling = "ceilingOversampling"; // 1.2.36: Limiter Ceiling 1x/4x/8x/16x
    enum TpRecoveryMode { tpTight = 0, tpAuto = 1, tpSmooth = 2 };
    inline constexpr auto limiterCalibrationDb = "limiterCalibrationDb";
    inline constexpr auto limiterOutputDb = "limiterOutputDb";
    inline float limiterRatio (float ratio, bool limiter, bool dualDown = false) noexcept
    {
        if (limiter && (dualDown || ratio > 1.0f))
            return juce::jlimit (limiterMinimumDownRatio, maximumDownRatio, ratio);
        return juce::jlimit (dualDown ? 1.0f : limiter ? limiterMinimumUpRatio : minimumUpRatio,
                            limiter ? 1.0f : normalMaximumDownRatio, ratio);
    }
    inline float upwardRatio (float ratio, bool limiter) noexcept
    {
        return juce::jlimit (limiter ? limiterDualMinimumUpRatio : minimumUpRatio, 1.0f, ratio);
    }
    template <typename T = double>
    inline juce::NormalisableRange<T> limiterSingleRange()
    {
        // Limiter SINGLE is still the normal QQ compression curve: the lower
        // half is Upward 1:8..1:1, the upper half is Downward 1:1..1000:1.
        // Unity is the exact midpoint. The final Ceiling/TP stage performs the
        // actual peak limiting, so no artificial 1:1..200:1 gap is needed.
        return { T(limiterMinimumUpRatio), T(maximumDownRatio),
            [] (T lo, T hi, T n) {
                return n <= T(0.5) ? lo * std::pow (T(1)/lo, T(2)*n)
                                   : std::pow (hi, T(2)*n-T(1));
            },
            [] (T lo, T hi, T v) {
                v=juce::jlimit(lo,hi,v);
                return v <= T(1) ? T(0.5)*std::log(juce::jmax(lo,v)/lo)/std::log(T(1)/lo)
                                 : T(0.5)+T(0.5)*std::log(v)/std::log(hi);
            },
            [] (T lo, T hi, T v) { return juce::jlimit(lo,hi,v); } };
    }
    inline constexpr auto algorithmMode = "algorithmMode"; // Appended host parameter in v1.2.4
    enum AlgorithmMode { classicAlgorithm = 0, superAlgorithm = 1 };

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
    inline constexpr auto inputOutputLink = "inputOutputLink";
    inline constexpr auto lookaheadMs    = "lookaheadMs";
    inline constexpr auto detectorMode   = "detectorMode"; // Original or bilateral peak window
    inline constexpr auto distort = "distort"; // 0% full window, 100% sample detection
    inline constexpr auto detectorWindowMs = "detectorWindowMs"; // Radius, independent of audio/PDC delay
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

    inline constexpr std::array<const char*,35> normalSoundIds { "ratio", "ratioL", "ratioR", "ratioM", "ratioS", "upRatio", "upRatioL", "upRatioR", "upRatioM", "upRatioS", "downRatio", "downRatioL", "downRatioR", "downRatioM", "downRatioS", "thresholdDb", "thresholdLDb", "thresholdRDb", "thresholdMDb", "thresholdSDb", "rangeDb", "rangeLDb", "rangeRDb", "rangeMDb", "rangeSDb", "upThresholdDb", "upThresholdLDb", "upThresholdRDb", "upThresholdMDb", "upThresholdSDb", "downThresholdDb", "downThresholdLDb", "downThresholdRDb", "downThresholdMDb", "downThresholdSDb" };
    inline constexpr std::array<const char*,35> limiterSoundIds { "limiterRatio", "limiterRatioL", "limiterRatioR", "limiterRatioM", "limiterRatioS", "limiterUpRatio", "limiterUpRatioL", "limiterUpRatioR", "limiterUpRatioM", "limiterUpRatioS", "limiterDownRatio", "limiterDownRatioL", "limiterDownRatioR", "limiterDownRatioM", "limiterDownRatioS", "limiterThresholdDb", "limiterThresholdLDb", "limiterThresholdRDb", "limiterThresholdMDb", "limiterThresholdSDb", "limiterRangeDb", "limiterRangeLDb", "limiterRangeRDb", "limiterRangeMDb", "limiterRangeSDb", "limiterUpThresholdDb", "limiterUpThresholdLDb", "limiterUpThresholdRDb", "limiterUpThresholdMDb", "limiterUpThresholdSDb", "limiterDownThresholdDb", "limiterDownThresholdLDb", "limiterDownThresholdRDb", "limiterDownThresholdMDb", "limiterDownThresholdSDb" };
    inline constexpr std::array<const char*,5> limiterRatioIds { "limiterRatio", "limiterRatioL", "limiterRatioR", "limiterRatioM", "limiterRatioS" };
    inline constexpr std::array<const char*,5> limiterUpRatioIds { "limiterUpRatio", "limiterUpRatioL", "limiterUpRatioR", "limiterUpRatioM", "limiterUpRatioS" };
    inline constexpr std::array<const char*,5> limiterDownRatioIds { "limiterDownRatio", "limiterDownRatioL", "limiterDownRatioR", "limiterDownRatioM", "limiterDownRatioS" };
    inline constexpr std::array<const char*,5> limiterThresholdIds { "limiterThresholdDb", "limiterThresholdLDb", "limiterThresholdRDb", "limiterThresholdMDb", "limiterThresholdSDb" };
    inline constexpr std::array<const char*,5> limiterRangeIds { "limiterRangeDb", "limiterRangeLDb", "limiterRangeRDb", "limiterRangeMDb", "limiterRangeSDb" };
    inline constexpr std::array<const char*,5> limiterUpThresholdIds { "limiterUpThresholdDb", "limiterUpThresholdLDb", "limiterUpThresholdRDb", "limiterUpThresholdMDb", "limiterUpThresholdSDb" };
    inline constexpr std::array<const char*,5> limiterDownThresholdIds { "limiterDownThresholdDb", "limiterDownThresholdLDb", "limiterDownThresholdRDb", "limiterDownThresholdMDb", "limiterDownThresholdSDb" };
    // These were shared before the independent-mode revision. Appended IDs
    // preserve every existing host parameter ID/index. Each mode owns its timing.
    inline constexpr std::array<const char*,33> normalModeIds { "inputGainDb", "makeupGainDb", "makeupGainLDb", "makeupGainRDb", "makeupGainMDb", "makeupGainSDb", "mix", "mixL", "mixR", "mixM", "mixS", "algorithmMode", "compressionMode", "processingMode", "domainLink", "dualRatioLink", "upEnabled", "upEnabledL", "upEnabledR", "upEnabledM", "upEnabledS", "downEnabled", "downEnabledL", "downEnabledR", "downEnabledM", "downEnabledS", "keySource", "keyGainDb", "keyHpfHz", "oversampling", "lookaheadMs", "upAlgorithmMode", "downAlgorithmMode" };
    inline constexpr std::array<const char*,33> limiterModeIds { "limiterInputGainDb", "limiterMakeupGainDb", "limiterMakeupGainLDb", "limiterMakeupGainRDb", "limiterMakeupGainMDb", "limiterMakeupGainSDb", "limiterMix", "limiterMixL", "limiterMixR", "limiterMixM", "limiterMixS", "limiterAlgorithmMode", "limiterCompressionMode", "limiterProcessingMode", "limiterDomainLink", "limiterDualRatioLink", "limiterUpEnabled", "limiterUpEnabledL", "limiterUpEnabledR", "limiterUpEnabledM", "limiterUpEnabledS", "limiterDownEnabled", "limiterDownEnabledL", "limiterDownEnabledR", "limiterDownEnabledM", "limiterDownEnabledS", "limiterKeySource", "limiterKeyGainDb", "limiterKeyHpfHz", "limiterOversampling", "limiterLookaheadMs", "limiterUpAlgorithmMode", "limiterDownAlgorithmMode" };
    inline const std::array<const char*,5>& boundaryBankIds (int bank, bool upper) noexcept
    {
        if (bank==0) return upper ? rangeIds : thresholdIds;
        if (bank==1) return upper ? downThresholdIds : upThresholdIds;
        if (bank==2) return upper ? limiterRangeIds : limiterThresholdIds;
        return upper ? limiterDownThresholdIds : limiterUpThresholdIds;
    }

    inline juce::NormalisableRange<float> dynamicsRatioRange (float minimum = qqsc::minimumUpRatio, float maximum = qqsc::normalMaximumDownRatio)
    {
        // Keep Single's unity position at the exact midpoint with finite,
        // logarithmic upward and downward ranges on their respective halves.
        if (minimum < 1.0f && maximum > 1.0f)
            return { minimum, maximum,
                     [] (float lo, float hi, float n)
                     {
                         return n < 0.5f ? lo * std::pow (1.0f / lo, n * 2.0f)
                                         : std::pow (hi, (n - 0.5f) * 2.0f);
                     },
                     [] (float lo, float hi, float value)
                     {
                         value = juce::jlimit (lo, hi, value);
                         return value < 1.0f ? 0.5f * std::log (value / lo) / std::log (1.0f / lo)
                                            : 0.5f + 0.5f * std::log (value) / std::log (hi);
                     },
                     [] (float lo, float hi, float value) { return juce::jlimit (lo, hi, value); } };
        return { minimum, maximum,
                 [] (float lo, float hi, float normalised) { return lo * std::pow (hi / lo, normalised); },
                 [] (float lo, float hi, float value) { return std::log (juce::jlimit (lo, hi, value) / lo) / std::log (hi / lo); },
                 [] (float lo, float hi, float value) { return juce::jlimit (lo, hi, value); } };
    }

    inline juce::String dynamicsRatioText (float value)
    {
        if (value < 0.99999f)
        {
            const auto denominator = 1.0f / value;
            return "1:" + juce::String (denominator, denominator >= 999.5f ? 0 : denominator >= 100.0f ? 1 : 2);
        }
        return juce::String (value, value >= 999.5f ? 0 : value < 10.0f ? 2 : 1) + ":1";
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

    // Preserve the host numeric endpoint. Super interprets it as -inf;
    // Classic clamps stored values below -90 dB inside its transfer functions.
    // Keeping the per-algorithm mapping in the engine also preserves both
    // sides of the Classic/Super crossfade at this shared stored value.
    inline constexpr float thresholdOffDb = -120.0f;
    inline constexpr float limiterThresholdMinimumDb = -45.0f;
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

        return juce::Decibels::decibelsToGain (juce::jlimit (thresholdOffDb, 0.0f, db), thresholdOffDb);
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

    inline juce::String boundaryText (float db, bool classic, int decimals = 2)
    {
        if (classic) db = juce::jmax (classicThresholdMinimumDb, db);
        return classic || isThresholdEnabled (db)
            ? juce::String (db, decimals) + " dB" : juce::String ("-inf dB");
    }

    inline juce::String rangeText (float db, bool classic = false)
    {
        if (! isRangeEnabled (db)) return "OFF";
        if (! classic && ! isThresholdEnabled (db)) return "-inf dB";
        if (classic) db = juce::jmax (classicThresholdMinimumDb, db);
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


    inline constexpr std::array<float, 5> lookaheadPresetMs { 0.0f, 26.0f, 40.0f, 80.0f, 100.0f };
    inline constexpr std::array<int, 4> oversamplingFactors { 1, 4, 8, 16 };
    inline constexpr std::array<int, 4> oversamplingStageCounts { 0, 2, 3, 4 };

    // Independent Core and Ceiling controls share the ordered 1x/4x/8x/16x choices.
    enum OversamplingChoice { osNative = 0, os4x = 1, os8x = 2, os16x = 3 };
    enum CeilingOversamplingChoice { ceilingNative = 0, ceiling4x = 1, ceiling8x = 2, ceiling16x = 3 };
    inline juce::StringArray ceilingOversamplingChoices() { return { "1x", "4x", "8x", "16x" }; }
    inline int ceilingOversamplingFactorForChoiceIndex(int index) noexcept
    {
        constexpr std::array<int,4> factors {1,4,8,16};
        return factors[size_t(juce::jlimit(0,3,index))];
    }
    inline int migrateLegacyOversamplingChoice(int index) noexcept
    {
        index=juce::jlimit(0,2,index);
        return index==0 ? ceilingNative : index+1;
    }

    inline juce::StringArray oversamplingChoices()
    {
        return { "1x", "4x", "8x", "16x" };
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
        return { "0 ms", "26 ms", "40 ms", "80 ms", "100 ms" };
    }

    inline int lookaheadChoiceIndexForMs (float ms) noexcept
    {
        // The retired 10 ms preset (including its legacy 5 ms midpoint) now
        // selects 26 ms. Do not migrate that nonzero mode to zero latency.
        if (ms >= 5.0f && ms < 26.0f)
            return 1;
        int bestIndex = 0;
        auto bestDistance = std::abs (ms - lookaheadPresetMs[0]);

        for (int i = 1; i < static_cast<int> (lookaheadPresetMs.size()); ++i)
        {
            const auto distance = std::abs (ms - lookaheadPresetMs[static_cast<size_t> (i)]);
            // On an exact tie prefer the longer preset.
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
        juce::ignoreUnused (requestedLookaheadMs); // One audio OS factor at every Lookahead.
        return juce::jlimit (0, static_cast<int> (oversamplingFactors.size()) - 1, userChoiceIndex);
    }

    inline float windowMsForDistort (float lookahead, float percent) noexcept
    { return snapLookaheadMs(lookahead) * (1.0f - juce::jlimit(0.0f,100.0f,percent) * 0.01f); }
    inline float distortForWindowMs (float lookahead, float window) noexcept
    {
        lookahead = snapLookaheadMs(lookahead);
        return lookahead > 0.0f ? juce::jlimit(0.0f,100.0f,100.0f*(1.0f-window/lookahead)) : 0.0f;
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
