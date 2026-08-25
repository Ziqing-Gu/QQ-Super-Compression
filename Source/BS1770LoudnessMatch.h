#pragma once

#include <array>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace qqsc
{
class BS1770LoudnessMatch
{
public:
    struct MatchDb
    {
        float st = 0.0f;
        float l  = 0.0f;
        float r  = 0.0f;
        float m  = 0.0f;
        float s  = 0.0f;
        bool validST = false;
        bool validL  = false;
        bool validR  = false;
        bool validM  = false;
        bool validS  = false;
    };

    void prepare (double newSampleRate)
    {
        sampleRate = std::max (1.0, newSampleRate);
        blockSamples = std::max (1, static_cast<int> (std::llround (sampleRate * 0.400)));
        hopSamples   = std::max (1, static_cast<int> (std::llround (sampleRate * 0.100)));

        squareHistory.assign (static_cast<size_t> (blockSamples), {});

        // 400 ms blocks with 100 ms hop produce ten completed blocks/second.
        // Reserve four hours so normal music work never allocates on the audio
        // thread. If a measurement exceeds this, std::vector remains exact and
        // can grow rather than silently truncating the LUFS result.
        blocks.clear();
        blocks.reserve (4u * 60u * 60u * 10u);

        for (auto& filter : filters)
            filter.prepare (sampleRate);

        reset();
    }

    void reset() noexcept
    {
        for (auto& filter : filters)
            filter.reset();

        for (auto& frame : squareHistory)
            frame.fill (0.0f);

        runningSums.fill (0.0);
        historyIndex = 0;
        samplesUntilBlock = blockSamples;
        blocks.clear();
        latestMatch = {};
    }

    // Inputs must be the delayed Dry and the corresponding compressed Wet
    // before Makeup/Mix. ST uses linked Wet L/R, LR uses independent Wet L/R,
    // and MS uses independent Wet M/S. All ten streams are K-weighted in
    // parallel so changing Mode never requires restarting the LUFS measurement.
    void processSample (float dryL, float dryR,
                        float wetSTL, float wetSTR,
                        float wetLRL, float wetLRR,
                        float dryM, float dryS,
                        float wetM, float wetS)
    {
        const std::array<float, streamCount> input
        {
            dryL, dryR,
            wetSTL, wetSTR,
            wetLRL, wetLRR,
            dryM, dryS,
            wetM, wetS
        };

        auto& oldFrame = squareHistory[static_cast<size_t> (historyIndex)];

        for (size_t i = 0; i < streamCount; ++i)
        {
            const auto weighted = filters[i].process (input[i]);
            const auto square = weighted * weighted;
            runningSums[i] += static_cast<double> (square) - static_cast<double> (oldFrame[i]);
            oldFrame[i] = square;
        }

        if (++historyIndex >= blockSamples)
            historyIndex = 0;

        if (--samplesUntilBlock > 0)
            return;

        BlockEnergies block;
        const auto invBlock = 1.0 / static_cast<double> (blockSamples);

        const auto mean = [&] (size_t index) noexcept
        {
            return static_cast<float> (std::max (0.0, runningSums[index] * invBlock));
        };

        // BS.1770 front-channel weights are 1.0. Stereo energy is the weighted
        // sum of the two channel mean squares. L/R/M/S individual results use
        // the exact same mono K-weighted gated-LUFS maths per component.
        block.dryST = mean (dryLIndex) + mean (dryRIndex);
        block.wetST = mean (wetSTLIndex) + mean (wetSTRIndex);
        block.dryL  = mean (dryLIndex);
        block.wetL  = mean (wetLRLIndex);
        block.dryR  = mean (dryRIndex);
        block.wetR  = mean (wetLRRIndex);
        block.dryM  = mean (dryMIndex);
        block.wetM  = mean (wetMIndex);
        block.dryS  = mean (drySIndex);
        block.wetS  = mean (wetSIndex);

        blocks.push_back (block);
        samplesUntilBlock = hopSamples;
        recomputeMatch();
    }

    bool hasAnyResult() const noexcept
    {
        return latestMatch.validST || latestMatch.validL || latestMatch.validR
            || latestMatch.validM || latestMatch.validS;
    }

    const MatchDb& getLatestMatch() const noexcept { return latestMatch; }
    size_t getBlockCount() const noexcept { return blocks.size(); }

    // Exposed for an isolated reference test without JUCE/VST3.
    static double integratedLoudnessFromEnergies (const std::vector<float>& energies) noexcept
    {
        if (energies.empty())
            return negativeInfinity();

        // L = -0.691 + 10 log10(E). Therefore the -70 LUFS absolute gate can
        // be applied directly in the energy domain.
        constexpr double absoluteGateLufs = -70.0;
        const double absoluteGateEnergy = std::pow (10.0, (absoluteGateLufs + 0.691) / 10.0);

        double absoluteSum = 0.0;
        uint64_t absoluteCount = 0;

        for (const auto energyF : energies)
        {
            const auto energy = static_cast<double> (energyF);
            if (energy > absoluteGateEnergy)
            {
                absoluteSum += energy;
                ++absoluteCount;
            }
        }

        if (absoluteCount == 0)
            return negativeInfinity();

        const auto absoluteMean = absoluteSum / static_cast<double> (absoluteCount);

        // The relative gate is 10 LU below the absolute-gated loudness. In the
        // linear energy domain that is exactly one tenth of the absolute-gated
        // mean energy. Both absolute and relative gates must be passed.
        const auto relativeGateEnergy = absoluteMean * 0.1;
        const auto finalGateEnergy = std::max (absoluteGateEnergy, relativeGateEnergy);

        double finalSum = 0.0;
        uint64_t finalCount = 0;

        for (const auto energyF : energies)
        {
            const auto energy = static_cast<double> (energyF);
            if (energy > finalGateEnergy)
            {
                finalSum += energy;
                ++finalCount;
            }
        }

        if (finalCount == 0)
            return negativeInfinity();

        const auto finalMean = finalSum / static_cast<double> (finalCount);
        return -0.691 + 10.0 * std::log10 (finalMean);
    }

private:
    class Biquad
    {
    public:
        void set (double newB0, double newB1, double newB2,
                  double newA1, double newA2) noexcept
        {
            b0 = newB0; b1 = newB1; b2 = newB2;
            a1 = newA1; a2 = newA2;
            reset();
        }

        void reset() noexcept
        {
            x1 = x2 = y1 = y2 = 0.0;
        }

        float process (float input) noexcept
        {
            const auto x0 = static_cast<double> (input);
            const auto y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x0;
            y2 = y1; y1 = y0;
            return static_cast<float> (y0);
        }

    private:
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    };

    class KWeightingFilter
    {
    public:
        void prepare (double sampleRate)
        {
            // De Man / BS.1770-compatible coefficient derivation. At 48 kHz
            // these reproduce the tabulated BS.1770 K-weighting coefficients.
            constexpr double shelfGainDb = 3.99984385397;
            constexpr double shelfQ = 0.7071752369554193;
            constexpr double shelfHz = 1681.9744509555319;
            constexpr double shelfExponent = 0.499666774155;

            const auto kShelf = std::tan (pi * shelfHz / sampleRate);
            const auto vh = std::pow (10.0, shelfGainDb / 20.0);
            const auto vb = std::pow (vh, shelfExponent);
            const auto a0Shelf = 1.0 + kShelf / shelfQ + kShelf * kShelf;

            shelf.set ((vh + vb * kShelf / shelfQ + kShelf * kShelf) / a0Shelf,
                       2.0 * (kShelf * kShelf - vh) / a0Shelf,
                       (vh - vb * kShelf / shelfQ + kShelf * kShelf) / a0Shelf,
                       2.0 * (kShelf * kShelf - 1.0) / a0Shelf,
                       (1.0 - kShelf / shelfQ + kShelf * kShelf) / a0Shelf);

            constexpr double hpQ = 0.5003270373253953;
            constexpr double hpHz = 38.13547087613982;
            const auto kHp = std::tan (pi * hpHz / sampleRate);
            const auto a0Hp = 1.0 + kHp / hpQ + kHp * kHp;

            // This form intentionally keeps the BS.1770 RLB numerator at
            // [1, -2, 1] while the denominator is normalised by a0.
            highPass.set (1.0,
                          -2.0,
                          1.0,
                          2.0 * (kHp * kHp - 1.0) / a0Hp,
                          (1.0 - kHp / hpQ + kHp * kHp) / a0Hp);
        }

        void reset() noexcept
        {
            shelf.reset();
            highPass.reset();
        }

        float process (float input) noexcept
        {
            return highPass.process (shelf.process (input));
        }

    private:
        static constexpr double pi = 3.141592653589793238462643383279502884;
        Biquad shelf;
        Biquad highPass;
    };

    struct BlockEnergies
    {
        float dryST = 0.0f, wetST = 0.0f;
        float dryL = 0.0f, wetL = 0.0f;
        float dryR = 0.0f, wetR = 0.0f;
        float dryM = 0.0f, wetM = 0.0f;
        float dryS = 0.0f, wetS = 0.0f;
    };

    static constexpr size_t dryLIndex   = 0;
    static constexpr size_t dryRIndex   = 1;
    static constexpr size_t wetSTLIndex = 2;
    static constexpr size_t wetSTRIndex = 3;
    static constexpr size_t wetLRLIndex = 4;
    static constexpr size_t wetLRRIndex = 5;
    static constexpr size_t dryMIndex   = 6;
    static constexpr size_t drySIndex   = 7;
    static constexpr size_t wetMIndex   = 8;
    static constexpr size_t wetSIndex   = 9;
    static constexpr size_t streamCount = 10;

    static constexpr double negativeInfinity() noexcept
    {
        return -1.0e300;
    }

    static bool isFiniteLoudness (double value) noexcept
    {
        return value > -1.0e200 && std::isfinite (value);
    }

    template <typename Member>
    double integratedFor (Member member) const noexcept
    {
        if (blocks.empty())
            return negativeInfinity();

        constexpr double absoluteGateLufs = -70.0;
        const double absoluteGateEnergy = std::pow (10.0, (absoluteGateLufs + 0.691) / 10.0);

        double absoluteSum = 0.0;
        uint64_t absoluteCount = 0;
        for (const auto& block : blocks)
        {
            const auto energy = static_cast<double> (block.*member);
            if (energy > absoluteGateEnergy)
            {
                absoluteSum += energy;
                ++absoluteCount;
            }
        }

        if (absoluteCount == 0)
            return negativeInfinity();

        const auto absoluteMean = absoluteSum / static_cast<double> (absoluteCount);
        const auto finalGateEnergy = std::max (absoluteGateEnergy, absoluteMean * 0.1);

        double finalSum = 0.0;
        uint64_t finalCount = 0;
        for (const auto& block : blocks)
        {
            const auto energy = static_cast<double> (block.*member);
            if (energy > finalGateEnergy)
            {
                finalSum += energy;
                ++finalCount;
            }
        }

        if (finalCount == 0)
            return negativeInfinity();

        return -0.691 + 10.0 * std::log10 (finalSum / static_cast<double> (finalCount));
    }

    void recomputeMatch() noexcept
    {
        // Recalculated when each 100 ms gating block completes. This keeps the
        // UI Match button ready even if the host stops audio callbacks exactly
        // at Stop. No filtering/windowing state is changed here.
        const auto dryST = integratedFor (&BlockEnergies::dryST);
        const auto wetST = integratedFor (&BlockEnergies::wetST);
        const auto dryL  = integratedFor (&BlockEnergies::dryL);
        const auto wetL  = integratedFor (&BlockEnergies::wetL);
        const auto dryR  = integratedFor (&BlockEnergies::dryR);
        const auto wetR  = integratedFor (&BlockEnergies::wetR);
        const auto dryM  = integratedFor (&BlockEnergies::dryM);
        const auto wetM  = integratedFor (&BlockEnergies::wetM);
        const auto dryS  = integratedFor (&BlockEnergies::dryS);
        const auto wetS  = integratedFor (&BlockEnergies::wetS);

        latestMatch = {};
        setDifference (dryST, wetST, latestMatch.st, latestMatch.validST);
        setDifference (dryL,  wetL,  latestMatch.l,  latestMatch.validL);
        setDifference (dryR,  wetR,  latestMatch.r,  latestMatch.validR);
        setDifference (dryM,  wetM,  latestMatch.m,  latestMatch.validM);
        setDifference (dryS,  wetS,  latestMatch.s,  latestMatch.validS);
    }

    static void setDifference (double dryLufs, double wetLufs,
                               float& destination, bool& valid) noexcept
    {
        valid = isFiniteLoudness (dryLufs) && isFiniteLoudness (wetLufs);
        if (! valid)
        {
            destination = 0.0f;
            return;
        }

        destination = std::clamp (static_cast<float> (dryLufs - wetLufs), -36.0f, 36.0f);
    }

    double sampleRate = 44100.0;
    int blockSamples = 17640;
    int hopSamples = 4410;
    int historyIndex = 0;
    int samplesUntilBlock = 17640;

    std::array<KWeightingFilter, streamCount> filters;
    std::vector<std::array<float, streamCount>> squareHistory;
    std::array<double, streamCount> runningSums {};
    std::vector<BlockEnergies> blocks;

    MatchDb latestMatch;
};
}
