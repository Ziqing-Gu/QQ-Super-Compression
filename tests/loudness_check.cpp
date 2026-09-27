#define main HistoricalDynamicsMain
#include "updown_processor_test.cpp"
#undef main
#include "PluginEditor.h"
#include <random>

struct LoudnessPlayHead : juce::AudioPlayHead
{
    bool playing = false;
    int64_t sample = 0;
    juce::Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setIsPlaying(playing); p.setTimeInSamples(sample); return p; }
};

static double exactGate(const std::vector<double>& energies)
{
    const double absolute = std::pow(10., (-70.+.691)/10.);
    double sum=0; size_t count=0;
    for (auto e: energies) if(e>absolute){sum+=e;++count;}
    if(!count)return -120;
    const auto gate=std::max(absolute,.1*sum/double(count)); sum=0;count=0;
    for(auto e:energies)if(e>gate){sum+=e;++count;}
    return count ? -.691+10*std::log10(sum/double(count)) : -120;
}

static void measurementChecks()
{
    double worstGate=0;
    qqsc::IntegratedLoudnessMeter meter;
    meter.prepare(48000);
    for(auto rate:{44100.,48000.,96000.})
    {
        meter.prepare(rate);
        for(int i=0;i<int(rate*3);++i)
        {
            auto x=float(std::pow(10.,-23./20.)*std::sin(2*pi*1000*i/rate));
            meter.processSample(x,x);
        }
        check(std::abs(meter.integratedLufs()+23)<.08,"Stereo 1 kHz -23 dBFS calibration");
        const auto stereo=meter.integratedLufs();
        meter.reset();
        for(int i=0;i<int(rate*3);++i)
            meter.processSample(float(std::pow(10.,-23./20.)*std::sin(2*pi*1000*i/rate)),0);
        check(std::abs(stereo-meter.integratedLufs()-3.0103)<.001,"Mono double counted");
        meter.reset();
        for(int i=0;i<int(rate);++i)meter.processSample(0,0);
        check(meter.integratedLufs()==-120,"Silence has a bogus measurement");
        for(int i=0;i<int(rate);++i)meter.processSample(float(1e-5*std::sin(2*pi*1000*i/rate)),0);
        check(meter.integratedLufs()==-120,"Absolute -70 LUFS gate missing");
    }
    std::mt19937 random(20260927);
    for(int scenario=0;scenario<16;++scenario)
    {
        meter.reset();std::vector<double> energies;
        for(int i=0;i<20000;++i)
        {
            const double level=-90.+double(random()%960000)/10000.;
            const double energy=std::pow(10.,(level+.691)/10.);
            energies.push_back(energy);meter.addBlockEnergy(energy);
            if(i%1000==999)
                worstGate=std::max(worstGate,std::abs(double(meter.integratedLufs())-exactGate(energies)));
        }
    }
    check(worstGate<.01,"Gated histogram differs from exact two-pass integration");
    std::cout<<"PASS: calibration 44.1/48/96 kHz, mono weight, silence/absolute gate; 320000 blocks vs exact relative gate, worst "<<worstGate<<" LU.\n";
}

