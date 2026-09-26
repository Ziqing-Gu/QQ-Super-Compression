#include <JuceHeader.h>
#include <array>
#include <iostream>
#include <cmath>
#include <stdexcept>

void require(bool ok,const juce::String& message){if(!ok)throw std::runtime_error(message.toStdString());}
void configure(juce::AudioPluginInstance& p,int algorithm,int dual,int mode,float threshold,float ratio,float range)
{
    juce::MemoryBlock state;p.getStateInformation(state);
    auto wrapper=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
    require(wrapper&&wrapper->hasTagName("VST3PluginState"),"VST3 wrapper");
    auto* component=wrapper->getChildByName("IComponent");require(component,"Component missing");
    juce::MemoryBlock data;require(data.fromBase64Encoding(component->getAllSubText()),"Component decode");
    const size_t xmlBytes=9+juce::ByteOrder::littleEndianInt(static_cast<const char*>(data.getData())+4);
    require(xmlBytes<=data.getSize(),"Component length");
    juce::MemoryBlock trailer(static_cast<const char*>(data.getData())+xmlBytes,data.getSize()-xmlBytes);
    auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));require(xml!=nullptr,"Processor XML");
    auto tree=juce::ValueTree::fromXml(*xml);
    const auto put=[&](const juce::String& id,float value){auto c=tree.getChildWithProperty("id",id);require(c.isValid(),"Parameter "+id);c.setProperty("value",value,nullptr);};
    if(tree.getChildWithProperty("id","algorithmMode").isValid())put("algorithmMode",float(algorithm));
    for(auto suffix:{"","L","R","M","S"})
    {
        const juce::String s(suffix);
        put("ratio"+s,ratio);put("threshold"+s+"Db",threshold);put("range"+s+"Db",range);
        put("upThreshold"+s+"Db",threshold);put("downThreshold"+s+"Db",-6);
        put("upRatio"+s,std::min(ratio,1.0f));put("downRatio"+s,std::max(ratio,1.0f));
        put("upEnabled"+s,1);put("downEnabled"+s,1);put("mix"+s,100);
        put(s.isEmpty()?"makeupGainDb":"makeupGain"+s+"Db",0);
    }
    put("compressionMode",float(dual));put("inputGainDb",0);put("outputGainDb",0);
    put("lookaheadMs",26);put("oversampling",0);put("processingMode",float(mode));
    put("bypass",0);put("keySource",0);put("keyGainDb",0);put("keyHpfHz",0);
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(),data);data.append(trailer.getData(),trailer.getSize());
    component->deleteAllChildElements();component->addTextElement(data.toBase64Encoding());
    juce::AudioProcessor::copyXmlToBinary(*wrapper,state);p.setStateInformation(state.getData(),int(state.getSize()));
}
int main(int argc,char** argv)
{
    if(argc!=4)return 2;
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        juce::VST3PluginFormat format;
        std::array<juce::OwnedArray<juce::PluginDescription>,3> desc;
        std::array<std::unique_ptr<juce::AudioPluginInstance>,3> plugins;
        for(int i=0;i<3;++i)
        {
            format.findAllTypesForFile(desc[i],argv[i+1]);require(desc[i].size()==1,"One plugin per bundle");
            juce::String error;plugins[i]=format.createInstanceFromDescription(*desc[i][0],48000,256,error);require(plugins[i]!=nullptr,error);
        }
        require(desc[0][0]->version=="1.2.5"&&desc[1][0]->version=="1.2.3"&&desc[2][0]->version=="1.2.2","Version / baseline mismatch");
        for(int i:{1,2})require(desc[0][0]->uniqueId==desc[i][0]->uniqueId,"Formal plugin identity changed");
        require(plugins[0]->getParameters().size()==plugins[1]->getParameters().size()+1,"Expected one appended parameter");
        for(int i=0;i<plugins[1]->getParameters().size();++i)
            require(plugins[0]->getHostedParameter(i)->getParameterID()==plugins[1]->getHostedParameter(i)->getParameterID(),"Existing VST3 parameter ID / index changed");
        std::array<double,2> maxError{};int cases=0;
        for(int algo:{0,1})for(int dual:{0,1})for(int mode:{0,1,2})
          for(float ratio:{.001f,.125f,1.f,8.f,1000.f})for(float range:{1.f,-6.f})
        {
            auto& a=*plugins[0];auto& b=*plugins[size_t(algo+1)];
            const float threshold=cases%3==0?(algo==0?-89.99f:-120.f):cases%3==1?-40.f:-10.f;
            configure(a,algo,dual,mode,threshold,ratio,range);configure(b,algo,dual,mode,threshold,ratio,range);
            a.prepareToPlay(48000,256);b.prepareToPlay(48000,256);
            juce::MidiBuffer midi;juce::AudioBuffer<float> x(2,256),y(2,256);
            for(int block=0;block<80;++block)
            {
                for(int ch=0;ch<2;++ch)for(int n=0;n<256;++n)
                {
                    const double t=(block*256+n)/48000.;
                    const float signal=float((.35+.34*std::sin(t*37))*std::sin(t*2*juce::MathConstants<double>::pi*(ch?613:400)));
                    x.setSample(ch,n,signal);y.setSample(ch,n,signal);
                }
                a.processBlock(x,midi);b.processBlock(y,midi);
                for(int ch=0;ch<2;++ch)for(int n=0;n<256;++n)
                    maxError[size_t(algo)]=std::max(maxError[size_t(algo)],double(std::abs(x.getSample(ch,n)-y.getSample(ch,n))));
            }
            require(a.getLatencySamples()==1248&&b.getLatencySamples()==1248,"PDC mismatch");
            a.releaseResources();b.releaseResources();++cases;
        }
        require(maxError[0]<1e-6&&maxError[1]<1e-6,"Candidate differs from released reference audio");
        // Real historical VST3 state, including wrapper trailer and A/B data.
        for(int ref:{1,2})
        {
            juce::MemoryBlock state;plugins[size_t(ref)]->getStateInformation(state);
            plugins[0]->setStateInformation(state.getData(),int(state.getSize()));
            bool found=false;
            for(auto* param:plugins[0]->getParameters())if(param->getName(128)=="Compression Algorithm")
            {require(param->getValue()==float(ref==2),"Actual released state chose wrong algorithm");found=true;}
            require(found,"Hosted algorithm parameter missing");
        }
        auto& current=*plugins[0];configure(current,0,0,0,-120,11.9f,1);current.prepareToPlay(48000,256);
        juce::MidiBuffer midi;juce::AudioBuffer<float> signal(2,256);
        for(int block=0;block<30;++block)
        {
            for(int ch=0;ch<2;++ch)for(int n=0;n<256;++n)signal.setSample(ch,n,.1f);
            current.processBlock(signal,midi);
        }
        const double expected=.1*std::pow(std::pow(10.,-70./20),1-1/11.9);
        require(std::abs(signal.getSample(0,255)/expected-1)<.00005,"Actual VST3 -90dB endpoint not Classic");
        for(int algo:{0,1})
        {
            configure(current,algo,0,0,-120,11.9f,1);
            bool found=false;
            for(auto* param:current.getParameters())if(param->getName(128)=="Threshold ST")
            {require(param->getText(0,128).contains(algo==0?"-90":"-inf"),"Host threshold text not algorithm-aware");found=true;}
            require(found,"Hosted threshold missing");
        }
        current.releaseResources();
        std::cout<<"PASS: actual VST3 identity / parameter continuity; "<<cases<<" Single/Dual ST/LR/MS reference audio cases (finite Classic, full Super). Classic vs1.2.3 max error="<<maxError[0]<<", Super vs1.2.2="<<maxError[1]<<"; old state migration; new -90dB audio and host readout.\n";
        for(auto& p:plugins)p.reset();
        return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
