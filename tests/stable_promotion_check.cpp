// Formal identity/state compatibility and audio parity with the accepted Preview.
#define main HistoricalReferenceMain
#include "algorithm_vst_check.cpp"
#undef main
#include <vector>
#include <cstring>

namespace
{
std::vector<float> renderPromotion(juce::AudioPluginInstance& p,int mode,int block,double rate)
{
    p.setRateAndBufferSizeDetails(rate,block);p.prepareToPlay(rate,block);
    require(p.getLatencySamples()==int(std::round(rate*.026)),"26ms latency changed");
    std::vector<float> output;const int count=int(rate*.45);output.reserve(size_t(2*count));
    juce::MidiBuffer midi;
    for(int n=0;n<count;n+=block)
    {
        const int size=std::min(block,count-n);juce::AudioBuffer<float> b(2,size);
        for(int i=0;i<size;++i)
        {
            const double t=(n+i)/rate;
            const double amp=t<.10?.03:t<.20?.9:t<.30?.09:.2+.19*std::sin(t*2*juce::MathConstants<double>::pi*11);
            const float l=float(amp*std::sin(t*2*juce::MathConstants<double>::pi*4341.1));
            const float r=mode==1?-l:float(amp*.8*std::sin(t*2*juce::MathConstants<double>::pi*53));
            b.setSample(0,i,l);b.setSample(1,i,r);
        }
        p.processBlock(b,midi);
        for(int i=0;i<size;++i)for(int c=0;c<2;++c)
        {const auto x=b.getSample(c,i);require(std::isfinite(x),"Nonfinite output");output.push_back(x);}
    }
    p.releaseResources();return output;
}
}
int main(int argc,char** argv)
{
    if(argc!=4)return 2;
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        juce::VST3PluginFormat format;
        std::array<juce::OwnedArray<juce::PluginDescription>,3> desc;
        std::array<std::unique_ptr<juce::AudioPluginInstance>,3> p;
        for(int j=0;j<3;++j)
        {
            format.findAllTypesForFile(desc[j],argv[j+1]);require(desc[j].size()==1,"One plugin per bundle");
            juce::String error;p[j]=format.createInstanceFromDescription(*desc[j][0],48000,256,error);require(p[j]!=nullptr,error);
        }
        require(desc[0][0]->version=="1.2.5"&&desc[1][0]->version=="1.2.6"&&desc[2][0]->version=="1.2.6","Version mismatch");
        require(desc[0][0]->uniqueId==desc[2][0]->uniqueId,"Formal VST3 identity changed");
        require(desc[1][0]->uniqueId!=desc[2][0]->uniqueId,"Formal identity still Preview");
        require(desc[2][0]->name=="QQ Super Compression","Formal product name");
        for(int j:{0,1})
        {
            require(p[j]->getParameters().size()==p[2]->getParameters().size(),"Parameter count changed");
            for(int i=0;i<p[j]->getParameters().size();++i)
            {
                require(p[j]->getHostedParameter(i)->getParameterID()==p[2]->getHostedParameter(i)->getParameterID(),"Parameter ID or order changed");
                require(p[j]->getParameters()[i]->getDefaultValue()==p[2]->getParameters()[i]->getDefaultValue(),"Parameter default changed");
            }
        }
        int cases=0;double worst=0;
        for(int algo:{0,1})for(int dual:{0,1})for(int mode:{0,1,2})
            for(float ratio:{.001f,.125f,1.f,8.f,1000.f})for(float range:{1.f,-6.f})
        {
            const float threshold=cases%3==0?(algo?-120.f:-90.f):cases%3==1?-40.96f:-12.f;
            const int blocks[]={17,256,1024};const double rates[]={44100,48000,96000};
            for(int j:{1,2})configure(*p[j],algo,dual,mode,threshold,ratio,range);
            const auto a=renderPromotion(*p[1],mode,blocks[cases%3],rates[cases%3]);
            const auto b=renderPromotion(*p[2],mode,blocks[cases%3],rates[cases%3]);
            require(a.size()==b.size(),"Audio size mismatch");
            for(size_t i=0;i<a.size();++i)worst=std::max(worst,double(std::abs(a[i]-b[i])));
            ++cases;
        }
        require(worst==0,"Formal audio must be bit-identical to approved Preview");
        // Actual old formal wrapper state, including A/B properties/trailer.
        for(int algo:{0,1})
        {
            configure(*p[0],algo,1,2,-32,.125f,1);
            juce::MemoryBlock state;p[0]->getStateInformation(state);
            p[2]->setStateInformation(state.getData(),int(state.getSize()));
            for(int i=0;i<p[0]->getParameters().size();++i)
                require(std::abs(p[0]->getParameters()[i]->getValue()-p[2]->getParameters()[i]->getValue())<1e-6,"Old formal state parameter recall mismatch");
            configure(*p[1],algo,1,2,-32,.125f,1);
            const auto a=renderPromotion(*p[1],2,256,48000),b=renderPromotion(*p[2],2,256,48000);
            require(a==b,"Old project recall does not use the accepted detector");
        }
        std::cout<<"PASS: formal Qscp identity/name restored, all parameter IDs/order/defaults preserved; "<<cases<<" actual Preview/Stable VST3 audio cases bit-identical (max error "<<worst<<"); Classic/Super,Single/Dual,ST/LR/MS,Up/Down,Range OFF/finite,44.1/48/96k,17/256/1024 blocks; actual1.2.5 wrapper state recall in both algorithms; 26ms latency unchanged.\n";
        for(auto& plugin:p)plugin.reset();return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
