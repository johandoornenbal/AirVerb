#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/VerbControls.h"
#include "../Source/kRockstar2Engine.h"
#include "../Source/kRockstarEngine.h"
#include "../Source/kCyberCityEngine.h"

#include "../Source/OutputMeter.h"
#include "../Source/MeterBallistics.h"
#include <iostream>
#include <stdexcept>
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
juce::RangedAudioParameter& parameter(juce::AudioProcessor& p,const juce::String& id) {
    for(auto* v:p.getParameters()) if(auto* r=dynamic_cast<juce::RangedAudioParameter*>(v);r && r->paramID==id) return *r;
    throw std::runtime_error("Missing parameter "+id.toStdString());
}
void select(juce::AudioProcessor& p,int model) { parameter(p,"model").setValueNotifyingHost(parameter(p,"model").convertTo0to1(static_cast<float>(model))); }
template<class Engine,class T> void compare(juce::AudioProcessor& p,int model,double rate) {
    select(p,model);
    p.setProcessingPrecision(std::is_same_v<T,float>?juce::AudioProcessor::singlePrecision:juce::AudioProcessor::doublePrecision);
    p.setRateAndBufferSizeDetails(rate,513); p.prepareToPlay(rate,513);
    Engine reference; reference.reset(rate);
    for(int i=0;i<controls::count(model);++i) {
        const auto value=static_cast<float>((i*3+4)%11)*.1f;
        parameter(p,controls::id(model,i)).setValueNotifyingHost(value); reference.setParameter(i,value);
    }
    juce::MidiBuffer midi;
    for(int block=0;block<20;++block) {
        const auto n=std::array<int,4>{1,37,257,513}[static_cast<size_t>(block%4)];
        juce::AudioBuffer<T> b(2,n),expected(2,n);
        for(int c=0;c<2;++c) for(int i=0;i<n;++i) {
            const auto value=static_cast<T>(.3*std::sin((block*513+i)*.05*(c+1)));
            b.setSample(c,i,value); expected.setSample(c,i,value);
        }
        p.processBlock(b,midi);
        T* ptr[]{expected.getWritePointer(0),expected.getWritePointer(1)};
        if constexpr(std::is_same_v<T,float>) reference.processReplacing(ptr,ptr,n);
        else reference.processDoubleReplacing(ptr,ptr,n);
        for(int c=0;c<2;++c) for(int i=0;i<n;++i)
            require(std::isfinite(b.getSample(c,i)) && b.getSample(c,i)==expected.getSample(c,i),"Wrapper altered engine output at unity trims");
    }
    require(p.getLatencySamples()==controls::latency(model),"Wrong host latency"); p.releaseResources();
}
struct HostObserver:juce::AudioProcessorParameter::Listener {
    int values=0,begins=0,ends=0;
    void parameterValueChanged(int,float) override { ++values; }
    void parameterGestureChanged(int,bool start) override { start?++begins:++ends; }
};
struct UI {
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    explicit UI(juce::AudioProcessor& p):editor(p.createEditor()) {}
    juce::Component& panel() { return *editor->getChildComponent(0); }
    juce::Component& component(const char* id) {
        for(auto* c:panel().getChildren()) if(c->getComponentID()==id) return *c;
        throw std::runtime_error("Missing UI component");
    }
    juce::Slider& slider(const juce::String& name) {
        for(auto* c:panel().getChildren()) if(auto* v=dynamic_cast<juce::Slider*>(c);v && v->getName()==name) return *v;
        throw std::runtime_error("Missing slider "+name.toStdString());
    }
    juce::ComboBox& selector() {
        for(auto* c:panel().getChildren()) if(auto* v=dynamic_cast<juce::ComboBox*>(c);v && v->getName()=="Reverb model") return *v;
        throw std::runtime_error("Missing selector");
    }
    juce::ToggleButton& link() {
        for(auto* c:panel().getChildren()) if(auto* v=dynamic_cast<juce::ToggleButton*>(c)) return *v;
        throw std::runtime_error("Missing link");
    }
    void selectModel(int model) { selector().setSelectedId(model+1,juce::sendNotificationSync); }
    void preview(int model,int size) {
        juce::Image image(juce::Image::ARGB,editor->getWidth(),editor->getHeight(),true);
        juce::Graphics g(image); editor->paintEntireComponent(g,true);
        auto name="ui-"+controls::modelName(model)+"-"+juce::String(size)+".png";
        auto out=juce::File::getCurrentWorkingDirectory().getChildFile(name).createOutputStream();
        require(out!=nullptr,"Cannot create preview"); out->setPosition(0); out->truncate();
        require(juce::PNGImageFormat().writeImageToStream(image,*out),"Preview rendering failed");
    }
};
bool close(double a,double b) { return std::abs(a-b)<1.e-5; }
void trimChecks() {
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter()); UI ui(*p);
    for(int model=0;model<controls::modelCount;++model) {
        ui.selectModel(model);
        auto& input=ui.slider("Input trim"); auto& output=ui.slider("Output trim");
        require(close(input.getValue(),0) && close(output.getValue(),0) && !ui.link().getToggleState(),"Trim/link defaults wrong");
        ui.link().setToggleState(true,juce::sendNotificationSync);
        HostObserver a,b;
        auto& in=parameter(*p,controls::inputId(model)); auto& out=parameter(*p,controls::outputId(model));
        in.addListener(&a); out.addListener(&b);
        input.setValue(6,juce::sendNotificationSync);
        in.removeListener(&a); out.removeListener(&b);
        require(close(output.getValue(),-6),"Link failed to compensate in dB");
        require(a.values && b.values && a.begins==1 && a.ends==1 && b.begins==1 && b.ends==1,"Host gestures unbalanced");
        output.setValue(22,juce::sendNotificationSync);
        input.setValue(-10,juce::sendNotificationSync); require(close(output.getValue(),24),"Link failed to clamp");
        input.setValue(6,juce::sendNotificationSync); require(close(output.getValue(),22),"Clamping lost reference");
        in.setValueNotifyingHost(in.convertTo0to1(3));
        require(close(output.getValue(),22),"Automation caused extra compensation");
        input.setValue(input.getValueFromText("5 dB"),juce::sendNotificationSync);
        require(close(output.getValue(),20),"Typed trim after automation used stale reference");
        ui.link().setToggleState(false,juce::sendNotificationSync);
        input.setValue(0,juce::sendNotificationSync); output.setValue(0,juce::sendNotificationSync);
        // A known output trim must be applied exactly once after the original reverb.
        out.setValueNotifyingHost(out.convertTo0to1(-6));
        p->setProcessingPrecision(juce::AudioProcessor::doublePrecision); p->prepareToPlay(48000,256);
        kRockstar2Engine pear; kRockstarEngine bez; kCyberCityEngine cyber; cyber.reset(48000);
        pear.reset(48000); bez.reset(48000);
        juce::AudioBuffer<double> audio(2,256),expected(2,256); juce::MidiBuffer midi;
        for(int i=0;i<256;++i) for(int c=0;c<2;++c) audio.setSample(c,i,.1*std::sin(i*.05*(c+1)));
        expected.makeCopyOf(audio); double* ptr[]{expected.getWritePointer(0),expected.getWritePointer(1)};
        switch(model) {
            case 0: pear.processDoubleReplacing(ptr,ptr,256); break;
            case 1: bez.processDoubleReplacing(ptr,ptr,256); break;
            case 2: cyber.processDoubleReplacing(ptr,ptr,256); break;
        }
        p->processBlock(audio,midi);
        for(int c=0;c<2;++c) for(int i=0;i<256;++i) require(std::abs(audio.getSample(c,i)-expected.getSample(c,i)*std::pow(10.,-6./20.))<1.e-12,"Output trim applied incorrectly");
        p->releaseResources(); out.setValueNotifyingHost(out.getDefaultValue());
    }
}
template<class T> void bypassChecks(juce::AudioProcessor& p,int model,double rate) {
    select(p,model); p.prepareToPlay(rate,37);
    std::array<std::array<T,2>,2> history{};
    juce::AudioBuffer<T> b(2,37); juce::MidiBuffer midi;
    for(int block=0;block<4;++block) {
        const auto frames=block%3==0?1:block%3==1?2:37; b.setSize(2,frames);
        for(int c=0;c<2;++c) for(int i=0;i<frames;++i) b.setSample(c,i,static_cast<T>((block*37+i+1)*.001*(c+1)));
        juce::AudioBuffer<T> dry; dry.makeCopyOf(b); p.processBlockBypassed(b,midi);
        const auto delay=controls::latency(model);
        for(int i=0;i<frames;++i) {
            for(int c=0;c<2;++c) require(b.getSample(c,i)==(delay?history[static_cast<size_t>(delay-1)][static_cast<size_t>(c)]:dry.getSample(c,i)),"Host bypass latency is wrong");
            history[1]=history[0]; history[0]={dry.getSample(0,i),dry.getSample(1,i)};
        }
    }
    p.releaseResources();
}
void meterChecks() {
    MeterBallistics meter;
    require(meter.advance(1.f,1./60)<-40,"Meter attack jumps");
    for(int i=0;i<90;++i) meter.advance(.5f,1./60);
    require(std::abs(meter.getLevelDb()+6.0206f)<.05f,"Meter calibration wrong");
    for(int i=0;i<360;++i) meter.advance(0,1./60);
    require(meter.getLevelDb()<=-60,"Meter never rests");
    MeterBallistics a,b;
    for(int i=0;i<60;++i) a.advance(.5f,1./60);
    for(int i=0;i<20;++i) { b.advance(.5f,.01); b.advance(.5f,.04); }
    require(std::abs(a.getLevelDb()-b.getLevelDb())<.001,"Meter depends on timer jitter");
}
int main() try {
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    require(parameter(*p,"model").getValue()==0,"kRockstar2 is not the default");
    require(p->getParameters().size()==28,"Unexpected parameter layout");
    juce::StringArray originalIds;
    for(int model:{0,1}) for(int i=0;i<controls::count(model);++i) originalIds.add(controls::id(model,i));
    originalIds.add("model");
    for(int model:{0,1}) originalIds.add(controls::linkId(model));
    for(int model:{0,1}) { originalIds.add(controls::matchId(model)); originalIds.add(controls::originalId(model)); }
    for(int model:{0,1}) { originalIds.add(controls::inputId(model)); originalIds.add(controls::outputId(model)); }
    require(originalIds.size()==20,"Old layout fixture incomplete");
    for(int i=0;i<originalIds.size();++i) require(dynamic_cast<juce::RangedAudioParameter*>(p->getParameters()[i])->paramID==originalIds[i],"Existing parameter index changed");
    // All native defaults and the public parameter layout are explicit.
    require(p->getName()=="AirVerb Studio","Wrong plugin identity");
    require(p->getTailLengthSeconds()>=30.,"Reverb tail not reported to the host");
    for(int model=0;model<controls::modelCount;++model)
        for(int i=0;i<controls::count(model);++i)
            require(close(parameter(*p,controls::id(model,i)).getValue(),controls::defaultValue(model,i)),"Native default changed");
    for(double rate:{44100.,48000.,96000.,192000.}) {
        for(int model=0;model<controls::modelCount;++model) { bypassChecks<float>(*p,model,rate); bypassChecks<double>(*p,model,rate); }
        compare<kRockstar2Engine,float>(*p,0,rate); compare<kRockstar2Engine,double>(*p,0,rate);
        compare<kRockstarEngine,float>(*p,1,rate); compare<kRockstarEngine,double>(*p,1,rate);
        compare<kCyberCityEngine,float>(*p,2,rate); compare<kCyberCityEngine,double>(*p,2,rate);

    }
    meterChecks(); trimChecks();
    UI ui(*p);
    for(int model:{0,1,2,0}) {
        ui.selectModel(model);
        require(close(parameter(*p,"model").convertFrom0to1(parameter(*p,"model").getValue()),model),"Selector not bound");
        int visible=0;
        for(auto* child:ui.panel().getChildren()) if(auto* slider=dynamic_cast<juce::Slider*>(child);slider && slider->isVisible()) ++visible;
        require(visible==controls::count(model)+2,"Wrong visible parameter bank");
        for(int i=0;i<controls::count(model);++i) {
            auto& slider=ui.slider(controls::name(model,i));
            slider.setValue(.31,juce::sendNotificationSync);
            require(close(parameter(*p,controls::id(model,i)).getValue(),.31),"Knob not bound to selected bank");
            require(close(slider.getValueFromText("75 %"),.75),"Typed value mapping wrong");
            require(slider.isDoubleClickReturnEnabled() && close(slider.getDoubleClickReturnValue(),controls::defaultValue(model,i)),"Double click default wrong");
        }
        for(auto* child:ui.panel().getChildren()) if(child->isVisible()) require(ui.panel().getLocalBounds().contains(child->getBounds()),"Control outside editor");
        // Render neutral settings for documentation after exercising the bindings.
        for(int i=0;i<controls::count(model);++i) ui.slider(controls::name(model,i)).setValue(controls::defaultValue(model,i),juce::sendNotificationSync);
        auto& sizes=dynamic_cast<juce::ComboBox&>(ui.component("uiSize"));
        for(int size:{1,2,3}) {
            sizes.setSelectedId(size,juce::sendNotificationSync);
            const auto scale=size==1?.75:size==3?1.25:1.;
            require(ui.editor->getWidth()==juce::roundToInt(980*scale) && ui.editor->getHeight()==juce::roundToInt(830*scale),"Size choice failed");
            ui.preview(model,size);
        }
        for(int i=0;i<controls::count(model);++i) ui.slider(controls::name(model,i)).setValue(.31,juce::sendNotificationSync);
    }
    // Reset only the selected reverb, including trims, while preserving the other bank.
    ui.selectModel(1); ui.slider("Input trim").setValue(3,juce::sendNotificationSync);
    dynamic_cast<juce::TextButton&>(ui.component("resetParameters")).onClick();
    for(int i=0;i<controls::count(1);++i) require(close(parameter(*p,controls::id(1,i)).getValue(),controls::defaultValue(1,i)),"Reset missed reverb parameter");
    require(close(parameter(*p,controls::inputId(1)).convertFrom0to1(parameter(*p,controls::inputId(1)).getValue()),0),"Reset missed trim");
    require(close(parameter(*p,controls::id(0,0)).getValue(),.31),"Reset changed other bank");
    for(int model=0;model<controls::modelCount;++model) {
        ui.selectModel(model);
        for(int i=0;i<controls::count(model);++i) ui.slider(controls::name(model,i)).setValue(.12,juce::sendNotificationSync);
        dynamic_cast<juce::TextButton&>(ui.component("resetParameters")).onClick();
        for(int i=0;i<controls::count(model);++i) require(close(parameter(*p,controls::id(model,i)).getValue(),controls::defaultValue(model,i)),"New model reset missed original default");
    }
    ui.selectModel(1);
    juce::MemoryBlock bytes; p->getStateInformation(bytes);
    std::unique_ptr<juce::AudioProcessor> restored(createPluginFilter()); restored->setStateInformation(bytes.getData(),static_cast<int>(bytes.getSize()));
    for(auto* raw:p->getParameters()) if(auto* v=dynamic_cast<juce::RangedAudioParameter*>(raw)) require(close(parameter(*restored,v->paramID).getValue(),v->getValue()),"Session lost parameter");
    UI recalled(*restored); require(recalled.editor->getWidth()==1225,"Session lost UI size");
    const char invalid[]{'b','a','d'}; restored->setStateInformation(invalid,3);
    require(close(parameter(*restored,"model").convertFrom0to1(parameter(*restored,"model").getValue()),1),"Invalid state damaged session");
    std::cout<<"Default model, float/double routing, per-model latency, UI banks/sizes, trims/link gestures, meters, reset and session recall passed\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
