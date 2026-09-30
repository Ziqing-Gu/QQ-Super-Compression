    static void oversampling1236(const juce::File& root)
    {
        using P=QQSuperCompressionAudioProcessor;
        const auto treeFrom=[](P& p)
        {
            juce::MemoryBlock b;p.getStateInformation(b);
            return juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(b.getData(),int(b.getSize())));
        };
        const auto load=[](P& p,const juce::ValueTree& t)
        {
            juce::MemoryBlock b;juce::AudioProcessor::copyXmlToBinary(*t.createXml(),b);
            p.setStateInformation(b.getData(),int(b.getSize()));
        };
        const auto put=[](juce::ValueTree& t,const char* id,int value)
        {auto n=t.getChildWithProperty("id",id);check(n.isValid(),"missing state fixture parameter");n.setProperty("value",value,nullptr);};
        const auto checkValues=[](P& p,int core,int limiter,int ceiling)
        {
            check(get(p,"oversampling")==core,"Normal Core OS recall");
            check(get(p,"limiterOversampling")==limiter,"Limiter Core OS recall");
            check(get(p,"ceilingOversampling")==ceiling,"Ceiling OS recall");
        };
        constexpr int oldToNew[]={0,2,3};
        for(int old=0;old<3;++old)
        {
            P donor;auto tree=treeFrom(donor);
            tree.setProperty("qqscStateSchemaVersion",27,nullptr);
            put(tree,"oversampling",old);put(tree,"limiterOversampling",2-old);put(tree,"ceilingOversampling",old);
            for(const auto& prefix:{juce::String("qqscAB_A_"),juce::String("qqscAB_B_")})
            {
                const bool b=prefix.contains("B_");const int v=b?2-old:old;
                tree.setProperty(prefix+"oversampling",v,nullptr);
                tree.setProperty(prefix+"limiterOversampling",2-v,nullptr);
                tree.setProperty(prefix+"ceilingOversampling",v,nullptr);
            }
            P q;load(q,tree);checkValues(q,oldToNew[old],oldToNew[2-old],oldToNew[old]);
            q.selectABSlot(1);checkValues(q,oldToNew[2-old],oldToNew[old],oldToNew[2-old]);
            q.selectABSlot(0);checkValues(q,oldToNew[old],oldToNew[2-old],oldToNew[old]);
            auto migrated=treeFrom(q);P again;load(again,migrated);
            checkValues(again,oldToNew[old],oldToNew[2-old],oldToNew[old]);
        }
        for(int os=0;os<4;++os)
        {
            P p;set(p,"oversampling",float(os));set(p,"limiterOversampling",float(3-os));set(p,"ceilingOversampling",float(os));
            p.copyAToB();set(p,"oversampling",float(3-os));set(p,"ceilingOversampling",float(3-os));
            auto tree=treeFrom(p);P q;load(q,tree);
            checkValues(q,3-os,3-os,3-os);q.selectABSlot(1);checkValues(q,os,3-os,os);
        }
        // Missing fields must reset even when loading into an already edited instance.
        {P p;auto tree=treeFrom(p);tree.setProperty("qqscStateSchemaVersion",27,nullptr);
         tree.removeChild(tree.getChildWithProperty("id","ceilingOversampling"),nullptr);
         tree.removeChild(tree.getChildWithProperty("id","oversampling"),nullptr);
         set(p,"ceilingOversampling",0);set(p,"oversampling",0);load(p,tree);
         check(get(p,"ceilingOversampling")==2 && get(p,"oversampling")==2,"missing OS does not default to 8x");}
        // Old discrete VST automation points 0/.5/1 still select 1x/8x/16x.
        {P p;for(const char* id:{"oversampling","limiterOversampling","ceilingOversampling"})
         for(int old=0;old<3;++old){p.getAPVTS().getParameter(id)->setValueNotifyingHost(old*.5f);
          check(get(p,id)==oldToNew[old],"legacy normalised automation point changed factor");}}
        std::cout<<"PASS 1.2.36 old state + both A/B banks, new 4-choice roundtrip, missing defaults, old discrete automation points\n";

        for(int core=0;core<4;++core)for(int ceilingOs=0;ceilingOs<4;++ceilingOs)
        {
            P p;configure(p,0,0,0);set(p,"limiterOversampling",float(core));set(p,"ceilingOversampling",float(ceilingOs));
            p.prepareToPlay(48000,17);
            check(p.currentOversamplingFactor==qqsc::params::oversamplingFactorForChoiceIndex(core),"wrong prepared Core factor");
            check(p.outputCeiling.oversamplingFactor()==qqsc::params::ceilingOversamplingFactorForChoiceIndex(ceilingOs),"wrong Ceiling factor");
            check(p.currentCeilingSharesCoreOversampling==(core==ceilingOs && core>0),"wrong shared/fallback topology");
            const int latency=p.getLatencySamples();
            if(ceilingOs>0)
            {
                set(p,"truePeakLimiting",1);p.updateProcessingConfiguration();
                check(p.getLatencySamples()==latency,"fixed OS Hard/TP PDC changed");
            }
            stream(p,48000,{1,17,63,512,2048},4096);
        }
        {P p;configure(p,0,3,3);set(p,"limiterLookaheadMs",26);p.prepareToPlay(48000,128);
         check(p.currentOversamplingFactor==1 && !p.currentCeilingSharesCoreOversampling,"nonzero lookahead must keep native core");
         check(p.outputCeiling.oversamplingFactor()==4,"independent 4x Ceiling lost at nonzero lookahead");}
        std::cout<<"PASS 1.2.36 all 16 Core/Ceiling combinations, variable blocks, shared 4x, fixed-quality Hard/TP PDC\n";

        // Exercise the actual editor's selectors without opening a native window.
        {P p;configure(p,0,0,0);juce::PropertiesFile::Options o;o.storageFormat=juce::PropertiesFile::storeAsXML;
         auto props=std::make_unique<juce::PropertiesFile>(root.getChildFile("os1236-ui.settings"),o);
         QQSuperCompressionAudioProcessorEditor editor(p,std::move(props));
         for(int expected:{1,2,3,0}){editor.cycleOversampling();check(get(p,"limiterOversampling")==expected,"Core UI cycle");}
         for(int expected:{1,2,3,0}){editor.cycleCeilingOversampling();check(get(p,"ceilingOversampling")==expected,"Hard UI cycle");}
         set(p,"truePeakLimiting",1);set(p,"ceilingOversampling",1);
         for(int expected:{2,3,1}){editor.cycleCeilingOversampling();check(get(p,"ceilingOversampling")==expected,"TP UI cycle");}
         editor.updateCeilingOversamplingUi();check(editor.ceilingOversamplingButton.getButtonText()=="4x","Ceiling UI 4x label");}
        std::cout<<"PASS 1.2.36 actual Core/Hard/TP button cycles and 4x label\n";

        for(bool limiter:{false,true})for(int os:{1,2,3})
        {
            P p;configure(p,0,0,0,limiter);set(p,p.soundParameterID("oversampling"),float(os));
            set(p,"ceilingOversampling",float(os));set(p,"truePeakLimiting",1);p.setEcoMode(true);p.prepareToPlay(48000,256);
            juce::MidiBuffer midi;juce::AudioBuffer<float> source(2,256),audio(2,256);
            for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)source.setSample(ch,i,float(.5*std::sin(2*pi*(ch?733:997)*i/48000.)));
            for(int n=0;n<64;++n){audio.makeCopyOf(source,true);p.processBlock(audio,midi);}
            constexpr int count=256;const auto start=std::chrono::steady_clock::now();
            for(int n=0;n<count;++n){audio.makeCopyOf(source,true);p.processBlock(audio,midi);}
            const double us=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()*1e6/count;
            std::cout<<"OS1236 PROFILE limiter="<<limiter<<" factor="<<p.currentOversamplingFactor<<" us_per_block="<<us<<"\n";
        }
    }
