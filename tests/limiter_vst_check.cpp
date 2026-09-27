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
    require(p.getLatencySamples()>=int(std::round(rate*.026)) && p.getLatencySamples()<int(rate*.040),"Reported latency outside 26-40ms budget");
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
double alignedError(const std::vector<float>& a,const std::vector<float>& b,int delay)
{
    require(delay>=0,"Guard reported negative delay");
    double error=0;
    for(size_t i=0;i+size_t(2*delay)<b.size();++i)
        error=std::max(error,double(std::abs(a[i]-b[i+size_t(2*delay)])));
    for(int i=0;i<2*delay;++i)require(b[size_t(i)]==0,"Guard output before reported latency");
    return error;
}

void limiterState(juce::AudioPluginInstance& p,int algo,int mode,bool tp,bool monitor,float ratio=200,float output=18)
{
    configure(p,algo,0,mode,-18,20,1);
    juce::MemoryBlock state;p.getStateInformation(state);
    auto wrapper=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
    auto* component=wrapper->getChildByName("IComponent");juce::MemoryBlock data;
    require(data.fromBase64Encoding(component->getAllSubText()),"Component decode");
    const size_t xmlBytes=9+juce::ByteOrder::littleEndianInt(static_cast<const char*>(data.getData())+4);
    juce::MemoryBlock trailer(static_cast<const char*>(data.getData())+xmlBytes,data.getSize()-xmlBytes);
    auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));
    auto tree=juce::ValueTree::fromXml(*xml);
    const auto put=[&](const juce::String& id,float value)
    {auto c=tree.getChildWithProperty("id",id);require(c.isValid(),id);c.setProperty("value",value,nullptr);};
    for(auto suffix:{"","L","R","M","S"})
    {
        const juce::String s(suffix);put("limiterRatio"+s,ratio);
        put("limiterDownRatio"+s,200);
        put("limiterThreshold"+s+"Db",-18);put("limiterRange"+s+"Db",1);
    }
    put("limiterMode",1);put("limiterOutputDb",output);put("limiterCalibrationDb",0);
    put("ceilingDb",-1);put("truePeakLimiting",tp?1.f:0.f);
    tree.setProperty("qqscLimiterUnityMonitor",monitor,nullptr);
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),data);data.append(trailer.getData(),trailer.getSize());
    component->deleteAllChildElements();component->addTextElement(data.toBase64Encoding());
    juce::AudioProcessor::copyXmlToBinary(*wrapper,state);p.setStateInformation(state.getData(),int(state.getSize()));
}
}
#ifndef QQSC_SKIP_LIMITER_VST_MAIN
int main(int argc,char** argv)
{
    if(argc!=5)return 2;
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        juce::VST3PluginFormat format;
        std::array<juce::OwnedArray<juce::PluginDescription>,4> desc;
        std::array<std::unique_ptr<juce::AudioPluginInstance>,4> p;
        for(int j=0;j<4;++j)
        {
            format.findAllTypesForFile(desc[j],argv[j+1]);require(desc[j].size()==1,"One plugin per bundle");
            juce::String error;p[j]=format.createInstanceFromDescription(*desc[j][0],48000,256,error);require(p[j]!=nullptr,error);
        }
        require(desc[0][0]->version=="1.2.6"&&desc[1][0]->version=="1.2.7"&&desc[2][0]->version=="1.2.7","Version mismatch");
        require(desc[0][0]->uniqueId==desc[2][0]->uniqueId,"Formal VST3 identity changed");
        require(desc[1][0]->uniqueId!=desc[2][0]->uniqueId,"Formal identity still Preview");
        require(desc[2][0]->name=="QQ Super Compression","Formal product name");
        juce::AudioProcessorParameter* tpParameter=nullptr;
        for(auto* parameter:p[2]->getParameters())
            if(parameter->getName(64)=="True Peak Limiting")tpParameter=parameter;
        require(tpParameter && tpParameter->getDefaultValue()==1,"TP parameter missing or wrong default");
        for(int j:{0,1})
        {
            require(j==0 ? p[j]->getParameters().size()<p[2]->getParameters().size() : p[j]->getParameters().size()+1==p[2]->getParameters().size(),"Expected one appended TP parameter");
            for(int i=0;i<p[j]->getParameters().size();++i)
            {
                require(p[j]->getHostedParameter(i)->getParameterID()==p[2]->getHostedParameter(i)->getParameterID(),"Parameter ID or order changed");
                if (p[j]->getParameters()[i]->getName(64)=="Theoretical Ceiling")
                    require(p[2]->getParameters()[i]->getDefaultValue()==1.0f,"Ceiling default must now be 0 dBFS");
                else
                    require(p[j]->getParameters()[i]->getDefaultValue()==p[2]->getParameters()[i]->getDefaultValue(),"Unrelated parameter default changed");
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
            worst=std::max(worst,alignedError(a,b,p[2]->getLatencySamples()-p[1]->getLatencySamples()));
            ++cases;
        }
        require(worst==0,"Limiter OFF must be bit-identical after compensating the reported guard delay");
        // Actual old formal wrapper state, including A/B properties/trailer.
        for(int algo:{0,1})
        {
            configure(*p[0],algo,1,2,-32,.125f,1);
            juce::MemoryBlock state;p[0]->getStateInformation(state);
            tpParameter->setValueNotifyingHost(1);
            p[2]->setStateInformation(state.getData(),int(state.getSize()));
            require(tpParameter->getValue()==0,"Old project without TP must restore sample-peak mode");
            for(int i=0;i<p[0]->getParameters().size();++i)
                require(std::abs(p[0]->getParameters()[i]->getValue()-p[2]->getParameters()[i]->getValue())<1e-6,"Old formal state parameter recall mismatch");
            configure(*p[1],algo,1,2,-32,.125f,1);
            const auto a=renderPromotion(*p[1],2,256,48000),b=renderPromotion(*p[2],2,256,48000);
            require(alignedError(a,b,p[2]->getLatencySamples()-p[1]->getLatencySamples())==0,"Old project recall does not use the accepted detector");
        }
        std::cout<<"PASS: original Qscp identity/name; historical parameter IDs/order preserved; TP appended and Ceiling default 0; "<<cases<<" actual VST3 Limiter OFF cases bit-identical after reported delay compensation (max error "<<worst<<"); Classic/Super,Single/Dual,ST/LR/MS,Up/Down,Range OFF/finite,44.1/48/96k,17/256/1024 blocks; actual 1.2.6 wrapper state recall in both algorithms.\n";
        double monitorError=0;int monitorCases=0;
        for(int algo:{0,1})for(int mode:{0,1,2})for(bool tp:{false,true})
        {
            limiterState(*p[3],algo,mode,tp,false);limiterState(*p[2],algo,mode,tp,false);
            const auto old=renderPromotion(*p[3],mode,127,48000),normal=renderPromotion(*p[2],mode,127,48000);
            require(old==normal,"Accepted Limiter waveform changed while headphones OFF");
            limiterState(*p[2],algo,mode,tp,true);const auto monitored=renderPromotion(*p[2],mode,127,48000);
            const double scalar=std::pow(10.,-18./20.);
            for(size_t i=0;i<normal.size();++i)monitorError=std::max(monitorError,std::abs(normal[i]*scalar-monitored[i]));
            ++monitorCases;
        }
        require(monitorError<1.e-6,"Real VST3 headphone compensation changed waveform");
        std::cout<<"PASS: "<<monitorCases<<" actual VST3 Limiter ON cases bit-identical to Adaptive Ceiling when monitor OFF; TP/Peak,Classic/Super,ST/MS/LR. Monitor scaled null max="<<monitorError<<".\n";
        for(auto& plugin:p)plugin.reset();return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
#endif
