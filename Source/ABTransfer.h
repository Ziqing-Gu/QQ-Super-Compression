#pragma once
#include "StaticCompressionEngine.h"
#include "Parameters.h"
#include <array>

namespace qqsc
{
// Complete gain-domain result of a sound bank. The detector, carrier delay,
// input gain and sidechain configuration are shared only when identical.
struct ABTransfer
{
    std::array<float,5> lower{}, upper{}, ratio{}, upRatio{}, downRatio{}, makeup{}, mix{};
    float stereoLink = 0;
    float stLRLink = 1, stMSLink = 1;
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
            && stereoLink==b.stereoLink && stLRLink==b.stLRLink && stMSLink==b.stMSLink && output==b.output && unityOutput==b.unityOutput && mode==b.mode && dual==b.dual && algorithm==b.algorithm
            && upAlgorithm==b.upAlgorithm && downAlgorithm==b.downAlgorithm;
    }
    using Matrix = std::array<float,4>; // L<-L, L<-R, R<-L, R<-R
    static Matrix stereoMatrix(const std::array<float,5>& g, float lr, float ms, bool stereo=true) noexcept
    {
        if(!stereo || (lr==1.f && ms==1.f)) return matrix(g,0,stereo);
        const float st=lr*ms, leftRight=.5f*(1-lr)*(1+ms), midSide=.5f*(1-ms)*(1+lr);
        const float common=st*g[0]+midSide*.5f*(g[3]+g[4]);
        const float cross=midSide*.5f*(g[3]-g[4]);
        return {common+leftRight*g[1],cross,cross,common+leftRight*g[2]};
    }
    static Matrix matrix (const std::array<float,5>& g, int mode, bool stereo = true) noexcept
    {
        if (mode == 1 && ! stereo) return { g[3],0,0,0 };
        if (mode == 1) return { .5f*(g[3]+g[4]), .5f*(g[3]-g[4]), .5f*(g[3]-g[4]), .5f*(g[3]+g[4]) };
        if (mode == 2) return { g[1],0,0,g[2] };
        return { g[0],0,0,g[0] };
    }
    Matrix dryMatrix(bool stereo = true) const noexcept
    {
        std::array<float,5> g;
        for (size_t d=0;d<5;++d) g[d]=(1-mix[d])*output;
        return matrix(g,mode,stereo);
    }
    Matrix evaluate (const std::array<float,5>& levels, bool externalKey, bool stereo = true) const noexcept
    {
        std::array<float,5> g {};
        // Each bank's matrix consumes only its own ST, LR or MS domains.
        const bool combined=mode==0 && stereo && (stLRLink<1 || stMSLink<1);
        const size_t first = mode == 1 ? 3u : mode == 2 ? 1u : 0u;
        const size_t end = combined ? 5u : first + (mode == 0 || (mode == 1 && !stereo) ? 1u : 2u);
        for(size_t d=first;d<end;++d)
        {
            const size_t c=combined ? 0u : d;
            const float core = externalKey && levels[d]<=1e-9f ? 1.f : dual
                ? StaticCompressionEngine::dualGainForLevel(levels[d],upRatio[c],downRatio[c],lower[c],upper[c],upAlgorithm,downAlgorithm)
                : StaticCompressionEngine::singleGainForLevel(levels[d],ratio[c],lower[c],upper[c],algorithm);
            g[d]=core;
        }
        if(mode==2 && stereo) params::coupleLimiterGains(g[1],g[2],stereoLink);
        for(size_t d=first;d<end;++d) {const size_t c=combined ? 0u : d;g[d]*=makeup[c]*mix[c]*output;}
        return combined ? stereoMatrix(g,stLRLink,stMSLink,stereo) : matrix(g,mode,stereo);
    }
};
}
