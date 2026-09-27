#define QQSC_SKIP_LIMITER_VST_MAIN
#include "limiter_vst_check.cpp"

juce::ValueTree componentState(juce::AudioPluginInstance& p)
{
    juce::MemoryBlock state;p.getStateInformation(state);
    auto wrapper=juce::AudioProcessor::getXmlFromBinary(state.getData(),int(state.getSize()));
    require(wrapper!=nullptr,"Wrapper state");
    auto* component=wrapper->getChildByName("IComponent");require(component!=nullptr,"Component state");
    juce::MemoryBlock data;require(data.fromBase64Encoding(component->getAllSubText()),"Decode state");
    auto xml=juce::AudioProcessor::getXmlFromBinary(data.getData(),int(data.getSize()));
    require(xml!=nullptr,"XML state");return juce::ValueTree::fromXml(*xml);
}

int main(int argc,char** argv)
{
    if(argc!=3&&argc!=4)return 2;juce::ScopedJuceInitialiser_GUI gui;std::cout<<std::unitbuf;
    try
    {
        juce::VST3PluginFormat format;
        std::array<juce::OwnedArray<juce::PluginDescription>,2> desc;
        std::array<std::unique_ptr<juce::AudioPluginInstance>,2> p;
        for(int j=0;j<2;++j)
        {
            format.findAllTypesForFile(desc[j],argv[j+1]);require(desc[j].size()==1,"One plugin per bundle");
            juce::String error;p[j]=format.createInstanceFromDescription(*desc[j][0],48000,256,error);require(p[j]!=nullptr,error);
        }
        if(argc==4)
        {
            // Characterise the accepted binary alone with neighbouring physical
            // Ratio values. The adaptive guard can amplify float-level changes
            // in an overloaded periodic input; this is not an exact-null test.
            limiterState(*p[0],1,0,false,false,200.f,18.f);
            limiterState(*p[1],1,0,false,false,200.0001f,18.f);
            const auto a=renderPromotion(*p[0],0,127,48000),b=renderPromotion(*p[1],0,127,48000);
            double error=0;for(size_t i=0;i<a.size();++i)error=std::max(error,std::abs(double(a[i])-b[i]));
            std::cout<<"BASELINE ONLY adjacent ratios "<<componentState(*p[0]).getChildWithProperty("id","limiterRatio").getProperty("value").toString()
                     <<" vs "<<componentState(*p[1]).getChildWithProperty("id","limiterRatio").getProperty("value").toString()
                     <<"; peak output residual="<<error<<"\n";
            return 0;
        }
        require(desc[0][0]->uniqueId==desc[1][0]->uniqueId&&desc[1][0]->name=="QQ Super Compression","Plugin identity changed");
        require(p[1]->getParameters().size()==p[0]->getParameters().size()+4,"Expected exactly four appended branch-algorithm parameters");
        for(int i=0;i<p[0]->getParameters().size();++i)
            require(p[0]->getHostedParameter(i)->getParameterID()==p[1]->getHostedParameter(i)->getParameterID(),"Parameter ID/order changed");
        int cases=0;double worst=0;
        for(int algo:{0,1})for(int dual:{0,1})for(int mode:{0,1,2})
            for(float ratio:{.005f,.125f,1.f,8.f,200.f})for(float range:{1.f,-6.f})
        {
            const float threshold=cases%3==0?(algo?-120.f:-90.f):cases%3==1?-40.96f:-12.f;
            const int blocks[]={17,256,1024};const double rates[]={44100,48000,96000};
            for(auto& plugin:p)configure(*plugin,algo,dual,mode,threshold,ratio,range);
            const auto old=renderPromotion(*p[0],mode,blocks[cases%3],rates[cases%3]);
            const auto next=renderPromotion(*p[1],mode,blocks[cases%3],rates[cases%3]);
            require(old.size()==next.size()&&p[0]->getLatencySamples()==p[1]->getLatencySamples(),"Sample count or latency changed");
            double error=0,peak=1;
            for(size_t i=0;i<old.size();++i){error=std::max(error,std::abs(double(old[i])-next[i]));peak=std::max(peak,std::abs(double(old[i])));}
            worst=std::max(worst,error/peak);require(error/peak<2.e-5,"Within-range audio regression");++cases;
        }
        std::cout<<"PASS: "<<cases<<" real VST3 normal-mode cases, same identity/parameter order/latency, max relative peak residual="<<worst<<".\n";
        // Actual old-wrapper state restores physical values, even though the
        // normalised host ranges changed. Former out-of-range values clamp.
        configure(*p[0],0,1,2,-32,.001f,1);
        juce::MemoryBlock oldState;p[0]->getStateInformation(oldState);p[1]->setStateInformation(oldState.getData(),int(oldState.getSize()));
        auto migrated=componentState(*p[1]);
        for(auto suffix:{"","L","R","M","S"})
        {
            const juce::String s(suffix);
            require(std::abs(float(migrated.getChildWithProperty("id","ratio"+s).getProperty("value"))-.005f)<1.e-6,"Old Single ratio migration");
            require(std::abs(float(migrated.getChildWithProperty("id","upRatio"+s).getProperty("value"))-.005f)<1.e-6,"Old Up ratio migration");
        }
        std::cout<<"PASS: actual previous VST3 wrapper state and ratio limit migration.\n";
        worst=0;int limiterCases=0;
        for(int algo:{0,1})for(int mode:{0,1,2})for(bool tp:{false,true})for(float ratio:{200.f,1000.f})
        {
            for(int j=0;j<2;++j)
            {
                p[j].reset();juce::String error;
                p[j]=format.createInstanceFromDescription(*desc[j][0],48000,256,error);require(p[j]!=nullptr,error);
            }
            // Use physical state values to avoid different host parsers.
            limiterState(*p[0],algo,mode,tp,false,ratio,ratio==200.f ? 0.f : 18.f);
            juce::MemoryBlock migration;p[0]->getStateInformation(migration);p[1]->setStateInformation(migration.getData(),int(migration.getSize()));
            const auto a=renderPromotion(*p[0],mode,127,48000),b=renderPromotion(*p[1],mode,127,48000);
            require(a.size()==b.size(),"Limiter sizes");double error=0,peak=1,tail=0;size_t worstIndex=0;
            for(size_t i=0;i<a.size();++i)
            {
                const auto difference=std::abs(double(a[i])-b[i]);
                if(difference>error){error=difference;worstIndex=i;}
                if(i>12000)tail=std::max(tail,difference);
                peak=std::max(peak,std::abs(double(a[i])));
            }
            worst=std::max(worst,error/peak);
            std::cout<<"Limiter comparison algo="<<algo<<" mode="<<mode<<" TP="<<tp<<" Ratio="<<ratio<<" error="<<error<<" peak="<<peak<<" tail="<<tail<<" at="<<worstIndex<<" old="<<a[worstIndex]<<" new="<<b[worstIndex]<<"\n";
            if(error/peak>=2.e-5)
            {
                auto old=componentState(*p[0]),next=componentState(*p[1]);
                for(auto c:old)
                {
                    const auto id=c.getProperty("id");auto other=next.getChildWithProperty("id",id);
                    if(other.isValid()&&float(c.getProperty("value"))!=float(other.getProperty("value")))
                        std::cout<<id.toString()<<": "<<c.getProperty("value").toString()<<" -> "<<other.getProperty("value").toString()<<"\n";
                }
            }
            require(error/peak<2.e-5,"Limiter 200:1 audio regression");++limiterCases;
        }
        std::cout<<"PASS: "<<limiterCases<<" actual VST3 Limiter cases, 200:1 without overload and exactly representable 1000:1 with overload, Classic/Super, ST/LR/MS, TP/Peak; residual="<<worst<<".\n";
        for(auto& plugin:p)plugin.reset();return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
