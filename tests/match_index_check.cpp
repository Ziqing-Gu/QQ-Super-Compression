#include "Source/ExactEnergyIndex.h"
#include <atomic>
#include <vector>
#include <iostream>
#include <random>
#include <cstdlib>
#include <new>
#include <thread>
#define private public
#include "Source/BS1770LoudnessMatch.h"
#undef private

static std::atomic<uint64_t> allocations{0};
void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
static void require(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
using Match=qqsc::BS1770LoudnessMatch;
using Block=Match::BlockEnergies;

static float exactMixed(const std::vector<Block>& blocks,float gain)
{
    double sum=0;size_t count=0;
    for(const auto& b:blocks)if(b.wetL+b.wetR>0){sum+=b.wetL+b.wetR;++count;}
    if(!count)return 0;
    double dry=0,wet=0,total=0;const auto gate=.1*sum/double(count);
    for(const auto& b:blocks)if(b.wetL+b.wetR>gate){dry+=b.dryST;wet+=b.wetST;total+=b.wetL+b.wetR;}
    if(wet<=1e-30)return 0;
    const double cross=.5*(total-dry-wet),target=total*std::pow(10.,double(gain)/10.);
    const auto d=cross*cross+wet*(target-dry);if(d<=0)return -120;
    const auto k=(-cross+std::sqrt(d))/wet;
    return float(std::clamp(20*std::log10(std::max(1e-6,k)),-120.,120.));
}
int main()
{
    try
    {
        Match m;m.prepare(48000);std::mt19937 rng(20260930);
        std::vector<float> dry,wet;std::vector<Block> history;double worst=0;
        // Five hours of 100 ms blocks, including repeated keys, silence,
        // deep quiet material, sharp gate changes and correlated parallel Mix.
        for(int i=0;i<180000;++i)
        {
            float d=i%17==0?0.f:float(std::pow(10.,(-180.+double(rng()%19000)/100.)/10.));
            if(i%7==0)d=.1f;
            const float w=d*(i%2?.01f:.25f);
            Block b;b.dryST=d;b.wetST=w;b.dryL=d;b.wetL=w;b.dryR=d;b.wetR=w;
            b.dryM=d;b.wetM=w;b.dryS=d;b.wetS=w;
            dry.push_back(d);wet.push_back(w);history.push_back(b);m.enqueue(b);
            if(i%64==63)m.servicePending();
            if(i%1000==999)
            {
                m.servicePending();const auto result=m.getLatestMatch();
                const double expected=Match::integratedLoudnessFromEnergies(dry)-Match::integratedLoudnessFromEnergies(wet);
                worst=std::max(worst,std::abs(double(result.st)-expected));
                require(result.validST && worst<1e-5,"indexed MATCH differs from original two-pass gate");
                for(float gain:{-30.f,-1.f,0.f,6.f,30.f})
                    require(std::abs(m.makeupAdjustmentForMixedGain(gain)-exactMixed(history,gain))<1e-4,"mixed correlation/gate changed");
            }
        }
        require(m.getBlockCount()==180000,"historical blocks lost");
        // Strict > gate behavior with equal keys spread over many leaves.
        qqsc::ExactEnergyIndex<> index;
        for(int i=0;i<5000;++i){index.add({.1f});index.add({1.f});}
        require(index.above(double(.1f)).count==5000,"equal boundary keys included");
        require(index.above(1).count==0,"gate comparison is not strict");
        // Audio producer and reset must never allocate, even after long history.
        const auto before=allocations.load();
        m.reset();
        for(int i=0;i<96000;++i){const float x=.1f*float(std::sin(.1*i));m.processSample(x,x,x,x,x,x,x,x,x,x);}
        require(allocations.load()==before,"audio producer/reset allocated");
        m.servicePending();require(m.getLatestMatch().validST,"reset failed to start a fresh measurement");
        require(std::abs(m.getLatestMatch().st)<1e-6,"reset retained old history");
        // Saturation invalidates the result; it never silently drops data and
        // advertises an apparently exact correction.
        Block b;b.dryST=.1f;b.wetST=.01f;
        for(size_t i=0;i<Match::queueCapacity+1;++i)m.enqueue(b);
        require(m.isQueueOverloaded()&&!m.hasAnyResult(),"queue overflow published a partial measurement");
        m.servicePending(Match::queueCapacity);m.reset();m.enqueue(b);m.servicePending();
        require(m.hasAnyResult() && std::abs(m.getLatestMatch().st-10)<1e-5,"overflow/reset recovery failed");
        // SPSC stress with resets while the consumer is running. Read only
        // consumer-owned results on the consumer/main thread.
        Match concurrent;concurrent.prepare(48000);std::atomic<bool> done{false};
        std::thread producer([&]{for(int i=0;i<100000;++i){if(i%4096==0)concurrent.reset();concurrent.enqueue(b);}done=true;});
        while(!done.load())concurrent.servicePending();producer.join();
        concurrent.servicePending(Match::queueCapacity);concurrent.reset();concurrent.enqueue(b);concurrent.servicePending();
        require(concurrent.getBlockCount()==1 && concurrent.hasAnyResult(),"concurrent generation recovery failed");
        std::cout<<"PASS exact MATCH index, five-hour history, mixed correlation, gate ties, zero DSP allocations, queue/reset stress; worst="<<worst<<" LU\n";
        return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
