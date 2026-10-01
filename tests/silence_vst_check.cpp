// Diagnostic host: load the installed VST3 and exercise its actual wrapper.
#define main HistoricalVstReferenceMain
#include "algorithm_vst_check.cpp"
#undef main
#include <chrono>
#include <functional>
#include <iomanip>

struct SilenceHost final : juce::AudioPlayHead
{
    bool playing=false, available=true; int64_t sample=0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        if(!available) return {};
        PositionInfo p; p.setIsPlaying(playing); p.setTimeInSamples(sample);
        p.setTimeInSeconds(double(sample)/48000.0);p.setBpm(120.0); return p;
    }
};
static void editState(juce::AudioPluginInstance& p,const std::function<void(juce::ValueTree&)>& edit)
{
    juce::MemoryBlock state;p.getStateInformation(state);
    auto wrapper=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
    require(wrapper && wrapper->hasTagName("VST3PluginState"),"VST3 state wrapper");
    auto* component=wrapper->getChildByName("IComponent");require(component!=nullptr,"VST3 component");
    juce::MemoryBlock data;require(data.fromBase64Encoding(component->getAllSubText()),"State decode");
    const size_t xmlBytes=9+juce::ByteOrder::littleEndianInt(static_cast<const char*>(data.getData())+4);
    require(xmlBytes<=data.getSize(),"State trailer bound");
    juce::MemoryBlock trailer(static_cast<const char*>(data.getData())+xmlBytes,data.getSize()-xmlBytes);
    auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));require(xml!=nullptr,"Processor state");
    auto tree=juce::ValueTree::fromXml(*xml);edit(tree);
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),data);data.append(trailer.getData(),trailer.getSize());
    component->deleteAllChildElements();component->addTextElement(data.toBase64Encoding());
    juce::AudioProcessor::copyXmlToBinary(*wrapper,state);p.setStateInformation(state.getData(),int(state.getSize()));
}

#include "revision1238_vst_performance.inc"
#include "revision1241_continuous_vst_checks.inc"
int main(int argc,char** argv)
{
    const bool residualCheck=argc==3 && juce::String(argv[1])=="--residual-1242";
    if(argc!=2 && argc!=4 && !residualCheck)return 2;
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        std::cout<<std::unitbuf<<std::setprecision(8);
        if(argc==4 && juce::String(argv[1])=="--continuous-1241")return compareContinuousVst1241(argv[2],argv[3]);
        if(argc==4 && juce::String(argv[1])=="--eco-compare")return compareEcoVst(argv[2],argv[3]);
        juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> desc;
        format.findAllTypesForFile(desc,juce::String::fromUTF8(argv[residualCheck?2:1]));require(desc.size()==1,"One VST3");
        std::cout<<"VST3 version="<<desc[0]->version<<" name="<<desc[0]->name<<"\n";
        struct Config{const char* name;bool limiter;float look;int core,ceiling;};
        const Config configs[]={
            {"Normal26",false,26,0,3},{"Normal0-4",false,0,1,3},{"Normal0-8",false,0,2,3},{"Normal0-16",false,0,3,3},
            {"Limiter26-4",true,26,0,1},{"Limiter26-8",true,26,0,2},{"Limiter26-16",true,26,0,3},
            {"Limiter0-16core4ceil",true,0,3,1},{"Limiter0-16core8ceil",true,0,3,2},{"Limiter0-16shared",true,0,3,3}};
        const int blockSize=residualCheck?1024:256;
        const int scale=blockSize/256;
        std::cout<<"block="<<blockSize<<"; config,host,source,latency,early_us,late_us,late_output_peak\n";
        for(const auto& c:configs)for(int hostCase=0;hostCase<3;++hostCase)
        {
            juce::String error;auto p=format.createInstanceFromDescription(*desc[0],48000,blockSize,error);require(p!=nullptr,error);
            editState(*p,[&](auto& tree)
            {
                const auto put=[&](const juce::String& id,float value){auto v=tree.getChildWithProperty("id",id);require(v.isValid(),id);v.setProperty("value",value,nullptr);};
                for(const juce::String prefix:{juce::String(),juce::String("limiter")})
                {
                    const auto id=[&](const char* s){juce::String n(s);return prefix.isEmpty()?n:prefix+n.substring(0,1).toUpperCase()+n.substring(1);};
                    put(id("lookaheadMs"),c.look);put(id("oversampling"),float(c.core));
                    put(id("keySource"),0);put(id("keyGainDb"),0);put(id("keyHpfHz"),0);
                    put(id("processingMode"),0);put(id("algorithmMode"),1);put(id("compressionMode"),0);
                    put(id("inputGainDb"),0);put(id("makeupGainDb"),c.limiter?1.36f:0.f);
                    put(id("mix"),100);put(id("ratio"),c.limiter?9.81f:18.1f);
                    put(id("thresholdDb"),c.limiter?-16.27f:-120.f);put(id("rangeDb"),1);
                }
                put("limiterMode",c.limiter?1.f:0.f);put("ceilingOversampling",float(c.ceiling));
                put("truePeakLimiting",1);put("tpRecoveryMode",2);put("ceilingDb",-.3f);
                put("limiterOutputDb",14.91f);put("outputGainDb",0);put("bypass",0);
                tree.setProperty("qqscPerformanceEco",false,nullptr);
                tree.setProperty("qqscLimiterUnityMonitor",false,nullptr);
                tree.setProperty("qqscLimiterBankInitialised",true,nullptr);
            });
            SilenceHost host;host.playing=true;p->setPlayHead(&host);
            p->setRateAndBufferSizeDetails(48000,blockSize);p->prepareToPlay(48000,blockSize);
            juce::AudioBuffer<float> b(juce::jmax(2,p->getTotalNumInputChannels()),blockSize);juce::MidiBuffer midi;
            for(int n=0;n<60/scale;++n)
            {
                b.clear();for(int i=0;i<blockSize;++i)for(int ch=0;ch<2;++ch)b.setSample(ch,i,float(.8*std::sin(2*juce::MathConstants<double>::pi*(ch?73:997)*(host.sample+i)/48000.)));
                p->processBlock(b,midi);host.sample+=blockSize;
            }
            host.playing=hostCase==2; const int latency=p->getLatencySamples();
            double early=0,late=0,peak=0;
            for(int n=0;n<1408/scale;++n)
            {
                b.clear();
                if(residualCheck)for(int ch=0;ch<2;++ch)juce::FloatVectorOperations::fill(b.getWritePointer(ch),1.e-10f,blockSize);
                const auto start=std::chrono::steady_clock::now();p->processBlock(b,midi);
                const auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
                if(n<64/scale)early+=us;if(n>=1152/scale){late+=us;peak=std::max(peak,double(b.getMagnitude(0,blockSize)));}
                require(p->getLatencySamples()==latency,"Latency drift");
                require(std::isfinite(b.getMagnitude(0,blockSize)),"Nonfinite output");
                if(hostCase!=0)host.sample+=blockSize;
            }
            std::cout<<c.name<<','<<(hostCase==0?"stopped-fixed":hostCase==1?"stopped-advancing":"playing")<<(residualCheck?",residual_-200,":",zero,")<<latency<<','<<early/(64/scale)<<','<<late/(256/scale)<<','<<peak<<"\n";
            require(residualCheck && hostCase==2 ? peak>0 : peak<1.e-20,"Residual sleep or active audio processing failed");p->releaseResources();p->setPlayHead(nullptr);
        }
        std::cout<<"PASS actual VST3 silence finite/drained and unchanged latency. Timings are diagnostic, not Cubase meter percentages.\n";
        return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
