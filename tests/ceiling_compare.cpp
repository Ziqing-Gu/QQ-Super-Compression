#include "OutputCeiling.h"
#include "PreviousOutputCeiling.h"
#include <iostream>
#include <fstream>
#include <complex>
#include <functional>
#include <stdexcept>

constexpr double fs=48000.,tau=6.2831853071795864769;
struct Audio { std::vector<float> y; int delay=0; };
template<class Guard> Audio render(bool tp,const std::function<float(double)>& signal,double seconds=4)
{
    Guard g;g.prepare(fs,true,tp,-1.0f);Audio a;a.delay=g.latencySamples();
    a.y.resize(size_t(fs*seconds));
    for(size_t i=0;i<a.y.size();++i)a.y[i]=g.process(signal(double(i)/fs),signal(double(i)/fs))[0];
    return a;
}
double magnitude(const Audio& a,double hz)
{
    std::complex<double> sum{};
    const int n=int(fs),start=int(2*fs)+a.delay;
    for(int i=0;i<n;++i)sum+=double(a.y[size_t(start+i)])*std::polar(1.,-tau*hz*i/fs);
    return std::abs(sum)*2/n;
}
double thd(const Audio& a,double frequency)
{
    double energy=0;
    for(int h=2;h<=7 && h*frequency<fs*.5;++h)energy+=std::pow(magnitude(a,h*frequency),2);
    return 10*std::log10(std::max(1.e-30,energy/std::pow(magnitude(a,frequency),2)));
}
double amResidual(const Audio& a,double modulation)
{
    // Residual outside the original carrier and first AM sideband pair.
    // This includes intentional envelope reshaping, not just harmonic THD.
    double energy=0;
    for(int i=0;i<int(fs);++i)energy+=std::pow(a.y[size_t(int(2*fs)+a.delay+i)],2)/fs;
    double wanted=0;
    for(double f:{1000.-modulation,1000.,1000.+modulation})wanted+=.5*std::pow(magnitude(a,f),2);
    return 10*std::log10(std::max(1.e-30,(energy-wanted)/energy));
}
double recoveryMs(const Audio& a,const std::function<float(double)>& signal,double eventEnd)
{
    // First complete 5 ms RMS window within 0.1 dB of untouched quiet audio.
    const int window=int(fs*.005);
    for(int offset=0;offset<int(fs*.5);++offset)
    {
        const int start=int(fs*eventEnd)+offset;double in=0,out=0;
        for(int j=0;j<window;++j)
        {in+=std::pow(signal(double(start+j)/fs),2);out+=std::pow(a.y[size_t(start+j+a.delay)],2);}
        if(out>=in*std::pow(10.,-.1/10.))return 1000.*offset/fs;
    }
    return 500;
}
int main(int argc,char** argv)
{
    std::cout<<std::unitbuf;
    const std::string root=argc>1?argv[1]:".";
    int failures=0;
    for(bool tp:{false,true})
    {
        for(double f:{20.,30.,50.,100.,200.,1000.,10000.})
        {
            auto signal=[=](double t){return float(4*std::sin(tau*f*t));};
            auto old=render<qqsc_previous::OutputCeiling>(tp,signal);
            auto now=render<qqsc::OutputCeiling>(tp,signal);
            const auto before=thd(old,f),after=thd(now,f);
            std::cout<<"THD tp="<<tp<<" Hz="<<f<<" old="<<before<<" new="<<after<<" dB\n";
            if(after>-85)++failures;
        }
        for(double modulation:{5.,40.,170.})
        {
            auto signal=[=](double t){return float(4*(.55+.45*std::sin(tau*modulation*t))*std::sin(tau*1000*t));};
            auto old=render<qqsc_previous::OutputCeiling>(tp,signal);
            auto now=render<qqsc::OutputCeiling>(tp,signal);
            std::cout<<"AM residual tp="<<tp<<" modulation="<<modulation<<" old="<<amResidual(old,modulation)<<" new="<<amResidual(now,modulation)<<" dB\n";
        }
        for(double f:{20.,1000.})
        {
            auto signal=[=](double t){return float((t>=1&&t<1.02?4.:.125)*std::sin(tau*f*t));};
            auto old=render<qqsc_previous::OutputCeiling>(tp,signal);
            auto now=render<qqsc::OutputCeiling>(tp,signal);
            auto before=recoveryMs(old,signal,1.02),after=recoveryMs(now,signal,1.02);
            std::cout<<"Recovery within 0.1dB tp="<<tp<<" Hz="<<f<<" old="<<before<<" new="<<after<<" ms\n";
            std::ofstream csv(root+"/burst-"+std::to_string(int(f))+"-tp"+std::to_string(int(tp))+".csv");
            csv<<"time,input,previous,candidate\n";
            for(int i=int(fs*.98);i<int(fs*1.4);++i)
                csv<<double(i)/fs<<','<<signal(double(i)/fs)<<','<<old.y[size_t(i+old.delay)]<<','<<now.y[size_t(i+now.delay)]<<'\n';
        }
    }
    if(failures){std::cerr<<"FAIL steady distortion: "<<failures<<'\n';return 1;}
    std::cout<<"PASS steady distortion comparison.\n";
}
