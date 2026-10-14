    static void stLinkChecks(const juce::File& root)
    {
        float worst=0;int cases=0;
        for(bool external:{false,true})for(int os:{0,2})for(int look:{0,26})for(bool dual:{false,true})for(int algorithm:{0,1})
        {
            // Independent processors are the oracle for the two original paths.
            std::array<std::unique_ptr<QQSuperCompressionAudioProcessor>,7> p;
            for(int n=0;n<7;++n)
            {
                p[n]=std::make_unique<QQSuperCompressionAudioProcessor>();auto& x=*p[n];x.setEcoMode(true);
                if(external){auto layout=x.getBusesLayout();layout.inputBuses.set(1,juce::AudioChannelSet::stereo());require(x.setBusesLayout(layout),"ST Link EXT layout");}
                value(x,"keySource",external?1.f:0.f);
                value(x,"processingMode",n==0?2.f:n==1?1.f:0.f);
                value(x,"stLRLink",n==3 || n==5?100.f:n==6?30.f:0.f);value(x,"stMSLink",n==2 || n==5?100.f:n==6?70.f:0.f);
                value(x,"lookaheadMs",float(look));value(x,"oversampling",float(os));value(x,"distort",37);
                value(x,"keyHpfHz",qqsc::params::keyHpfOffHz);value(x,"algorithmMode",float(algorithm));
                value(x,"compressionMode",dual?1.f:0.f);value(x,"upAlgorithmMode",float(algorithm));value(x,"downAlgorithmMode",float(algorithm));
                for(auto id:qqsc::params::ratioIds)value(x,id,4);
                for(auto id:qqsc::params::upRatioIds)value(x,id,.5f);
                for(auto id:qqsc::params::downRatioIds)value(x,id,4);
                for(auto id:qqsc::params::thresholdIds)value(x,id,-18);
                for(auto id:qqsc::params::upThresholdIds)value(x,id,-50);
                for(auto id:qqsc::params::downThresholdIds)value(x,id,-18);
                for(auto id:qqsc::params::rangeIds)value(x,id,qqsc::params::rangeOffDb);
                for(auto id:{"mix","mixL","mixR","mixM","mixS"})value(x,id,73);
                for(auto id:{"makeupGainDb","makeupGainLDb","makeupGainRDb","makeupGainMDb","makeupGainSDb"})value(x,id,2);
                value(x,"outputGainDb",-2);x.setRateAndBufferSizeDetails(48000,128);x.prepareToPlay(48000,128);
            }
            juce::MidiBuffer midi;
            for(int block=0;block<140;++block)
            {
                juce::AudioBuffer<float> raw(external?4:2,128);std::array<juce::AudioBuffer<float>,7> audio;
                for(int ch=0;ch<(external?4:2);++ch)for(int i=0;i<128;++i)
                {const auto t=float(block*128+i);raw.setSample(ch,i,.3f*std::sin(t*.057f)+.25f*std::sin(t*(ch?.143f:.073f)));}
                for(int n=0;n<7;++n){audio[n].makeCopyOf(raw);p[n]->processBlock(audio[n],midi);}
                for(int ch=0;ch<2;++ch)for(int i=0;i<128;++i)
                {
                    worst=std::max(worst,std::abs(audio[0].getSample(ch,i)-audio[2].getSample(ch,i)));
                    worst=std::max(worst,std::abs(audio[1].getSample(ch,i)-audio[3].getSample(ch,i)));
                    worst=std::max(worst,std::abs(.5f*(audio[0].getSample(ch,i)+audio[1].getSample(ch,i))-audio[4].getSample(ch,i)));
                    worst=std::max(worst,std::abs(.21f*audio[5].getSample(ch,i)+.595f*audio[0].getSample(ch,i)+.195f*audio[1].getSample(ch,i)-audio[6].getSample(ch,i)));
                }
            }
            ++cases;
        }
        std::cout<<"ST Link endpoints cases="<<cases<<" max audio error="<<worst<<std::endl;
        require(worst<.00001f,"ST Link does not match independent LR/MS audio and 50/50 combination");
        QQSuperCompressionAudioProcessor p;
        juce::PropertiesFile::Options options;options.applicationName="QQSCSTLinkCheck";
        QQSuperCompressionAudioProcessorEditor e(p,std::make_unique<juce::PropertiesFile>(root.getChildFile("st-ui.settings"),options));
        e.stopTimer();e.display.stopTimer();e.updateModeUi();
        require(e.stLinkValues[0].isVisible() && e.stLinkValues[1].isVisible(),"Both ST Link fields visible");
        require(p.readSoundParameter("stLRLink")==100 && p.readSoundParameter("stMSLink")==100,"ST Link defaults");
        for(auto& c:e.stLinkValues)
        {
            require(c.getTooltip().contains(juce::String::fromUTF8(u8"联动")),"ST Link Chinese tooltip");
            c.start();c.move(-300,false);c.finish();
        }
        require(p.readSoundParameter("stLRLink")==70 && p.readSoundParameter("stMSLink")==70,"Both Link drags");
        p.copyAToB();p.selectABSlot(1);value(p,"stLRLink",0);value(p,"stMSLink",35);
        p.selectABSlot(0);require(p.readSoundParameter("stLRLink")==70,"ST Link A recall");
        p.selectABSlot(1);require(p.readSoundParameter("stLRLink")==0 && p.readSoundParameter("stMSLink")==35,"ST Link B recall");
        juce::MemoryBlock saved;p.getStateInformation(saved);
        QQSuperCompressionAudioProcessor restored;restored.setStateInformation(saved.getData(),int(saved.getSize()));
        require(restored.readSoundParameter("stMSLink")==35,"ST Link project recall");
        restored.selectABSlot(0);require(restored.readSoundParameter("stMSLink")==70,"ST Link project inactive A recall");
        for(auto& c:e.stLinkValues)c.reset();
        require(p.readSoundParameter("stLRLink")==100 && p.readSoundParameter("stMSLink")==100,"ST Link Alt reset");
        auto old=juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(saved.getData(),int(saved.getSize())));
        for(int i=old.getNumChildren();--i>=0;){const auto id=old.getChild(i).getProperty("id").toString();if(id=="stLRLink" || id=="stMSLink")old.removeChild(i,nullptr);}
        for(int i=old.getNumProperties();--i>=0;){const auto id=old.getPropertyName(i).toString();if(id.contains("stLRLink") || id.contains("stMSLink"))old.removeProperty(old.getPropertyName(i),nullptr);}
        juce::MemoryBlock legacy;juce::AudioProcessor::copyXmlToBinary(*old.createXml(),legacy);
        restored.setStateInformation(legacy.getData(),int(legacy.getSize()));
        require(restored.readSoundParameter("stLRLink")==100 && restored.readSoundParameter("stMSLink")==100,"Old project must restore ST 100/100");
        e.resized();
        require(!e.modeButton.getBounds().intersects(e.stLinkValues[0].getBounds()) && !e.stLinkValues[0].getBounds().intersects(e.stLinkValues[1].getBounds()),"ST Link fields overlap");
        auto image=e.createComponentSnapshot(e.getLocalBounds());juce::PNGImageFormat png;juce::FileOutputStream out(root.getChildFile("st-links-ui.png"));png.writeImageToStream(image,out);
        p.enterClipperMode();e.updateModeUi();require(!e.stLinkValues[0].isVisible() && !e.stLinkValues[1].isVisible(),"ST links must be hidden outside Compressor ST");
        std::cout<<"PASS: ST Link audio endpoints, UI, Chinese tooltips, A/B, project, legacy restore, drag and Alt reset"<<std::endl;
    }
