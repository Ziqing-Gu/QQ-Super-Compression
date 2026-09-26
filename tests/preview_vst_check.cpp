// Exercise both actual VST3 bundles, including their wrapper and latency.
#define main HistoricalReferenceMain
#include "algorithm_vst_check.cpp"
#undef main
#include "StaticCompressionEngine.h"
#include <fstream>
#include <vector>
#include <iomanip>

namespace
{
constexpr double tau = 6.28318530717958647692;
std::vector<float> renderPlugin(juce::AudioPluginInstance& p, const std::vector<float>& input,
                                double rate, int blockSize)
{
    p.setRateAndBufferSizeDetails(rate,blockSize); p.prepareToPlay(rate,blockSize);
    const int delay = int(std::round(rate*.026));
    require(p.getLatencySamples()==delay,"26 ms latency changed");
    std::vector<float> result(input.size()); juce::MidiBuffer midi;
    const int total=int(input.size())+delay;
    for(int n=0;n<total;n+=blockSize)
    {
        const int size=std::min(blockSize,total-n); juce::AudioBuffer<float> b(2,size);
        for(int i=0;i<size;++i)
        {
            const float x=n+i<int(input.size())?input[size_t(n+i)]:0.f;
            b.setSample(0,i,x); b.setSample(1,i,x);
        }
        p.processBlock(b,midi);
        for(int i=0;i<size;++i)
        {
            require(std::isfinite(b.getSample(0,i)),"Nonfinite VST3 output");
            require(b.getSample(0,i)==b.getSample(1,i),"ST channel mismatch");
            if(n+i>=delay) result[size_t(n+i-delay)]=b.getSample(0,i);
        }
    }
    p.releaseResources(); return result;
}
std::vector<float> stepInput(double rate, double hz, double phase)
{
    std::vector<float> x(size_t(rate*4));
    for(size_t i=0;i<x.size();++i)
    {
        const double t=double(i)/rate, amplitude=t>=1&&t<3?1.:.1;
        x[i]=float(amplitude*std::sin(tau*hz*t+phase));
    }
    return x;
}
double gainError(const std::vector<float>& x,const std::vector<float>& y,int begin,int end,double gain)
{
    double error=0;
    for(int i=begin;i<end;++i) if(std::abs(x[size_t(i)])>.015f)
        error=std::max(error,std::abs(20*std::log10(std::max(1e-30,std::abs(double(y[size_t(i)])/x[size_t(i)]/gain)))));
    return error;
}
double thd(const std::vector<float>& x,double rate,double hz)
{
    const int start=int(rate*.4),count=int(rate); double fundamental=0,harmonics=0;
    for(int h=1;h<=7 && hz*h<rate*.5;++h)
    {
        double re=0,im=0;
        for(int i=0;i<count;++i)
        {const double a=tau*hz*h*i/rate;re+=x[size_t(start+i)]*std::cos(a);im+=x[size_t(start+i)]*std::sin(a);}
        if(h==1)fundamental=re*re+im*im; else harmonics+=re*re+im*im;
    }
    return 10*std::log10(std::max(1e-30,harmonics/fundamental));
}
void floats(const juce::File& f,const std::vector<float>& values)
{
    std::ofstream out(f.getFullPathName().toStdString(),std::ios::binary);
    out.write(reinterpret_cast<const char*>(values.data()),std::streamsize(values.size()*sizeof(float)));
    require(bool(out),"Write float result");
}
void modulationRenders(std::array<std::unique_ptr<juce::AudioPluginInstance>,2>& plugins,const juce::File& dir)
{
    std::ofstream manifest(dir.getChildFile("modulation-manifest.csv").getFullPathName().toStdString());
    manifest<<"id,algorithm,ratio,kind,carrier,modulation,depth_db,tone2\n";
    int cases=0;
    for(int algo:{0,1})for(bool upward:{false,true})for(int stimulus=0;stimulus<9;++stimulus)
    {
        const bool twoTone=stimulus>=6;
        const double carriers[]={20,50,50,400,400,1000,400,37,997};
        const double mods[]={1,3,11,3,11,11,0,0,0};
        const double seconds[]={0,0,0,0,0,0,401,73,1201};
        const double carrier=carriers[stimulus],mod=mods[stimulus],tone2=seconds[stimulus];
        const double depth=stimulus==2?20.:10.;
        const float ratio=upward?1.f/12.4f:12.4f;
        std::vector<float> input(144000),ideal(144000);
        for(size_t i=0;i<input.size();++i)
        {
            const double t=double(i)/48000;
            const float amp=float(std::pow(10.,(-25+depth*std::sin(tau*mod*t))/20));
            input[i]=twoTone?float(.09*std::sin(tau*carrier*t+.23)+.075*std::sin(tau*tone2*t+.71)):amp*float(std::sin(tau*carrier*t+.37));
            ideal[i]=input[i]*qqsc::StaticCompressionEngine::singleGainForLevel(amp,ratio,std::pow(10.f,-40.96f/20),
                std::numeric_limits<float>::infinity(),algo?qqsc::CompressionAlgorithm::super:qqsc::CompressionAlgorithm::classic);
        }
        const juce::String id("mod-"+std::to_string(cases));
        for(int j=0;j<2;++j)
        {
            configure(*plugins[j],algo,0,0,-40.96f,ratio,1);
            floats(dir.getChildFile(id+(j?"-preview.f32":"-stable.f32")),renderPlugin(*plugins[j],input,48000,256));
        }
        if(!twoTone)floats(dir.getChildFile(id+"-ideal.f32"),ideal);
        manifest<<id.toStdString()<<','<<(algo?"Super":"Classic")<<','<<ratio<<','<<(twoTone?"two-tone":"AM")<<','<<carrier<<','<<mod<<','<<depth<<','<<tone2<<'\n';
        ++cases;
    }
    require(bool(manifest),"Modulation manifest write");
    std::cout<<"PASS: rendered "<<cases<<" actual VST3 AM/two-tone cases, Classic/Super Up/Down, for spectral analysis.\n";
}
}
int main(int argc,char** argv)
{
    if(argc!=4&&argc!=5)return 2;
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        juce::File destination(juce::String::fromUTF8(argv[3])); require(destination.createDirectory().wasOk(),"Output directory");
        juce::VST3PluginFormat format;
        std::array<juce::OwnedArray<juce::PluginDescription>,2> descriptions;
        std::array<std::unique_ptr<juce::AudioPluginInstance>,2> plugins;
        for(int j=0;j<2;++j)
        {
            format.findAllTypesForFile(descriptions[j],argv[j+1]); require(descriptions[j].size()==1,"Plugin identity");
            juce::String error; plugins[j]=format.createInstanceFromDescription(*descriptions[j][0],48000,256,error);
            require(plugins[j]!=nullptr,error);
        }
        require(descriptions[0][0]->version=="1.2.5"&&descriptions[1][0]->version=="1.2.6","Version mismatch");
        require(descriptions[0][0]->uniqueId!=descriptions[1][0]->uniqueId,"Preview identity must be independent");
        require(descriptions[1][0]->name=="QQ Super Compression Preview","Preview name");
        require(plugins[0]->getParameters().size()==plugins[1]->getParameters().size(),"Unexpected parameter count");
        for(int i=0;i<plugins[0]->getParameters().size();++i)
            require(plugins[0]->getHostedParameter(i)->getParameterID()==plugins[1]->getHostedParameter(i)->getParameterID(),"Parameter layout changed");
        if(argc==5)
        {
            require(juce::String(argv[4])=="--modulation","Unknown test selection");
            modulationRenders(plugins,destination);return 0;
        }
        std::ofstream csv(destination.getChildFile("step-results.csv").getFullPathName().toStdString());
        csv<<"algorithm,threshold,ratio,hz,phase,stable_pre_error_db,preview_pre_error_db,preview_post_error_db,preview_high_peak_dbfs\n"<<std::setprecision(10);
        double worstPre=0,worstPost=0; int cases=0;
        for(int algo:{0,1})for(int setting:{0,1,2,3})for(double hz:{50.,400.,1000.,4341.1})for(double phase:{0.,.37})
        {
            const float threshold=setting==0?-12.f:setting==1?-40.96f:-60.f;
            const float ratio=setting==0?8.f:setting==1?12.4f:setting==2?1000.f:.125f;
            const auto input=stepInput(48000,hz,phase);
            std::array<std::vector<float>,2> output;
            for(int j=0;j<2;++j){configure(*plugins[j],algo,0,0,threshold,ratio,1);output[j]=renderPlugin(*plugins[j],input,48000,256);}
            const auto algorithm=algo==0?qqsc::CompressionAlgorithm::classic:qqsc::CompressionAlgorithm::super;
            const float thresholdLinear=std::pow(10.f,threshold/20);
            const double quiet=qqsc::StaticCompressionEngine::singleGainForLevel(.1f,ratio,thresholdLinear,std::numeric_limits<float>::infinity(),algorithm);
            const double high=qqsc::StaticCompressionEngine::singleGainForLevel(1,ratio,thresholdLinear,std::numeric_limits<float>::infinity(),algorithm);
            const auto stablePre=gainError(input,output[0],48000-1248,48000,quiet);
            const auto previewPre=gainError(input,output[1],48000-1248,48000,quiet);
            const auto previewPost=gainError(input,output[1],144000,144000+1248,quiet);
            double highPeak=0;
            for(int i=48000;i<144000;++i)highPeak=std::max(highPeak,std::abs(double(output[1][size_t(i)])));
            require(previewPre<.02,"Preview attenuated quiet plateau before step");
            require(previewPost<.02,"Preview attenuated quiet plateau after step");
            require(highPeak<=high*1.002,"Preview overshoots the high plateau ceiling");
            worstPre=std::max(worstPre,previewPre);worstPost=std::max(worstPost,previewPost);++cases;
            csv<<(algo?"Super":"Classic")<<','<<threshold<<','<<ratio<<','<<hz<<','<<phase<<','<<stablePre<<','<<previewPre<<','<<previewPost<<','<<20*std::log10(highPeak)<<'\n';
            if(algo==0&&setting==0&&hz==4341.1&&phase==0)
            {
                floats(destination.getChildFile("step-input.f32"),input);
                floats(destination.getChildFile("step-stable.f32"),output[0]);
                floats(destination.getChildFile("step-preview.f32"),output[1]);
                require(stablePre>10.,"Screenshot pre-dip not reproduced in Stable");
            }
        }
        csv.close();
        for(double rate:{44100.,48000.,96000.})
        {
            const auto input=stepInput(rate,4341.1,.37); std::vector<float> reference;
            for(int block:{17,256,1024})
            {
                configure(*plugins[1],0,0,0,-12,8,1);
                const auto output=renderPlugin(*plugins[1],input,rate,block);
                require(gainError(input,output,int(rate*.974),int(rate),1)<.02,"Sample-rate pre-dip");
                if(reference.empty())reference=output;
                else for(size_t i=0;i<output.size();++i)require(std::abs(output[i]-reference[i])<1e-7,"Block size changes audio");
            }
        }
        std::ofstream spectrum(destination.getChildFile("steady-thd.csv").getFullPathName().toStdString());
        spectrum<<"algorithm,hz,stable_h2_h7_dbc,preview_h2_h7_dbc\n";
        double worstTHD=-300;
        for(int algo:{0,1})for(double hz:{20.,30.,50.,100.,400.,997.,2000.,4000.,6000.})
        {
            std::vector<float> input(96000);for(size_t i=0;i<input.size();++i)input[i]=float(.8*std::sin(tau*hz*double(i)/48000));
            std::array<double,2> results;
            for(int j=0;j<2;++j)
            {configure(*plugins[j],algo,0,0,-40,12.4f,1);results[j]=thd(renderPlugin(*plugins[j],input,48000,256),48000,hz);}
            require(results[1]<-110,"Steady tone harmonic regression");worstTHD=std::max(worstTHD,results[1]);
            spectrum<<(algo?"Super":"Classic")<<','<<hz<<','<<results[0]<<','<<results[1]<<'\n';
        }
        std::cout<<"PASS: actual Stable and Preview VST3s coexist; parameter layout unchanged; "<<cases<<" step cases. Worst quiet pre/post error="<<worstPre<<"/"<<worstPost<<" dB. 44.1/48/96 kHz, blocks 17/256/1024 identical. Steady H2-H7 worst="<<worstTHD<<" dBc. Lookahead remains 26 ms.\n";
        for(auto& p:plugins)p.reset(); return 0;
    }
    catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
