#include <set>
#include <memory>
#include <string>
#include <cmath>
#include <array>
#include <iostream>
#include <stdexcept>
#include "audioeffectx.h"
#define private public
#include "../third_party/airwindows/kStation/kStation.h"
#undef private
#include "../third_party/airwindows/kStation/kStation.cpp"
#include "../third_party/airwindows/kStation/kStationProc.cpp"
#include "../Source/kStationEngine.h"
void require(bool ok,const char* msg) { if(!ok) throw std::runtime_error(msg); }
template<class T> void scenario(double rate,int setting) {
    auto originalInstance=std::make_unique<kStation>(nullptr); auto& original=*originalInstance; original.setSampleRate(rate);
    original.fpdL=0x12345678u; original.fpdR=0x87654321u;
    auto engineInstance=std::make_unique<kStationEngine>(); auto& engine=*engineInstance; engine.reset(rate);
    for(int i=0;i<kNumParameters;++i) {
        const float v=setting==0?original.getParameter(i):setting==1?0.f:setting==2?1.f:static_cast<float>((i*3+setting)%11)*.1f;
        original.setParameter(i,v); engine.setParameter(i,v);
    }
    std::array<T,513> l{},r{},el{},er{};
    for(int block=0;block<160;++block) {
        const int count=std::array<int,5>{1,37,256,513,127}[static_cast<size_t>(block%5)];
        if constexpr(kNumParameters>0) { if(block==20) { original.setParameter(0,.63f); engine.setParameter(0,.63f); } }
        for(int i=0;i<count;++i) {
            l[static_cast<size_t>(i)]=el[static_cast<size_t>(i)]=block<3 || block>=100?T{}:static_cast<T>(.27*std::sin((block*513+i)*.07));
            r[static_cast<size_t>(i)]=er[static_cast<size_t>(i)]=block<3 || block>=100?T{}:static_cast<T>(.19*std::cos((block*513+i)*.11));
        }
        T* a[]{l.data(),r.data()}; T* b[]{el.data(),er.data()};
        if constexpr(std::is_same_v<T,float>) { engine.processReplacing(a,a,count); original.processReplacing(b,b,count); }
        else { engine.processDoubleReplacing(a,a,count); original.processDoubleReplacing(b,b,count); }
        for(int c=0;c<2;++c) for(int i=0;i<count;++i) {
            if(!std::isfinite(a[c][i])) { std::cerr<<"rate="<<rate<<" setting="<<setting<<" block="<<block<<'\n'; }
            require(std::isfinite(a[c][i]),"Non-finite engine output");
            require(a[c][i]==b[c][i],"Adapted DSP differs from upstream");
        }
    }
}
int main() try {
    int cases=0;
    for(double rate:{44100.,48000.,96000.,192000.}) for(int setting=0;setting<12;++setting) {
        scenario<float>(rate,setting); scenario<double>(rate,setting); cases+=2;
    }
    auto resetInstance=std::make_unique<kStationEngine>(); auto& e=*resetInstance; std::array<double,8> l{1},r{1}; double* p[]{l.data(),r.data()};
    e.processDoubleReplacing(p,p,8); const auto expected=l;
    e.reset(44100.); l={1}; r={1}; e.processDoubleReplacing(p,p,8);
    require(l==expected,"Reset is not deterministic");
    std::cout<<"kStation: "<<cases<<" exact upstream comparisons; variable blocks, silence, automation, extremes, reset passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
