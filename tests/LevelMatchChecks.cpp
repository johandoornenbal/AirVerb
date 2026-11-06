#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/VerbControls.h"
#include "../Source/kRockstar2Engine.h"
#include "../Source/kRockstarEngine.h"
#include "../Source/kCyberCityEngine.h"

#include "../Source/LoudnessMatch.h"
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* text) { if(!ok) throw std::runtime_error(text); }
juce::RangedAudioParameter& param(juce::AudioProcessor& p,const juce::String& id) {
    for(auto* v:p.getParameters()) if(auto* r=dynamic_cast<juce::RangedAudioParameter*>(v);r && r->paramID==id) return *r;
    throw std::runtime_error("Missing parameter "+id.toStdString());
}
juce::ValueTree snapshot(juce::AudioProcessor& p) {
    juce::MemoryBlock bytes; p.getStateInformation(bytes);
    auto xml=juce::AudioProcessor::getXmlFromBinary(bytes.getData(),static_cast<int>(bytes.getSize()));
    check(xml!=nullptr,"Invalid saved state"); return juce::ValueTree::fromXml(*xml);
}
void restore(juce::AudioProcessor& p,const juce::ValueTree& state) {
    auto xml=state.createXml(); juce::MemoryBlock bytes; juce::AudioProcessor::copyXmlToBinary(*xml,bytes);
    p.setStateInformation(bytes.getData(),static_cast<int>(bytes.getSize()));
}
juce::String key(int model,const char* property) { return controls::prefix(model)+property; }
struct UI {
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    explicit UI(juce::AudioProcessor& p):editor(p.createEditor()) {}
    juce::Component& component(const char* id) {
        for(auto* c:editor->getChildComponent(0)->getChildren()) if(c->getComponentID()==id) return *c;
        throw std::runtime_error("Missing UI component");
    }
    void click(const char* id) { dynamic_cast<juce::TextButton&>(component(id)).onClick(); }
    void select(int model) {
        for(auto* c:editor->getChildComponent(0)->getChildren()) if(auto* combo=dynamic_cast<juce::ComboBox*>(c);combo && combo->getName()=="Reverb model") combo->setSelectedId(model+1,juce::sendNotificationSync);
    }
    void refresh(int model) { select(model==0?1:0); select(model); }
};
void fill(juce::AudioBuffer<double>& b,int base) {
    for(int i=0;i<b.getNumSamples();++i) {
        const auto t=static_cast<double>(base+i)/48000.;
        b.setSample(0,i,.11*std::sin(t*2*3.141592653589793*997)+.03*std::sin(t*2*3.141592653589793*213));
        b.setSample(1,i,.07*std::sin(t*2*3.141592653589793*521));
    }
}
template<class Engine> void learn(juce::AudioProcessor& p,UI& ui,int model) {
    ui.select(model);
    param(p,controls::originalId(model)).setValueNotifyingHost(0);
    p.setProcessingPrecision(juce::AudioProcessor::doublePrecision);
    p.setRateAndBufferSizeDetails(48000,256); p.prepareToPlay(48000,256);
    Engine reference; reference.reset(48000);
    for(int i=0;i<controls::count(model);++i) reference.setParameter(i,param(p,controls::id(model,i)).getValue());
    LoudnessMatch expected; expected.prepare(48000); expected.start();
    juce::AudioBuffer<double> b(2,256),wet(2,256); juce::MidiBuffer midi;
    ui.click("learnMatch");
    int base=0;
    for(;base<48000*4;base+=256) {
        fill(b,base); wet.makeCopyOf(b);
        double* data[]{wet.getWritePointer(0),wet.getWritePointer(1)};
        wet.applyGain(juce::Decibels::decibelsToGain(static_cast<double>(param(p,controls::inputId(model)).convertFrom0to1(param(p,controls::inputId(model)).getValue()))));
        reference.processDoubleReplacing(data,data,256);
        wet.applyGain(juce::Decibels::decibelsToGain(static_cast<double>(param(p,controls::outputId(model)).convertFrom0to1(param(p,controls::outputId(model)).getValue()))));
        for(int i=0;i<256;++i) expected.sample(b.getSample(0,i),b.getSample(1,i),wet.getSample(0,i),wet.getSample(1,i));
        p.processBlock(b,midi);
        for(int c=0;c<2;++c) for(int i=0;i<256;++i) check(b.getSample(c,i)==wet.getSample(c,i),"Learning changes unmatched reverb output");
    }
    check(expected.finish()==LoudnessMatch::Result::success,"Reference passage not learnable");
    ui.click("learnMatch"); // Finish after four seconds, using the real audio-thread command.
    for(int block=0;block<8;++block,base+=256) {
        fill(b,base); wet.makeCopyOf(b);
        double* data[]{wet.getWritePointer(0),wet.getWritePointer(1)};
        wet.applyGain(juce::Decibels::decibelsToGain(static_cast<double>(param(p,controls::inputId(model)).convertFrom0to1(param(p,controls::inputId(model)).getValue()))));
        reference.processDoubleReplacing(data,data,256);
        wet.applyGain(juce::Decibels::decibelsToGain(static_cast<double>(param(p,controls::outputId(model)).convertFrom0to1(param(p,controls::outputId(model)).getValue()))));
        p.processBlock(b,midi);
        if(block==7) {
            auto saved=snapshot(p);
            const auto db=static_cast<float>(saved.getProperty(key(model,"matchDb")));
            check(static_cast<bool>(saved.getProperty(key(model,"hasMatch"))),"Learned match not saved");
            check(std::abs(db-expected.trimDb)<1.e-4,"Learned trim differs from original-engine measurement");
            check(std::abs(static_cast<double>(saved.getProperty(key(model,"inputLufs")))-expected.inputLufs)<1.e-4,"Measured input LUFS wrong");
            const auto gain=std::pow(10.,static_cast<double>(db)/20.);
            for(int c=0;c<2;++c) for(int i=0;i<256;++i) check(std::abs(b.getSample(c,i)-wet.getSample(c,i)*gain)<1.e-12,"Gain is not applied exactly once after the engine");
            std::cout<<controls::modelName(model)<<": learned "<<db<<" dB, expected "<<expected.trimDb<<'\n';
        }
    }
    ui.refresh(model);
    auto& label=dynamic_cast<juce::Label&>(ui.component("matchReadout"));
    check(label.getText().contains("dB") && !label.getText().contains("--"),"Learned readout missing");
    check(label.getFont().getHeight()<=14,"Applied gain readout too prominent");
    for(const char* id:{"learnMatch","resetMatch","matchEnabled","abOriginal","matchReadout","matchStatus"})
        check(ui.editor->getLocalBounds().contains(ui.component(id).getBounds()),"Matching controls outside editor");
    p.releaseResources();
}
template<class Engine> void verifyAppliedAfterSwitch(juce::AudioProcessor& p,int model) {
    param(p,"model").setValueNotifyingHost(param(p,"model").convertTo0to1(static_cast<float>(model)));
    Engine reference; reference.reset(48000);
    for(int i=0;i<controls::count(model);++i) reference.setParameter(i,param(p,controls::id(model,i)).getValue());
    juce::AudioBuffer<double> b(2,256),wet(2,256); juce::MidiBuffer midi;
    const auto saved=snapshot(p);
    const auto db=static_cast<double>(saved.getProperty(key(model,"matchDb")));
    const auto gain=param(p,controls::matchId(model)).getValue()>.5f?std::pow(10.,db/20.):1.;
    for(int block=0;block<8;++block) {
        fill(b,block*256); wet.makeCopyOf(b);
        double* data[]{wet.getWritePointer(0),wet.getWritePointer(1)};
        wet.applyGain(juce::Decibels::decibelsToGain(static_cast<double>(param(p,controls::inputId(model)).convertFrom0to1(param(p,controls::inputId(model)).getValue()))));
        reference.processDoubleReplacing(data,data,256);
        wet.applyGain(juce::Decibels::decibelsToGain(static_cast<double>(param(p,controls::outputId(model)).convertFrom0to1(param(p,controls::outputId(model)).getValue())))); p.processBlock(b,midi);
        if(block==7) for(int c=0;c<2;++c) for(int i=0;i<256;++i)
            check(std::abs(b.getSample(c,i)-wet.getSample(c,i)*gain)<1.e-12,"Model switch did not apply that model's learned adjustment/enable setting");
    }
    check(p.getLatencySamples()==controls::latency(model),"Model switch did not update host latency");
}
template<class T> void original(juce::AudioProcessor& p,int model) {
    param(p,"model").setValueNotifyingHost(param(p,"model").convertTo0to1(static_cast<float>(model)));
    param(p,controls::originalId(model)).setValueNotifyingHost(1);
    p.setProcessingPrecision(std::is_same_v<T,float>?juce::AudioProcessor::singlePrecision:juce::AudioProcessor::doublePrecision);
    p.prepareToPlay(48000,37);
    juce::AudioBuffer<T> b(2,37); juce::MidiBuffer midi;

    std::array<std::array<T,2>,2> history{};
    for(int block=0;block<8;++block) {
        const auto frames=block%3==0?1:block%3==1?2:37; b.setSize(2,frames);
        for(int c=0;c<2;++c) for(int i=0;i<frames;++i) b.setSample(c,i,static_cast<T>(.15*std::sin((block*37+i)*.07*(c+1))));
        juce::AudioBuffer<T> dry; dry.makeCopyOf(b);
        p.processBlock(b,midi);
        const auto delay=controls::latency(model);
        for(int i=0;i<frames;++i) {
            for(int c=0;c<2;++c) check(b.getSample(c,i)==(delay?history[static_cast<size_t>(delay-1)][static_cast<size_t>(c)]:dry.getSample(c,i)),"Original A/B delay is wrong");
            history[1]=history[0]; history[0]={dry.getSample(0,i),dry.getSample(1,i)};
        }
    }
    p.releaseResources();
}
int main() try {
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter()); UI ui(*p);
    param(*p,controls::id(0,4)).setValueNotifyingHost(.8f);
    param(*p,controls::id(1,3)).setValueNotifyingHost(.35f);
    auto& in=param(*p,controls::inputId(0)); auto& out=param(*p,controls::outputId(0));
    in.setValueNotifyingHost(in.convertTo0to1(3)); out.setValueNotifyingHost(out.convertTo0to1(-2));
    learn<kRockstar2Engine>(*p,ui,0); learn<kRockstarEngine>(*p,ui,1); learn<kCyberCityEngine>(*p,ui,2);
    in.setValueNotifyingHost(in.getDefaultValue()); out.setValueNotifyingHost(out.getDefaultValue());
    const auto learned=snapshot(*p);
    const double db0=learned.getProperty(key(0,"matchDb")),db1=learned.getProperty(key(1,"matchDb"));
    check(std::abs(db0-db1)>.1,"Model matches were shared");
    // Relearn with compensation already enabled: it must still measure before matching.
    ui.select(1); p->prepareToPlay(48000,256); ui.click("learnMatch");
    juce::AudioBuffer<double> b(2,256); juce::MidiBuffer midi;
    for(int base=0;base<48000*4;base+=256) { fill(b,base); p->processBlock(b,midi); }
    ui.click("learnMatch"); fill(b,0); p->processBlock(b,midi);
    check(std::abs(static_cast<double>(snapshot(*p).getProperty(key(1,"matchDb")))-db1)<1.e-4,"Relearning measured compensated output");
    p->releaseResources();
    param(*p,controls::matchId(1)).setValueNotifyingHost(0);
    ui.select(0); p->prepareToPlay(48000,256);
    verifyAppliedAfterSwitch<kRockstarEngine>(*p,1); verifyAppliedAfterSwitch<kRockstar2Engine>(*p,0); verifyAppliedAfterSwitch<kCyberCityEngine>(*p,2);

    p->releaseResources();
    for(int model=0;model<controls::modelCount;++model) { original<float>(*p,model); original<double>(*p,model); }
    for(int model=0;model<controls::modelCount;++model) {
        param(*p,controls::originalId(model)).setValueNotifyingHost(model%2?1.f:0.f);
        param(*p,controls::matchId(model)).setValueNotifyingHost(model%2?0.f:1.f);
    }
    for(int model:{0,1,2,0,1}) {
        ui.select(model);
        check(dynamic_cast<juce::TextButton&>(ui.component("matchEnabled")).getToggleState()==(model%2==0),"Model match enable not recalled");
        check(dynamic_cast<juce::TextButton&>(ui.component("abOriginal")).getToggleState()==(model%2==1),"Model A/B not recalled");
    }
    juce::MemoryBlock bytes; p->getStateInformation(bytes);
    std::unique_ptr<juce::AudioProcessor> restored(createPluginFilter());
    restored->setStateInformation(bytes.getData(),static_cast<int>(bytes.getSize()));
    for(int model=0;model<controls::modelCount;++model) {
        const auto recalled=snapshot(*restored);
        check(recalled.getProperty(key(model,"matchDb"))==learned.getProperty(key(model,"matchDb")),"Session lost a learned trim");
        check(static_cast<bool>(recalled.getProperty(key(model,"hasMatch"))),"Session lost learned flag");
        check(param(*restored,controls::originalId(model)).getValue()==param(*p,controls::originalId(model)).getValue(),"Session lost A/B");
        check(param(*restored,controls::matchId(model)).getValue()==param(*p,controls::matchId(model)).getValue(),"Session lost match enable");
    }
    // Sessions from the two-model release retain their controls and learned matches.
    auto old=learned.createCopy();
    for(int i=old.getNumChildren();--i>=0;)
        if(old.getChild(i).getProperty("id").toString().startsWith("kcyber_")) old.removeChild(i,nullptr);
    for(const char* property:{"matchDb","hasMatch","inputLufs","outputLufs"}) old.removeProperty(key(2,property),nullptr);
    old.getChildWithProperty("id","model").setProperty("value",1,nullptr);
    restore(*restored,old);
    check(param(*restored,"model").convertFrom0to1(param(*restored,"model").getValue())==1,"Old session lost model selection");
    for(int model:{0,1}) {
        for(int i=0;i<controls::count(model);++i) check(param(*restored,controls::id(model,i)).getValue()==param(*p,controls::id(model,i)).getValue(),"Old session lost controls");
        check(snapshot(*restored).getProperty(key(model,"matchDb"))==learned.getProperty(key(model,"matchDb")),"Old session lost learned match");
    }
    for(int i=0;i<controls::count(2);++i) check(std::abs(param(*restored,controls::id(2,i)).getValue()-controls::defaultValue(2,i))<1.e-6,"Old session lost kCyberCity defaults");
    check(!static_cast<bool>(snapshot(*restored).getProperty(key(2,"hasMatch"))),"Old session invented kCyberCity match");
    ui.select(1); const auto tone=param(*p,controls::id(1,2)).getValue();
    ui.click("resetMatch"); auto reset=snapshot(*p);
    check(!static_cast<bool>(reset.getProperty(key(1,"hasMatch"))) && static_cast<bool>(reset.getProperty(key(0,"hasMatch"))),"Reset did not isolate one bank");
    check(param(*p,controls::id(1,2)).getValue()==tone,"Reset match changed reverb");
    // Model switch cancels learning, including while host bypass is active.
    for(bool bypass:{false,true}) {
        ui.select(0); param(*p,controls::originalId(0)).setValueNotifyingHost(0);
        p->prepareToPlay(48000,256); ui.click("learnMatch");
        for(int n=0;n<220;++n) { fill(b,n*256); p->processBlock(b,midi); }
        ui.select(1); fill(b,0);
        if(bypass) p->processBlockBypassed(b,midi); else p->processBlock(b,midi);
        ui.refresh(1);
        check(dynamic_cast<juce::TextButton&>(ui.component("learnMatch")).getButtonText()=="Learn Match","Switch failed to cancel learning");
        check(snapshot(*p).getProperty(key(0,"matchDb"))==learned.getProperty(key(0,"matchDb")),"Canceled learn lost previous match");
        p->releaseResources();
    }
    ui.select(0); p->prepareToPlay(48000,256); ui.click("learnMatch");
    for(int n=0;n<220;++n) { fill(b,n*256); p->processBlock(b,midi); }
    for(int n=0;n<700;++n) { fill(b,n*256); p->processBlockBypassed(b,midi); }
    ui.click("learnMatch"); fill(b,0); p->processBlock(b,midi); ui.refresh(0);
    check(dynamic_cast<juce::Label&>(ui.component("matchStatus")).getText().contains("Not enough audio"),"Bypass counted toward learning");
    p->releaseResources();
    // Silent learn pauses indefinitely and preserves last successful match.
    p->prepareToPlay(48000,256); ui.click("learnMatch");
    for(int n=0;n<3000;++n) { b.clear(); p->processBlock(b,midi); }
    ui.click("learnMatch"); b.clear(); p->processBlock(b,midi);
    check(snapshot(*p).getProperty(key(0,"matchDb"))==learned.getProperty(key(0,"matchDb")),"Silent learn replaced match");
    p->releaseResources();
    // Automatic completion at fifteen seconds of accepted windows.
    ui.select(1); param(*p,controls::originalId(1)).setValueNotifyingHost(0);
    p->prepareToPlay(48000,256); ui.click("learnMatch");
    for(int n=0;n<3000;++n) { fill(b,n*256); p->processBlock(b,midi); }
    ui.refresh(1);
    check(static_cast<bool>(snapshot(*p).getProperty(key(1,"hasMatch"))),"Automatic learn failed");
    check(dynamic_cast<juce::TextButton&>(ui.component("learnMatch")).getButtonText()=="Learn Match","Automatic learn did not stop");
    p->releaseResources();
    std::cout<<"Per-model LUFS learning, fixed gain, dry A/B, session recall, reset, switch/bypass cancellation and silence passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
