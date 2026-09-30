#pragma once
#include "DynamicsLimits.h"
#include "KWeightingFilter.h"
#include "ExactEnergyIndex.h"
#include <atomic>

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

        // The queue is allocated at construction, never resized by DSP.
        // Historical allocation and gate queries belong to servicePending().

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
        generation.fetch_add(1,std::memory_order_acq_rel);
        queueOverflow.store(false,std::memory_order_release);
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

        enqueue(block);
        samplesUntilBlock = hopSamples;
    }

    // Single consumer: the processor's existing message-thread timer, or an
    // offline test driver after/alongside audio production. Never call from DSP.
    // Bounded draining also keeps a paused/busy GUI from doing an unlimited catch-up.
    void servicePending(size_t budget = 128)
    {
        const auto wanted=generation.load(std::memory_order_acquire);
        if(consumerGeneration!=wanted)
        {
            for(auto& index:indices)index.clear();mixedIndex.clear();
            consumerGeneration=wanted;completedBlocks=0;latestMatch={};
        }
        size_t consumed=0;
        auto read=readPosition.load(std::memory_order_relaxed);
        while(consumed<budget && read!=writePosition.load(std::memory_order_acquire))
        {
            const auto item=queue[read];
            // A concurrent transport reset must not consume the first block of
            // the new generation under the old consumer state.
            if(item.generation>wanted || generation.load(std::memory_order_acquire)!=wanted)break;
            read=(read+1)%queueCapacity;
            readPosition.store(read,std::memory_order_release);
            ++consumed;
            if(item.generation!=wanted)continue;
            const auto& b=item.block;
            const std::array<float,10> energies{b.dryST,b.wetST,b.dryL,b.wetL,b.dryR,b.wetR,b.dryM,b.wetM,b.dryS,b.wetS};
            for(size_t i=0;i<energies.size();++i)indices[i].add({energies[i]});
            mixedIndex.add({b.wetL+b.wetR,b.dryST,b.wetST});
            ++completedBlocks;
        }
        if(consumed>0)recomputeMatch();
    }
    bool hasAnyResult() const noexcept
    {
        const auto& result=getLatestMatch();
        return result.validST || result.validL || result.validR || result.validM || result.validS;
    }
    const MatchDb& getLatestMatch() const noexcept
    {
        return measurementValid() ? latestMatch : emptyMatch;
    }
    size_t getBlockCount() const noexcept { return measurementValid() ? completedBlocks : 0; }
    bool isQueueOverloaded() const noexcept { return queueOverflow.load(std::memory_order_acquire); }

    // Dry, Wet and their sum are indexed together under the exact mixed-energy
    // gate. This preserves correlation without scanning programme history.
    float makeupAdjustmentForMixedGain(float gainDb) const noexcept
    {
        if(!measurementValid())return 0;
        const auto all=mixedIndex.total();if(!all.count)return 0;
        const auto kept=mixedIndex.above(.1*all.values[0]/double(all.count));
        const double total=kept.values[0],dry=kept.values[1],wet=kept.values[2];
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

    bool measurementValid() const noexcept
    {
        return !queueOverflow.load(std::memory_order_acquire)
            && consumerGeneration==generation.load(std::memory_order_acquire);
    }
    void enqueue(const BlockEnergies& block) noexcept
    {
        if(queueOverflow.load(std::memory_order_relaxed))return;
        const auto write=writePosition.load(std::memory_order_relaxed);
        const auto next=(write+1)%queueCapacity;
        if(next==readPosition.load(std::memory_order_acquire))
        {
            // Fail closed: never apply MATCH calculated from a silently truncated
            // measurement. Playback/reset starts a fresh valid measurement.
            queueOverflow.store(true,std::memory_order_release);return;
        }
        queue[write]={block,generation.load(std::memory_order_relaxed)};
        writePosition.store(next,std::memory_order_release);
    }
    template <typename Member>
    double integratedFor(Member member) const noexcept
    {
        const std::array<float BlockEnergies::*,10> members{
            &BlockEnergies::dryST,&BlockEnergies::wetST,&BlockEnergies::dryL,&BlockEnergies::wetL,
            &BlockEnergies::dryR,&BlockEnergies::wetR,&BlockEnergies::dryM,&BlockEnergies::wetM,
            &BlockEnergies::dryS,&BlockEnergies::wetS};
        const auto found=std::find(members.begin(),members.end(),member);
        if(found==members.end())return negativeInfinity();
        const auto& index=indices[size_t(found-members.begin())];
        const auto all=index.total();if(!all.count)return negativeInfinity();
        const auto kept=index.above(.1*all.values[0]/double(all.count));
        return kept.count && kept.values[0]>0
            ? -.691+10.*std::log10(kept.values[0]/double(kept.count)) : negativeInfinity();
    }

    void recomputeMatch() noexcept
    {
        // Message-thread service continues after the host stops its callbacks.
        // Query indexed sums rather than rescanning the entire programme.
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
    struct QueuedBlock { BlockEnergies block; uint64_t generation=0; };
    // Over thirteen minutes of unserviced programme at ten blocks/second.
    static constexpr size_t queueCapacity=8192;
    std::vector<QueuedBlock> queue=std::vector<QueuedBlock>(queueCapacity);
    std::atomic<size_t> writePosition{0},readPosition{0};
    std::atomic<uint64_t> generation{1};
    std::atomic<bool> queueOverflow{false};
    uint64_t consumerGeneration=0;
    size_t completedBlocks=0;
    std::array<ExactEnergyIndex<>,10> indices;
    ExactEnergyIndex<2> mixedIndex;
    const MatchDb emptyMatch{};

    MatchDb latestMatch;
};
}
