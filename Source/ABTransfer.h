#pragma once
#include "StaticCompressionEngine.h"
#include <array>

namespace qqsc
{
// Complete gain-domain result of a sound bank. The detector, carrier delay,
// input gain and sidechain configuration are shared only when identical.
struct ABTransfer
{
    std::array<float,5> lower{}, upper{}, ratio{}, upRatio{}, downRatio{}, makeup{}, mix{};
    float output = 1;
    float unityOutput = 1;
    int mode = 0;
    bool dual = false;
    CompressionAlgorithm algorithm = CompressionAlgorithm::classic;
    CompressionAlgorithm upAlgorithm = CompressionAlgorithm::classic, downAlgorithm = CompressionAlgorithm::classic;
    bool operator== (const ABTransfer& b) const noexcept
    {
        return lower==b.lower && upper==b.upper && ratio==b.ratio && upRatio==b.upRatio
            && downRatio==b.downRatio && makeup==b.makeup && mix==b.mix
            && output==b.output && unityOutput==b.unityOutput && mode==b.mode && dual==b.dual && algorithm==b.algorithm
            && upAlgorithm==b.upAlgorithm && downAlgorithm==b.downAlgorithm;
    }
    using Matrix = std::array<float,4>; // L<-L, L<-R, R<-L, R<-R
    static Matrix matrix (const std::array<float,5>& g, int mode) noexcept
    {
        if (mode == 1) return { .5f*(g[3]+g[4]), .5f*(g[3]-g[4]), .5f*(g[3]-g[4]), .5f*(g[3]+g[4]) };
        if (mode == 2) return { g[1],0,0,g[2] };
        return { g[0],0,0,g[0] };
    }
    Matrix dryMatrix() const noexcept
    {
        std::array<float,5> g;
        for (size_t d=0;d<5;++d) g[d]=(1-mix[d])*output;
        return matrix(g,mode);
    }
    Matrix evaluate (const std::array<float,5>& levels, bool externalKey) const noexcept
    {
        std::array<float,5> g;
        for(size_t d=0;d<5;++d)
        {
            const float core = externalKey && levels[d]<=1e-9f ? 1.f : dual
                ? StaticCompressionEngine::dualGainForLevel(levels[d],upRatio[d],downRatio[d],lower[d],upper[d],upAlgorithm,downAlgorithm)
                : StaticCompressionEngine::singleGainForLevel(levels[d],ratio[d],lower[d],upper[d],algorithm);
            g[d]=core*makeup[d]*mix[d]*output;
        }
        return matrix(g,mode);
    }
};
}
