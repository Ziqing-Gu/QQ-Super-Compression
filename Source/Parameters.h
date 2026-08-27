#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace qqsc::params
{
    inline constexpr auto inputGainDb    = "inputGainDb";
    inline constexpr auto ratio          = "ratio";
    inline constexpr auto makeupGainDb   = "makeupGainDb";   // ST / legacy shared Makeup
    inline constexpr auto makeupGainLDb  = "makeupGainLDb";
    inline constexpr auto makeupGainRDb  = "makeupGainRDb";
    inline constexpr auto makeupGainMDb  = "makeupGainMDb";
    inline constexpr auto makeupGainSDb  = "makeupGainSDb";
    inline constexpr auto mix            = "mix";
    inline constexpr auto outputGainDb   = "outputGainDb";
    inline constexpr auto lookaheadMs    = "lookaheadMs";
    inline constexpr auto oversampling   = "oversampling";
    inline constexpr auto processingMode = "processingMode";
    inline constexpr auto bypass         = "bypass";

    enum ProcessingMode
    {
        stereoLinked = 0,
        midSide      = 1,
        leftRight    = 2
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