static void transportChecks()
{
    for(int mode:{0,1,2})for(bool unity:{false,true})for(bool bypass:{false,true})
    {
        QQSuperCompressionAudioProcessor p;baseline(p);p.enterLimiterMode();
        set(p,p.soundParameterID("processingMode"),float(mode));
        set(p,"limiterOutputDb",8);set(p,"ceilingDb",-3);set(p,"limiterCalibrationDb",0);
        set(p,p.soundParameterID("mix"),66.9f);
        p.setUnityMonitorEnabled(unity);
        LoudnessPlayHead play;p.setPlayHead(&play);p.prepareToPlay(48000,256);
        qqsc::IntegratedLoudnessMeter reference;reference.prepare(48000);
        juce::MidiBuffer midi;
        auto run=[&](int n,float amplitude,bool playing,bool record)
        {
            play.playing=playing;
            for(int done=0;done<n;)
            {
                const int count=std::min(256,n-done);juce::AudioBuffer<float> b(2,count);
                for(int i=0;i<count;++i)
                {
                    const auto x=amplitude*float(std::sin(2*pi*1000*double(play.sample+i)/48000.));
                    b.setSample(0,i,x);b.setSample(1,i,x*.7f);
                }
                if(bypass)p.processBlockBypassed(b,midi);else p.processBlock(b,midi);
                if(record)for(int i=0;i<count;++i)reference.processSample(b.getSample(0,i),b.getSample(1,i));
                if(playing)play.sample+=count;
                done+=count;
            }
        };
        run(48000,1,false,false);
        check(p.getMeterState().outputLoudnessSeconds.load()==0,"Stopped audio entered LUFS accumulator");
        run(96000,.2f,true,true);
        check(std::abs(p.getMeterState().outputIntegratedLufs.load()-reference.integratedLufs())<.0001,
              "Meter does not measure final emitted audio (Ceiling/monitor/bypass)");
        const auto held=p.getMeterState().outputIntegratedLufs.load();
        run(48000,1,false,false);
        check(p.getMeterState().outputIntegratedLufs.load()==held,"Stop did not hold result");
        check(p.getMeterState().outputLoudnessSeconds.load()==2,"Held duration changed");
        reference.reset();run(256,.01f,true,true);
        check(p.getMeterState().outputIntegratedLufs.load()==-120,"Restart did not clear readout immediately");
        run(96000-256,.01f,true,true);
        check(std::abs(p.getMeterState().outputIntegratedLufs.load()-reference.integratedLufs())<.0001,"New pass includes previous pass");
        play.sample=0;run(256,.01f,true,false);
        check(p.getMeterState().outputIntegratedLufs.load()==-120,"Seek/loop did not start a fresh pass");
        p.leaveLimiterMode();run(48000,.1f,true,false);
        const auto duration=p.getMeterState().outputLoudnessSeconds.load();
        run(48000,.2f,true,false);
        check(p.getMeterState().outputLoudnessSeconds.load()==duration,"Normal mode still measures Limiter LUFS");
    }
    QQSuperCompressionAudioProcessor live;baseline(live);live.enterLimiterMode();live.prepareToPlay(48000,256);
    juce::AudioBuffer<float> b(2,256);b.clear();juce::MidiBuffer m;live.processBlock(b,m);
    check(live.getMeterState().outputLoudnessSeconds.load()>0,"No-transport live host is stuck");
    std::cout<<"PASS: 12 actual-processor ST/LR/MS x 1:1 x bypass cases; final output, stop/hold/restart, seek, normal-mode exclusion, live-host fallback.\n";
}

struct QQSCLoudnessCheck
{
    static void ui(const juce::File& root)
    {
        QQSuperCompressionAudioProcessor p;baseline(p);
        juce::PropertiesFile::Options opts;opts.applicationName="LoudnessCheck";opts.storageFormat=juce::PropertiesFile::storeAsXML;
        QQSuperCompressionAudioProcessorEditor e(p,std::make_unique<juce::PropertiesFile>(root.getChildFile("ui.settings"),opts));
        e.setSize(1200,800);e.resized();
        check(e.display.getLoudnessReadoutBounds().isEmpty(),"Normal-mode LUFS visible");
        e.limiterButton.onClick();e.updateCompressionUi();
        p.getMeterState().outputIntegratedLufs.store(-14.3f);
        p.getMeterState().outputLoudnessSeconds.store(36.0f);
        p.getMeterState().outputLoudnessMeasuring.store(false);
        for(auto theme:{qqsc::ui::Theme::light,qqsc::ui::Theme::dark,qqsc::ui::Theme::classic})
        {
            e.theme=theme;e.applyTheme();e.updateLimiterUi();
            for(int mode:{0,1,2})
            {
                set(p,p.soundParameterID("processingMode"),float(mode));e.updateModeUi();e.updateCompressionUi();e.resized();
                const auto bounds=e.display.getLoudnessReadoutBounds();
                check(e.display.getLocalBounds().toFloat().contains(bounds)&&bounds.getHeight()>80,"Readout clipping");
                auto stream=root.getChildFile(juce::String("lufs-")+qqsc::ui::themeKey(theme)+"-"+juce::String(mode)+".png").createOutputStream();
                check(stream!=nullptr,"Screenshot stream");stream->setPosition(0);stream->truncate();
                juce::PNGImageFormat().writeImageToStream(e.createComponentSnapshot(e.getLocalBounds()),*stream);
            }
        }
        for(int width:{810,1500})
        {
            e.setSize(width,width*2/3);e.resized();
            check(e.display.getLocalBounds().toFloat().contains(e.display.getLoudnessReadoutBounds()),"Resized meter clipping");
        }
        e.limiterButton.onClick();e.updateCompressionUi();
        check(e.display.getLoudnessReadoutBounds().isEmpty(),"Limiter off still shows meter");
        std::cout<<"PASS: offscreen real editor, three themes x ST/LR/MS, resizing and Limiter-only visibility.\n";
    }
};

int main(int argc,char** argv)
{
    if(argc!=2)return 2;std::cout<<std::unitbuf;juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        const juce::File root(argv[1]);root.createDirectory();
        measurementChecks();transportChecks();QQSCLoudnessCheck::ui(root);
        return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
