#pragma once
#include "DynamicsLimits.h"
#include "KWeightingFilter.h"

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

    // Optional monitor analyser: Dry holds the dry Mix contribution, Wet ST
    // the wet contribution, and Wet LR their sum. Account for their correlation
    // when converting the desired overall level change into a Makeup change.
    float makeupAdjustmentForMixedGain(float gainDb) const noexcept
    {
        double sum=0; size_t count=0;
        for(const auto& b:blocks) if(b.wetL+b.wetR>0) {sum+=b.wetL+b.wetR;++count;}
        if(count==0)return 0;
        const double gate=sum/double(count)*.1;
        double dry=0,wet=0,total=0;
        for(const auto& b:blocks) if(b.wetL+b.wetR>gate)
        {dry+=b.dryST;wet+=b.wetST;total+=b.wetL+b.wetR;}
        if(wet<=1.e-30)return 0;
        const double cross=(total-dry-wet)*.5;
        const double target=total*std::pow(10.,double(gainDb)/10.);
        const double discriminant=cross*cross+wet*(target-dry);
        if(discriminant<=0)return -120;
        const double k=(-cross+std::sqrt(discriminant))/wet;
        return float(std::clamp(20.*std::log10(std::max(1.e-6,k)),-120.,120.));
    }

    // Exposed for an isolated reference test without JUCE/VST3.
    static double integratedLoudnessFromEnergies (const std::vector<float>& energies) noexcept
    {
        if (energies.empty())
            return negativeInfinity();

        // MATCH deliberately has no absolute level gate. Any positive energy
        // can be matched; the relative programme gate remains scale invariant.
        constexpr double absoluteGateEnergy = 0.0;

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

        // Relative gate is 10 LU below the nonzero programme mean.
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

        constexpr double absoluteGateEnergy = 0.0; // No fixed low-level cutoff for MATCH.

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

        destination = std::clamp (static_cast<float> (dryLufs - wetLufs), -maximumMakeupDb, maximumMakeupDb);
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
