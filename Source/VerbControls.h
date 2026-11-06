#pragma once
#include <array>
namespace controls {
constexpr int modelCount=2;
constexpr int maxParameters=5;
constexpr std::array<const char*,modelCount> models{"kRockstar2","kRockstar"};
inline juce::String modelName(int model) { return models[static_cast<size_t>(model)]; }
inline int count(int model) { return model==0?5:4; }
inline juce::String prefix(int model) { return model==0?"krock2_":"krock_"; }
inline juce::String id(int model,int index) { return prefix(model)+juce::String(index); }
inline juce::String matchId(int model) { return prefix(model)+"matchEnabled"; }
inline juce::String originalId(int model) { return prefix(model)+"original"; }
inline juce::String inputId(int model) { return prefix(model)+"inputTrim"; }
inline juce::String outputId(int model) { return prefix(model)+"outputTrim"; }
inline juce::String linkId(int model) { return prefix(model)+"linkIO"; }
inline int primaryCount(int model) { return count(model); }
inline int primaryIndex(int,int column) { return column; }
inline float defaultValue(int model,int index) {
    constexpr std::array<std::array<float,5>,2> values{{{.5f,.75f,1.f,.25f,.5f},{.5f,1.f,1.f,.5f,0.f}}};
    return values[static_cast<size_t>(model)][static_cast<size_t>(index)];
}
inline int latency(int) { return 0; }
inline juce::StringArray choices() { juce::StringArray result; for(auto* name:models) result.add(name); return result; }
inline const char* name(int model,int index) {
    constexpr std::array<const char*,5> newer{"Regen","Position","DownRez","Distance","Dry/Wet"};
    constexpr std::array<const char*,4> older{"Regen","Position","DownRez","Dry/Wet"};
    return model==0?newer[static_cast<size_t>(index)]:older[static_cast<size_t>(index)];
}
inline const char* description(int model) { return model==0?"KROCKSTAR2  /  AIRWINDOWS REVERB":"KROCKSTAR  /  AIRWINDOWS REVERB"; }
inline const char* defaultsText(int model) {
    return model==0?"Defaults: Regen 50%  /  Position 75%  /  DownRez 100%  /  Distance 25%  /  Dry/Wet 50%":
        "Defaults: Regen 50%  /  Position 100%  /  DownRez 100%  /  Dry/Wet 50%";
}
inline juce::String tip(int model,int index) {
    if(index==0) return "Original reverb regeneration mapping. Default 50%; normalized control, not a decay time.";
    if(index==1) return "Original stepped early-reflection position mapping. Default "+juce::String(model==0?75:100)+"%.";
    if(index==2) return "Original internal downsampling mapping. Default 100%; normalized control, not Hz.";
    if(model==0 && index==3) return "Original distance processing. Default 25%.";
    return "Original dry/wet blend. Default 50%. Use 100% wet for a send effect; Original A/B compares untouched input.";
}
inline juce::String display(int,double v) { return juce::String(v*100.,1)+" %"; }
inline double parse(int,const juce::String& text) { return juce::jlimit(0.,1.,text.getDoubleValue()/100.); }
}
