#pragma once
#include "KWeightingFilter.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace qqsc
{
// BS.1770 programme loudness: 400 ms K-weighted blocks, 100 ms hops,
// -70 LUFS absolute gate and -10 LU relative gate. Unlike MATCH, this meter
// intentionally retains the standard absolute gate.
class IntegratedLoudnessMeter
{
public:
    void prepare(double rate)
    {
        sampleRate = std::max(8000.0, rate);
        blockSamples = std::max(1, int(std::llround(sampleRate * .4)));
        hopSamples = std::max(1, int(std::llround(sampleRate * .1)));
        history.assign(size_t(blockSamples), 0.0);
        histogram.assign(binCount + 1, {});
        for (auto& f : filters) f.prepare(sampleRate);
        reset();
    }

    void reset() noexcept
    {
        for (auto& f : filters) f.reset();
        std::fill(history.begin(), history.end(), 0.0);
        std::fill(histogram.begin(), histogram.end(), Energy{});
        cursor = 0;
        untilBlock = blockSamples;
        sum = 0;
        total = {};
        samples = 0;
        value = noReading;
    }

    void processSample(float left, float right) noexcept
    {
        const double l = filters[0].process(std::isfinite(left) ? left : 0.0f);
        const double r = filters[1].process(std::isfinite(right) ? right : 0.0f);
        const double energy = l*l + r*r;
        sum += energy - history[size_t(cursor)];
        history[size_t(cursor)] = energy;
        cursor = (cursor + 1) % blockSamples;
        ++samples;
        if (--untilBlock != 0) return;
        untilBlock = hopSamples;
        addBlockEnergy(std::max(0.0, sum / double(blockSamples)));
    }

    float integratedLufs() const noexcept { return value; }
    float seconds() const noexcept { return float(double(samples) / sampleRate); }

    // Also used by the exact two-pass gate regression. Energies retain their
    // full precision; only membership of the relative-gate boundary bin uses
    // its mean (0.01 LU bins). A Fenwick tree bounds work and memory regardless
    // of session length. No allocation, mutex or growing programme scan in DSP.
    void addBlockEnergy(double energy) noexcept
    {
        constexpr double absoluteGate = 1.1724653045822981e-7; // 10^((-70+.691)/10)
        if (!std::isfinite(energy) || energy <= absoluteGate) return;
        const double lufs = -.691 + 10.0 * std::log10(energy);
        const size_t bin = size_t(std::clamp(int(std::floor((lufs + 70.0) * binsPerLu)), 0, int(binCount)-1));
        for (size_t i = bin + 1; i <= binCount; i += i & (~i + 1))
        {
            histogram[i].sum += energy;
            ++histogram[i].count;
        }
        total.sum += energy;
        ++total.count;
        const double gate = std::max(absoluteGate, .1 * total.sum / double(total.count));
        const double gateLufs = -.691 + 10.0 * std::log10(gate);
        const size_t gateBin = size_t(std::clamp(int(std::floor((gateLufs + 70.0) * binsPerLu)), 0, int(binCount)-1));
        auto excluded = prefix(gateBin);
        const auto through = prefix(gateBin + 1);
        const auto boundaryCount = through.count - excluded.count;
        if (boundaryCount > 0 && (through.sum - excluded.sum) / double(boundaryCount) <= gate)
            excluded = through;
        const auto count = total.count - excluded.count;
        const double kept = total.sum - excluded.sum;
        value = count > 0 && kept > 0 ? float(-.691 + 10.0*std::log10(kept/double(count))) : noReading;
    }

    static constexpr float noReading = -120.0f;

private:
    struct Energy { double sum = 0; uint64_t count = 0; };
    Energy prefix(size_t end) const noexcept
    {
        Energy result;
        for (size_t i = end; i > 0; i -= i & (~i + 1))
        { result.sum += histogram[i].sum; result.count += histogram[i].count; }
        return result;
    }
    static constexpr double binsPerLu = 100;
    static constexpr size_t binCount = 20001; // -70 to +130 LUFS; tail stays in final bin.
    std::array<KWeightingFilter, 2> filters;
    std::vector<double> history;
    std::vector<Energy> histogram;
    Energy total;
    double sampleRate = 48000, sum = 0;
    int blockSamples = 19200, hopSamples = 4800, cursor = 0, untilBlock = 19200;
    uint64_t samples = 0;
    float value = noReading;
};
}
