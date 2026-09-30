#include "PluginEditor.h"
#include <iostream>
#include <fstream>
#include <windows.h>

// Documentation-only offscreen capture, linked to the validated 1.2.36 shared code.
// No production source is modified and no host/editor window is created.
struct QQSCVisualCheck
{
    struct PlayHead:juce::AudioPlayHead
    {
        int64_t sample=0;
        juce::Optional<PositionInfo> getPosition() const override
        {PositionInfo p;p.setIsPlaying(true);p.setTimeInSamples(sample);return p;}
    };
    static void pump()
    {
        MSG message;
        while(PeekMessage(&message,nullptr,0,0,PM_REMOVE))
        { TranslateMessage(&message);DispatchMessage(&message); }
    }
    static void set(QQSuperCompressionAudioProcessor& p, const char* id, float v)
    {
        auto* parameter=p.getAPVTS().getParameter(id);
        if(!parameter) throw std::runtime_error(id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
    }
    static void save(QQSuperCompressionAudioProcessorEditor& e, const juce::File& dir,
                     const juce::String& name, juce::Rectangle<int> area)
    {
        // The component has no native peer, so render the same production
        // projection explicitly after asynchronous history replay settles.
        for(int i=0;i<100;++i){pump();juce::Thread::sleep(1);}
        e.display.refreshRenderCaches(juce::roundToInt(e.processor.readSoundParameter(qqsc::params::processingMode)));
        auto image=e.createComponentSnapshot(area,true,2.0f);
        auto output=dir.getChildFile(name+".png").createOutputStream();
        if(!output || !output->setPosition(0) || output->truncate().failed()
            || !juce::PNGImageFormat().writeImageToStream(image,*output))
            throw std::runtime_error("Capture failed");
        std::cout<<name<<".png | logical crop "<<area.toString()
                 <<" | pixels "<<image.getWidth()<<"x"<<image.getHeight()<<"\n";
    }
    static int run(const juce::File& dir)
    {
        if(dir.createDirectory().failed()) return 3;
        juce::PropertiesFile::Options options;
        options.storageFormat=juce::PropertiesFile::storeAsXML;
        // The override exists before processor/editor construction. The processor
        // only reads global Lookahead; every capture resets it explicitly below.
        auto isolatedSettings=std::make_unique<juce::PropertiesFile>
            (dir.getChildFile("documentation-only.settings"),options);
        isolatedSettings->setValue("uiTheme","light");
        isolatedSettings->setValue("landscapeEditorWidth",1200);
        isolatedSettings->setValue("dualRatioLink",true);
        isolatedSettings->setValue("inputOutputLink",true);
        isolatedSettings->setValue("lastAlgorithmMode",0);
        QQSuperCompressionAudioProcessor processor;
        PlayHead playhead;processor.setPlayHead(&playhead);
        processor.setRateAndBufferSizeDetails(48000,800);
        processor.prepareToPlay(48000,800);
        QQSuperCompressionAudioProcessorEditor editor(processor,std::move(isolatedSettings));
        editor.stopTimer(); editor.display.stopTimer(); editor.setSize(1200,800);
        editor.theme=qqsc::ui::Theme::light; editor.applyTheme();
        set(processor,"processingMode",qqsc::params::stereoLinked);
        set(processor,"compressionMode",qqsc::params::singleCompression);
        set(processor,"lookaheadMs",26); set(processor,"inputGainDb",0);
        set(processor,"outputGainDb",0); set(processor,"mix",100);
        set(processor,"domainLink",0); set(processor,"dualRatioLink",1);
        set(processor,"inputOutputLink",1); set(processor,"algorithmMode",0);
        juce::AudioBuffer<float> audio(2,800); juce::MidiBuffer midi;
        auto feed=[&]
        {
            editor.timerCallback();
            for(int frame=0;frame<520;++frame)
            {
                for(int i=0;i<800;++i)
                {
                    const double t=(frame*800+i)/48000.0;
                    const double pulse=std::pow(.5+.5*std::sin(t*6.4),2.5);
                    const double phrase=.4+.6*std::pow(.5+.5*std::sin(t*2.3+.7),1.7);
                    const float level=static_cast<float>((.008+.26*pulse)*phrase);
                    audio.setSample(0,i,level*static_cast<float>(std::sin(t*1137)+.3*std::sin(t*2763)));
                    audio.setSample(1,i,level*static_cast<float>(std::sin(t*1137+.25)+.26*std::sin(t*2701)));
                }
                processor.processBlock(audio,midi);
                playhead.sample+=800;
                processor.refreshMatchResults();
                editor.display.timerCallback();
                if(frame%16==0){pump();juce::Thread::sleep(1);}
            }
            editor.timerCallback();
        };
        set(processor,"ratio",5); set(processor,"thresholdDb",-28);
        set(processor,"rangeDb",qqsc::params::rangeOffDb); feed();
        save(editor,dir,"manual-light-ST",editor.getLocalBounds());
        save(editor,dir,"manual-light-controls",{16,616,1168,168});
        save(editor,dir,"manual-light-display",{16,76,1168,536});
        save(editor,dir,"manual-theme-button",editor.themeButton.getBounds().expanded(10,8));
        save(editor,dir,"manual-algorithm-button",editor.algorithmButton.getBounds().expanded(10,8));
        save(editor,dir,"manual-top-controls",{475,13,703,53});
        save(editor,dir,"manual-performance-full",editor.performanceModeButton.getBounds().expanded(12,10));
        processor.setEcoMode(true);editor.timerCallback();
        save(editor,dir,"manual-performance-eco",editor.performanceModeButton.getBounds().expanded(12,10));
        processor.setEcoMode(false);editor.timerCallback();
        set(processor,"algorithmMode",1); feed();
        save(editor,dir,"manual-light-super",editor.getLocalBounds());
        set(processor,"algorithmMode",0);
        set(processor,"inputGainDb",3); set(processor,"outputGainDb",-3); feed();
        save(editor,dir,"manual-input-output-link",{16,616,1168,168});
        set(processor,"inputGainDb",0); set(processor,"outputGainDb",0);
        set(processor,"keyGainDb",3); set(processor,"keyHpfHz",100);
        editor.toggleSidechainPanel(); editor.timerCallback();
        save(editor,dir,"manual-sidechain-panel",editor.sidechainPanelBounds.expanded(5,5));
        editor.toggleSidechainPanel(); set(processor,"keyGainDb",0); set(processor,"keyHpfHz",0);
        set(processor,"ratio",.125f); set(processor,"thresholdDb",-50); set(processor,"rangeDb",-10); feed();
        save(editor,dir,"manual-light-single-up",editor.getLocalBounds());
        set(processor,"compressionMode",qqsc::params::dualCompression);
        for(int domain=0;domain<5;++domain)
        {
            const auto d=static_cast<size_t>(domain);
            set(processor,qqsc::params::upThresholdIds[d],-50);
            set(processor,qqsc::params::downThresholdIds[d],-20);
            set(processor,qqsc::params::upRatioIds[d],.125f);
            set(processor,qqsc::params::downRatioIds[d],4);
            set(processor,qqsc::params::upEnabledIds[d],1);
            set(processor,qqsc::params::downEnabledIds[d],1);
        }
        feed(); save(editor,dir,"manual-light-dual",editor.getLocalBounds());
        save(editor,dir,"manual-light-dual-controls",{190,615,254,164});
        set(processor,"downEnabled",0); feed();
        save(editor,dir,"manual-dual-up-only",{190,615,254,164});
        set(processor,"upEnabled",0); set(processor,"downEnabled",1); feed();
        save(editor,dir,"manual-dual-down-only",{190,615,254,164});
        set(processor,"upEnabled",1);
        set(processor,"processingMode",qqsc::params::leftRight); feed();
        save(editor,dir,"manual-light-dual-LR",editor.getLocalBounds());
        set(processor,"processingMode",qqsc::params::midSide); feed();
        save(editor,dir,"manual-light-dual-MS",editor.getLocalBounds());
        set(processor,"compressionMode",qqsc::params::singleCompression);
        set(processor,"processingMode",qqsc::params::leftRight);
        set(processor,"ratioL",3); set(processor,"ratioR",5);
        set(processor,"thresholdLDb",-28); set(processor,"thresholdRDb",-20);
        set(processor,"rangeLDb",qqsc::params::rangeOffDb);
        set(processor,"rangeRDb",qqsc::params::rangeOffDb);
        set(processor,"domainLink",1); feed();
        save(editor,dir,"manual-light-LR",editor.getLocalBounds());
        save(editor,dir,"manual-light-LR-link",{16,616,1168,168});
        set(processor,"processingMode",qqsc::params::midSide);
        set(processor,"ratioM",5); set(processor,"ratioS",3);
        set(processor,"thresholdMDb",-28); set(processor,"thresholdSDb",-32);
        set(processor,"rangeMDb",qqsc::params::rangeOffDb);
        set(processor,"rangeSDb",qqsc::params::rangeOffDb);
        set(processor,"domainLink",0); feed();
        save(editor,dir,"manual-light-MS",editor.getLocalBounds());
        set(processor,"processingMode",qqsc::params::stereoLinked);
        set(processor,"ratio",5); set(processor,"thresholdDb",-28);
        set(processor,"rangeDb",qqsc::params::rangeOffDb);
        set(processor,"lookaheadMs",0); set(processor,"oversampling",qqsc::params::os4x); feed();
        save(editor,dir,"manual-light-0ms",editor.getLocalBounds());
        save(editor,dir,"manual-light-0ms-controls",{1008,616,176,170});
        set(processor,"lookaheadMs",26); set(processor,"ratio",.125f);
        set(processor,"thresholdDb",-50); set(processor,"rangeDb",-10); feed();
        editor.theme=qqsc::ui::Theme::dark; editor.applyTheme();
        save(editor,dir,"manual-dark",editor.getLocalBounds());
        editor.theme=qqsc::ui::Theme::classic; editor.applyTheme();
        save(editor,dir,"manual-classic",editor.getLocalBounds());

        editor.theme=qqsc::ui::Theme::light;editor.applyTheme();
        set(processor,"ratio",20);set(processor,"rangeDb",qqsc::params::rangeOffDb);
        set(processor,"makeupGainDb",0);set(processor,"outputGainDb",0);set(processor,"mix",100);
        processor.enterLimiterMode();
        set(processor,"limiterAlgorithmMode",0);set(processor,"limiterCompressionMode",0);
        set(processor,"limiterProcessingMode",0);set(processor,"limiterLookaheadMs",26);
        set(processor,"limiterRatio",100);set(processor,"limiterOutputDb",18);set(processor,"limiterMakeupGainDb",0);set(processor,"limiterMix",100);
        processor.setBoundaryForDomainDb(false,false,0,-18);
        set(processor,"ceilingDb",-1);set(processor,"truePeakLimiting",1);
        set(processor,"ceilingOversampling",qqsc::params::ceiling8x);set(processor,"tpRecoveryMode",1);
        editor.updateCompressionUi();feed();
        save(editor,dir,"manual-limiter-light",editor.getLocalBounds());
        save(editor,dir,"manual-limiter-top",{475,13,703,53});
        save(editor,dir,"manual-limiter-controls",{16,616,1168,176});
        save(editor,dir,"manual-limiter-ceiling-tp",{868,618,140,172});
        save(editor,dir,"manual-limiter-tp-meter",{978,78,198,96});
        save(editor,dir,"manual-limiter-lufs",editor.display.getLoudnessReadoutBounds().translated(float(editor.display.getX()),float(editor.display.getY())).toNearestInt().expanded(5));
        save(editor,dir,"manual-recovery-auto",{880,740,132,49});
        set(processor,"tpRecoveryMode",0);editor.timerCallback();
        save(editor,dir,"manual-recovery-tight",{880,740,132,49});
        set(processor,"tpRecoveryMode",2);editor.timerCallback();
        save(editor,dir,"manual-recovery-smooth",{880,740,132,49});
        set(processor,"tpRecoveryMode",1);
        set(processor,"limiterLookaheadMs",0);set(processor,"limiterOversampling",qqsc::params::os4x);
        set(processor,"ceilingOversampling",qqsc::params::ceiling4x);feed();
        save(editor,dir,"manual-limiter-os",{1010,616,174,175});
        set(processor,"limiterLookaheadMs",26);set(processor,"ceilingOversampling",qqsc::params::ceiling8x);feed();
        processor.setCompressionModeFromEditor(qqsc::params::dualCompression);
        set(processor,"limiterDownRatio",100);set(processor,"limiterUpRatio",1.0f/1.2f);
        set(processor,"limiterDownAlgorithmMode",0);set(processor,"limiterUpAlgorithmMode",0);
        set(processor,"limiterDualRatioLink",0);set(processor,"limiterLink",1);
        processor.setBoundaryForDomainDb(true,false,0,-50);
        editor.updateCompressionUi();feed();
        save(editor,dir,"manual-limiter-dual-gentle",editor.getLocalBounds());
        processor.setCompressionModeFromEditor(qqsc::params::singleCompression);
        editor.updateCompressionUi();feed();
        set(processor,"truePeakLimiting",0);feed();
        save(editor,dir,"manual-limiter-ceiling-peak",{868,618,140,172});
        set(processor,"truePeakLimiting",1);processor.setUnityMonitorEnabled(true);feed();
        if(!processor.applyMatchForCurrentMode())throw std::runtime_error("Monitor Match capture had no valid data");
        feed();
        save(editor,dir,"manual-limiter-monitor",editor.getLocalBounds());
        save(editor,dir,"manual-limiter-monitor-controls",{16,616,1168,176});
        save(editor,dir,"manual-limiter-monitor-output",{855,618,153,174});
        editor.theme=qqsc::ui::Theme::dark;editor.applyTheme();
        save(editor,dir,"manual-limiter-dark",editor.getLocalBounds());
        editor.theme=qqsc::ui::Theme::classic;editor.applyTheme();
        save(editor,dir,"manual-limiter-classic",editor.getLocalBounds());
        std::cout<<"PROVENANCE: production 1.2.36 shared code; 1200x800 logical; 2x PNG; "
                    "48000Hz/800-sample blocks; 520 varying audio blocks per scenario; isolated D-drive settings.\n";
        processor.releaseResources(); return 0;
    }
};
int main(int argc,char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if(argc!=2)return 2;
    try{return QQSCVisualCheck::run(juce::File(juce::String::fromUTF8(argv[1])));}
    catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
