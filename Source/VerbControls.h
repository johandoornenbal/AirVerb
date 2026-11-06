#pragma once
#include <array>
namespace controls {
constexpr int modelCount=13;
constexpr int maxParameters=6;
constexpr std::array<const char*,modelCount> models{"kRockstar2","kRockstar","kCyberCity","VerbThic","kWoodRoom","MV2","VerbSevens","kStation","kGuitarHall2","kCathedral5","kBeyond","kPlate140","kPlate240"};
inline juce::String modelName(int model) { return models[static_cast<size_t>(model)]; }
inline int count(int model) { constexpr std::array<int,modelCount> counts{5,4,3,5,6,5,0,6,6,6,6,5,5}; return counts[static_cast<size_t>(model)]; }
inline juce::String prefix(int model) { constexpr std::array<const char*,modelCount> prefixes{"krock2_","krock_","kcyber_","thic_","wood_","mv2_","sevens_","station_","guitar2_","cathedral5_","beyond_","plate140_","plate240_"}; return prefixes[static_cast<size_t>(model)]; }
inline juce::String id(int model,int index) { return prefix(model)+juce::String(index); }
inline juce::String matchId(int model) { return prefix(model)+"matchEnabled"; }
inline juce::String originalId(int model) { return prefix(model)+"original"; }
inline juce::String inputId(int model) { return prefix(model)+"inputTrim"; }
inline juce::String outputId(int model) { return prefix(model)+"outputTrim"; }
inline juce::String linkId(int model) { return prefix(model)+"linkIO"; }
inline int primaryCount(int model) { return count(model); }
inline int primaryIndex(int,int column) { return column; }
inline float defaultValue(int model,int index) {
    constexpr std::array<std::array<float,maxParameters>,modelCount> values{{{.5f,.75f,1.f,.25f,.5f},{.5f,1.f,1.f,.5f},{.5f,1.f,.5f},{.5f,1.f,1.f,0.f,1.f},{.5f,.5f,.25f,.5f,.75f,.5f},{.5f,.5f,.5f,1.f,1.f},{},{.5f,.5f,.5f,.5f,.9f,.5f},{.5f,.5f,.5f,.5f,.5f,.5f},{.5f,.5f,.24f,.5f,.5f,.5f},{.5f,1.f,.5f,.5f,0.f,.5f},{1.f,.5f,1.f,0.f,.25f},{1.f,.5f,1.f,0.f,.25f}}};
    return values[static_cast<size_t>(model)][static_cast<size_t>(index)];
}
inline int latency(int) { return 0; }
inline juce::StringArray choices() { juce::StringArray result; for(auto* name:models) result.add(name); return result; }
inline const char* name(int model,int index) {
    if(model==7 || model==8 || model==9) return name(4,index);
    if(model==10) { constexpr std::array<const char*,6> names{"Regen","Derez","Filter","Early RF","Pre-delay","Dry/Wet"}; return names[static_cast<size_t>(index)]; }
    if(model==11 || model==12) { constexpr std::array<const char*,5> names{"Input","Regen","DeRez","Pre-delay","Wetness"}; return names[static_cast<size_t>(index)]; }
    if(model>=3) {
        constexpr std::array<std::array<const char*,maxParameters>,4> names{{
            {"Replace","Derez","Filter","Wider","Dry/Wet"},
            {"Regen","Derez","Filter","Early RF","Position","Dry/Wet"},
            {"Depth","Bright","Regen","Output","Dry/Wet"},{}}};
        return names[static_cast<size_t>(model-3)][static_cast<size_t>(index)];
    }
    constexpr std::array<const char*,5> newer{"Regen","Position","DownRez","Distance","Dry/Wet"};
    constexpr std::array<const char*,4> older{"Regen","Position","DownRez","Dry/Wet"};
    if(model==2) { constexpr std::array<const char*,3> cyber{"Regen","Position","Dry/Wet"}; return cyber[static_cast<size_t>(index)]; }
    return model==0?newer[static_cast<size_t>(index)]:older[static_cast<size_t>(index)];
}
inline juce::String description(int model) { return modelName(model).toUpperCase()+"  /  AIRWINDOWS REVERB"; }
inline juce::String defaultsText(int model) {
    if(count(model)==0) return "Original fixed reverb  /  No native controls  /  Use wrapper trims and LUFS matching";
    juce::String text="Defaults: ";
    for(int i=0;i<count(model);++i) { if(i>0) text+="  /  "; text+=juce::String(name(model,i))+" "+juce::String(defaultValue(model,i)*100.f,0)+"%"; }
    return text;
}
inline juce::String tip(int model,int index) {
    if(model>=11 && index==0) return "Original plate input pad. Default 100%. Independent of wrapper Input trim.";
    if(model>=11 && index==4) return "Original Wetness mapping: 0% dry only, 50% full dry plus full wet, 100% wet only. Default 25%.";
    if(model>=10 && juce::String(name(model,index))=="Pre-delay") return "Original normalized pre-delay mapping. Default 0%; not a millisecond readout.";
    if(model>=3) {
        auto text="Original Airwindows "+juce::String(name(model,index))+" control. Default "+juce::String(defaultValue(model,index)*100.f,0)+"%.";
        if(juce::String(name(model,index))=="Dry/Wet") text+=" Original dry/wet mapping; use 100% wet for a send effect.";
        else if(model==5 && index==3) text+=" Native output gain, separate from wrapper Output trim.";
        else text+=" Uses the native normalized mapping.";
        return text;
    }
    if(index==0) return "Original reverb regeneration mapping. Default 50%; normalized control, not a decay time.";
    if(index==1) return "Original stepped early-reflection position mapping. Default "+juce::String(model==0?75:100)+"%.";
    if(model!=2 && index==2) return "Original internal downsampling mapping. Default 100%; normalized control, not Hz.";
    if(model==0 && index==3) return "Original distance processing. Default 25%.";
    return "Original dry/wet blend. Default 50%. Use 100% wet for a send effect; Original A/B compares untouched input.";
}
inline juce::String display(int,double v) { return juce::String(v*100.,1)+" %"; }
inline double parse(int,const juce::String& text) { return juce::jlimit(0.,1.,text.getDoubleValue()/100.); }
}
