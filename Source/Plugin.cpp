#include <JuceHeader.h>
#include <atomic>
#include <limits>
#include "kRockstar2Engine.h"
#include "kRockstarEngine.h"

#include "VerbControls.h"

#include "OutputMeter.h"
#include "MeterBallistics.h"
#include "GainLinkAttachment.h"
#include "VerbLevelMatch.h"

class VerbProcessor final : public juce::AudioProcessor, private juce::Timer {
public:
    VerbProcessor() : AudioProcessor(BusesProperties()
        .withInput("Input",juce::AudioChannelSet::stereo(),true)
        .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
        state(*this,nullptr,"AirVerbState",layout()) {
        for(int model=0;model<controls::modelCount;++model)
            for(int i=0;i<controls::count(model);++i) values[static_cast<size_t>(model)][static_cast<size_t>(i)]=state.getRawParameterValue(controls::id(model,i));
        modelValue=state.getRawParameterValue("model");
        defaults=state.copyState();
        for(int model=0;model<controls::modelCount;++model) {
            matchEnabled[static_cast<size_t>(model)]=state.getRawParameterValue(controls::matchId(model));
            original[static_cast<size_t>(model)]=state.getRawParameterValue(controls::originalId(model));
            inputTrim[static_cast<size_t>(model)]=state.getRawParameterValue(controls::inputId(model));
            outputTrim[static_cast<size_t>(model)]=state.getRawParameterValue(controls::outputId(model));
        }
        startTimerHz(10);
    }
    ~VerbProcessor() override { stopTimer(); }
    static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
        juce::AudioProcessorValueTreeState::ParameterLayout p;
        // Stable parameter IDs and indices for each complete model bank.
        auto addBank=[&p](int model) {
            for(int i=0;i<controls::count(model);++i) {
                auto attributes=juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction([i](float v,int) { return controls::display(i,v); })
                    .withValueFromStringFunction([i](const juce::String& text) { return static_cast<float>(controls::parse(i,text)); });
                p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{controls::id(model,i),1},
                    controls::modelName(model)+" "+controls::name(model,i),juce::NormalisableRange<float>(0,1),controls::defaultValue(model,i),attributes));
            }
        };
        auto addLink=[&p](int model) {
            p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{controls::linkId(model),1},controls::modelName(model)+" Link Input / Output",false));
        };
        auto addComparison=[&p](int model) {
            p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{controls::matchId(model),1},controls::modelName(model)+" Level Match",true));
            p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{controls::originalId(model),1},controls::modelName(model)+" Original A/B",false));
        };
        auto addTrims=[&p](int model) {
            for(bool input:{true,false}) {
                auto attributes=juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v,int) { return juce::String(v,1)+" dB"; })
                    .withValueFromStringFunction([](const juce::String& text) { return text.getFloatValue(); });
                p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{input?controls::inputId(model):controls::outputId(model),1},
                    controls::modelName(model)+(input?" Input trim":" Output trim"),juce::NormalisableRange<float>(-24,24),0,attributes));
            }
        };
        addBank(0); addBank(1);
        p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"model",1},"Reverb model",controls::choices(),0));
        for(int model:{0,1}) addLink(model);
        for(int model:{0,1}) addComparison(model);
        for(int model:{0,1}) addTrims(model);
        return p;
    }
    int selectedModel() const { return juce::jlimit(0,controls::modelCount-1,juce::roundToInt(modelValue->load())); }
    void prepareToPlay(double rate,int maximumBlockSize) override {
        sampleRate=rate; engine.reset(rate); engine1.reset(rate);
        dryHistory={};
        activeModel=selectedModel(); setLatencySamples(controls::latency(activeModel)); peaks[0]=0; peaks[1]=0;
        meterTimeoutSeconds.store(juce::jmax(.15,2.0*maximumBlockSize/rate),std::memory_order_relaxed);
        meterSequence.fetch_add(1,std::memory_order_release);
        inputSmooth.reset(rate,.02); outputSmooth.reset(rate,.02);
        inputSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(static_cast<double>(inputTrim[static_cast<size_t>(activeModel)]->load())));
        outputSmooth.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(static_cast<double>(outputTrim[static_cast<size_t>(activeModel)]->load())));
        loudness.prepare(rate); learning=false; progress=0; command=0;
        gainSmooth.reset(rate,.02); blendSmooth.reset(rate,.005);
        gainSmooth.setCurrentAndTargetValue(targetGain(activeModel));
        blendSmooth.setCurrentAndTargetValue(original[static_cast<size_t>(activeModel)]->load()>.5f?0.:1.);
    }
    template<class Engine,class T> void render(Engine& e,juce::AudioBuffer<T>& b,int model) {
        for(int i=0;i<controls::count(model);++i) e.setParameter(i,values[static_cast<size_t>(model)][static_cast<size_t>(i)]->load());
        // Fixed stack storage preserves dry input without allocating on the audio thread.
        // Measure the original input and unmatched reverb output, then apply a separate gain.
        std::array<T,256> dryL{},dryR{};
        for(int offset=0;offset<b.getNumSamples();offset+=256) {
            const int count=juce::jmin(256,b.getNumSamples()-offset);
            auto* left=b.getWritePointer(0)+offset; auto* right=b.getWritePointer(1)+offset;
            std::copy_n(left,count,dryL.data()); std::copy_n(right,count,dryR.data());
            inputSmooth.setTargetValue(juce::Decibels::decibelsToGain(static_cast<double>(inputTrim[static_cast<size_t>(model)]->load())));
            outputSmooth.setTargetValue(juce::Decibels::decibelsToGain(static_cast<double>(outputTrim[static_cast<size_t>(model)]->load())));
            for(int i=0;i<count;++i) {
                const auto gain=inputSmooth.getNextValue();
                if(gain<1. || gain>1.) { left[i]=static_cast<T>(left[i]*gain); right[i]=static_cast<T>(right[i]*gain); }
            }
            T* data[]{left,right};
            if constexpr (std::is_same_v<T,float>) e.processReplacing(data,data,count);
            else e.processDoubleReplacing(data,data,count);
            gainSmooth.setTargetValue(targetGain(model));
            blendSmooth.setTargetValue(original[static_cast<size_t>(model)]->load()>.5f?0.:1.);
            for(int i=0;i<count;++i) {
                const auto dryIndex=static_cast<size_t>(i);
                const auto outputGain=outputSmooth.getNextValue();
                if(outputGain<1. || outputGain>1.) { left[i]=static_cast<T>(left[i]*outputGain); right[i]=static_cast<T>(right[i]*outputGain); }
                if(loudness.learning()) {
                    loudness.sample(dryL[dryIndex],dryR[dryIndex],left[i],right[i]);
                    if(loudness.complete()) { finishLearn(); gainSmooth.setTargetValue(targetGain(model)); }
                }
                // Align dry A/B to the original fixed delay, leaving the wet DSP untouched.
                const auto aligned=alignedDry(dryL[dryIndex],dryR[dryIndex],controls::latency(model));
                const auto alignedL=aligned[0],alignedR=aligned[1];
                const auto gain=gainSmooth.getNextValue(),blend=blendSmooth.getNextValue();
                if(blend<=0) { left[i]=static_cast<T>(alignedL); right[i]=static_cast<T>(alignedR); }
                else if(blend>=1) {
                    if(gain<1 || gain>1) { left[i]=static_cast<T>(left[i]*gain); right[i]=static_cast<T>(right[i]*gain); }
                } else {
                    left[i]=static_cast<T>(alignedL*(1-blend)+left[i]*gain*blend);
                    right[i]=static_cast<T>(alignedR*(1-blend)+right[i]*gain*blend);
                }
            }
        }
        progress=loudness.seconds();
    }
    void releaseResources() override { peaks[0]=0; peaks[1]=0; meterSequence.fetch_add(1,std::memory_order_release); }
    bool isBusesLayoutSupported(const BusesLayout& b) const override {
        return b.getMainInputChannelSet()==juce::AudioChannelSet::stereo()
            && b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
    }
    template<class T> void process(juce::AudioBuffer<T>& b) {
        juce::ScopedNoDenormals guard;
        if(b.getNumChannels()<2) { b.clear(); return; }
        const int model=selectedModel();
        syncModel(model);
        handleCommand(model);
        switch(model) {
            case 0: render(engine,b,model); break;
            case 1: render(engine1,b,model); break;
            default: break;
        }
        publishMeters(b);
    }
    template<class T> void publishMeters(juce::AudioBuffer<T>& b) {
        for(int c=0;c<2;++c) {
            const auto peak=static_cast<float>(b.getMagnitude(c,0,b.getNumSamples()));
            auto old=peaks[c].load(std::memory_order_relaxed);
            while(old<peak && !peaks[c].compare_exchange_weak(old,peak,std::memory_order_relaxed)) {}
        }
        meterSequence.fetch_add(1,std::memory_order_release);
    }
    void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) override { process(b); }
    void processBlock(juce::AudioBuffer<double>& b,juce::MidiBuffer&) override { process(b); }
    template<class T> void bypass(juce::AudioBuffer<T>& b) {
        syncModel(selectedModel());
        handleCommand(selectedModel());
        if(b.getNumChannels()>=2 && b.getNumSamples()>0) {
            // Host bypass must retain the same fixed latency as the active engine.
            const auto delay=controls::latency(selectedModel());
            for(int i=0;i<b.getNumSamples();++i) {
                const auto dry=alignedDry(b.getSample(0,i),b.getSample(1,i),delay);
                b.setSample(0,i,static_cast<T>(dry[0])); b.setSample(1,i,static_cast<T>(dry[1]));
            }
            publishMeters(b);
        }
    }
    void processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&) override { bypass(b); }
    void processBlockBypassed(juce::AudioBuffer<double>& b,juce::MidiBuffer&) override { bypass(b); }
    bool supportsDoublePrecisionProcessing() const override { return true; }
    const juce::String getName() const override { return "AirVerb Studio"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    // Native room-tone generation and regeneration have no fixed silence cutoff.
    double getTailLengthSeconds() const override { return std::numeric_limits<double>::infinity(); }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    void getStateInformation(juce::MemoryBlock& dst) override {
        auto saved=state.copyState();
        saved.setProperty("uiSize",uiSize.load(),nullptr);
        for(int model=0;model<controls::modelCount;++model) {
            const auto prefix=controls::prefix(model);
            const auto& match=matches[static_cast<size_t>(model)];
            saved.setProperty(prefix+"matchDb",match.db.load(),nullptr);
            saved.setProperty(prefix+"hasMatch",match.learned.load(),nullptr);
            saved.setProperty(prefix+"inputLufs",match.inputLufs.load(),nullptr);
            saved.setProperty(prefix+"outputLufs",match.outputLufs.load(),nullptr);
        }
        if(auto xml=saved.createXml()) copyXmlToBinary(*xml,dst);
    }
    void setStateInformation(const void* data,int size) override {
        if(auto xml=getXmlFromBinary(data,size); xml && xml->hasTagName(state.state.getType()))
        {
            auto restored=juce::ValueTree::fromXml(*xml);
            uiSize=juce::jlimit(1,3,static_cast<int>(restored.getProperty("uiSize",2)));
            // Supply defaults for missing parameters when reading older states.
            // Keep existing session values intact.
            for(const auto& child:defaults) {
                const auto id=child.getProperty("id");
                if(!restored.getChildWithProperty("id",id).isValid()) restored.addChild(child.createCopy(),-1,nullptr);
            }
            for(int model=0;model<controls::modelCount;++model) {
                const auto prefix=controls::prefix(model);
                auto& match=matches[static_cast<size_t>(model)];
                const auto db=static_cast<float>(restored.getProperty(prefix+"matchDb",0.f));
                match.db=std::isfinite(db)?juce::jlimit(-24.f,24.f,db):0.f;
                match.learned=std::isfinite(db)&&static_cast<bool>(restored.getProperty(prefix+"hasMatch",false));
                const auto in=static_cast<float>(restored.getProperty(prefix+"inputLufs",0.f));
                const auto out=static_cast<float>(restored.getProperty(prefix+"outputLufs",0.f));
                match.inputLufs=std::isfinite(in)?in:0.f; match.outputLufs=std::isfinite(out)?out:0.f;
            }
            command=4; learning=false; progress=0; learnResult=0;
            state.replaceState(restored);
        }
    }
    double targetGain(int model) const {
        const auto index=static_cast<size_t>(model);
        return matches[index].learned.load() && matchEnabled[index]->load()>.5f
            ? std::pow(10.,static_cast<double>(matches[index].db.load())/20.) : 1.;
    }
    void requestLearn() {
        const auto model=selectedModel();
        command=model*8+(learning.load() && learningModel.load()==model?2:1);
    }
    void resetMatch() {
        const auto model=selectedModel();
        matches[static_cast<size_t>(model)].clear();
        learning=false; progress=0; learnResult=0; command=model*8+3; matchRevision.fetch_add(1);
    }
    std::array<VerbMatchState,controls::modelCount> matches;
    std::atomic<bool> learning{false};
    std::atomic<int> learningModel{0},learnResult{0};
    std::atomic<float> progress{0};
    std::atomic<int> command{0};
    std::atomic<uint32_t> matchRevision{0};
    std::atomic<int> uiSize{2};
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> peaks[2]{};
    std::atomic<uint32_t> meterSequence{0};
    std::atomic<double> meterTimeoutSeconds{.15};
private:
    void syncModel(int model) {
        if(model!=activeModel) {
            loudness.cancel(); learning=false; progress=0; learnResult=0;
            // Reset only the entering engine, on the audio thread, without allocation.
            switch(model) {
                case 0: engine.reset(sampleRate); break;
                case 1: engine1.reset(sampleRate); break;
                default: break;
            }
            setLatencySamples(controls::latency(model));
            activeModel=model;
        }
    }
    void finishLearn() {
        const auto result=loudness.finish();
        if(result==LoudnessMatch::Result::success || result==LoudnessMatch::Result::limited) {
            auto& match=matches[static_cast<size_t>(learningModel.load())];
            match.db=static_cast<float>(loudness.trimDb);
            match.inputLufs=static_cast<float>(loudness.inputLufs); match.outputLufs=static_cast<float>(loudness.outputLufs);
            match.learned=true; matchRevision.fetch_add(1);
        }
        learnResult=static_cast<int>(result); learning=false;
    }
    void handleCommand(int model) {
        const auto request=command.exchange(0);
        if(request==4) { loudness.cancel(); learning=false; return; }
        if(request==0) return;
        const auto requestedModel=request/8,action=request%8;
        if(action==3) {
            matches[static_cast<size_t>(requestedModel)].clear();
            if(learningModel.load()==requestedModel) { loudness.cancel(); learning=false; progress=0; learnResult=0; }
        } else if(requestedModel==model) {
            if(action==1) { loudness.start(); learningModel=requestedModel; learning=true; progress=0; learnResult=0; }
            if(action==2 && learning.load() && learningModel.load()==requestedModel) finishLearn();
        }
    }
    void timerCallback() override {
        const auto revision=matchRevision.load();
        if(revision!=notifiedRevision) {
            notifiedRevision=revision;
            updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
        }
    }
    LoudnessMatch loudness;
    juce::SmoothedValue<double> inputSmooth{1.},outputSmooth{1.};
    std::array<std::atomic<float>*,controls::modelCount> inputTrim{},outputTrim{};
    juce::SmoothedValue<double> gainSmooth{1.},blendSmooth{1.};
    std::array<std::atomic<float>*,controls::modelCount> matchEnabled{},original{};
    uint32_t notifiedRevision=0;
    kRockstar2Engine engine;
    kRockstarEngine engine1;
    // [0] is the previous input sample; [1] is two samples back.
    std::array<std::array<double,2>,2> dryHistory{};
    std::array<double,2> alignedDry(double left,double right,int delay) {
        const std::array<double,2> current{left,right};
        const auto result=delay>0?dryHistory[static_cast<size_t>(delay-1)]:current;
        dryHistory[1]=dryHistory[0]; dryHistory[0]=current;
        return result;
    }
    std::array<std::array<std::atomic<float>*,controls::maxParameters>,controls::modelCount> values{};
    std::atomic<float>* modelValue=nullptr;
    juce::ValueTree defaults;
    double sampleRate=44100;
    int activeModel=0;
};

namespace style {
const juce::Colour cream{0xfff4eddb}, ink{0xff252b29}, gold{0xffd7ac61}, muted{0xff858b80};
class Look final : public juce::LookAndFeel_V4 {
public:
    Look() {
        setColour(juce::Slider::textBoxTextColourId, cream);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::TextEditor::highlightColourId, gold.withAlpha(.4f));
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
        auto r=juce::Rectangle<float>(static_cast<float>(x),static_cast<float>(y),static_cast<float>(w),static_cast<float>(h)).reduced(16);
        auto radius=juce::jmin(r.getWidth(),r.getHeight())*.5f;
        auto cx=r.getCentreX(), cy=r.getCentreY();
        juce::Path track, active;
        track.addCentredArc(cx,cy,radius,radius,0,start,end,true);
        active.addCentredArc(cx,cy,radius,radius,0,start,start+pos*(end-start),true);
        g.setColour(cream.withAlpha(.12f)); g.strokePath(track,juce::PathStrokeType(3));
        g.setColour(gold); g.strokePath(active,juce::PathStrokeType(3));
        auto dial=juce::Rectangle<float>(radius*1.58f,radius*1.58f).withCentre({cx,cy});
        g.setColour(juce::Colours::black.withAlpha(.3f)); g.fillEllipse(dial.translated(0,5));
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff535a52),cx,cy-radius,ink,cx,cy+radius,false));
        g.fillEllipse(dial); g.setColour(cream.withAlpha(.15f)); g.drawEllipse(dial,1);
        const auto angle=start+pos*(end-start);
        g.setColour(gold);
        g.drawLine(cx+std::sin(angle)*radius*.48f,cy-std::cos(angle)*radius*.48f,
                   cx+std::sin(angle)*radius*.68f,cy-std::cos(angle)*radius*.68f,3);
    }
};
}

class VerbPanel final : public juce::Component, private juce::Timer {
public:
    explicit VerbPanel(VerbProcessor& p) : processor(p) {
        setLookAndFeel(&look);
        for(size_t i=0;i<knobs.size();++i) {
            auto& slider=knobs[i]; addAndMakeVisible(slider);
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,115,23);
        }
        for(auto* slider:{&inputSlider,&outputSlider}) {
            addAndMakeVisible(*slider); slider->setSliderStyle(juce::Slider::LinearHorizontal);
            slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,75,24);
            slider->setColour(juce::Slider::trackColourId,style::gold);
            slider->setColour(juce::Slider::thumbColourId,style::gold);
            slider->setDoubleClickReturnValue(true,0.);
            slider->textFromValueFunction=[](double v) { return juce::String(v,1)+" dB"; };
            slider->valueFromTextFunction=[](const juce::String& text) { return text.getDoubleValue(); };
        }
        inputSlider.setName("Input trim"); outputSlider.setName("Output trim");
        inputSlider.setTooltip("Wrapper input trim before the reverb, -24 to +24 dB. Double-click for unity.");
        outputSlider.setTooltip("Wrapper output trim after the reverb, before LUFS matching. Double-click for unity.");
        linkToggle.setButtonText("Link Input / Output");
        linkToggle.setName("Link Input / Output");
        linkToggle.setTooltip("Move Output by the opposite gain change when editing Input. Output edits set a new reference. Compensation stops at the output range limits.");
        linkToggle.setColour(juce::ToggleButton::textColourId,style::cream);
        linkToggle.setColour(juce::ToggleButton::tickColourId,style::gold);
        linkToggle.setColour(juce::ToggleButton::tickDisabledColourId,style::cream.withAlpha(.55f));
        addAndMakeVisible(linkToggle);
        for(auto* button:{&learnButton,&resetMatchButton,&enableMatchButton,&originalButton}) {
            addAndMakeVisible(*button);
            button->setColour(juce::TextButton::buttonColourId,style::ink.brighter(.15f));
            button->setColour(juce::TextButton::textColourOffId,style::cream);
        }
        learnButton.setComponentID("learnMatch"); resetMatchButton.setComponentID("resetMatch");
        enableMatchButton.setComponentID("matchEnabled"); originalButton.setComponentID("abOriginal");
        learnButton.onClick=[this] { processor.requestLearn(); refreshMatch(); };
        resetMatchButton.onClick=[this] { processor.resetMatch(); refreshMatch(); };
        enableMatchButton.setClickingTogglesState(true); originalButton.setClickingTogglesState(true);
        learnButton.setTooltip("Learn a fixed LUFS level match for the selected model from up to 15 seconds of audible audio. Finish after at least 3 seconds.");
        resetMatchButton.setTooltip("Clear only this model's learned adjustment. Reverb controls, trims, linking and A/B remain as set.");
        enableMatchButton.setTooltip("Enable or disable this model's saved level match.");
        originalButton.setTooltip("Compare untouched input with the reverb output, using a short crossfade. Reverb keeps running so its tail continues when returning to Processed.");
        addAndMakeVisible(matchReadout); addAndMakeVisible(matchStatus);
        matchReadout.setComponentID("matchReadout"); matchStatus.setComponentID("matchStatus");
        matchReadout.setColour(juce::Label::textColourId,style::gold);
        matchReadout.setFont(juce::FontOptions(13.f)); matchReadout.setJustificationType(juce::Justification::right);
        matchStatus.setColour(juce::Label::textColourId,style::cream.withAlpha(.7f));
        matchStatus.setFont(juce::FontOptions(11.f));
        selector.setName("Reverb model");
        selector.setTooltip("Choose an original Airwindows reverb or slew algorithm. Each model retains its own controls and loudness match.");
        selector.addItemList(controls::choices(),1);
        selector.setColour(juce::ComboBox::backgroundColourId,style::ink);
        selector.setColour(juce::ComboBox::textColourId,style::gold);
        selector.setColour(juce::ComboBox::outlineColourId,style::gold.withAlpha(.4f));
        addAndMakeVisible(selector);
        selectorAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"model",selector);
        selector.onChange=[this] { if(selector.getSelectedId()>0) bindModel(selector.getSelectedId()-1); };
        bindModel(processor.selectedModel());
        reset.setComponentID("resetParameters");
        reset.setButtonText("RESET TO DEFAULT"); reset.setTooltip("Reset only the selected model to the Airwindows defaults.");
        reset.onClick=[this] {
            const auto model=processor.selectedModel();
            for(int i=0;i<controls::count(model);++i) {
                auto* parameter=processor.state.getParameter(controls::id(model,i));
                parameter->beginChangeGesture(); parameter->setValueNotifyingHost(parameter->getDefaultValue()); parameter->endChangeGesture();
            }
            for(const auto& id:{controls::inputId(model),controls::outputId(model)}) {
                auto* parameter=processor.state.getParameter(id);
                parameter->beginChangeGesture(); parameter->setValueNotifyingHost(parameter->getDefaultValue()); parameter->endChangeGesture();
            }
        };
        addAndMakeVisible(reset);
        reset.setColour(juce::TextButton::buttonColourId,style::ink);
        reset.setColour(juce::TextButton::textColourOffId,style::gold);
        addAndMakeVisible(leftMeter); addAndMakeVisible(rightMeter);
        lastMeterSequence=processor.meterSequence.load(std::memory_order_acquire);
        for(int c=0;c<2;++c) {
            targets[static_cast<size_t>(c)]=processor.peaks[c].exchange(0,std::memory_order_relaxed);
            ballistics[static_cast<size_t>(c)].resetPeak(targets[static_cast<size_t>(c)]);
        }
        leftMeter.setLevelDb(ballistics[0].getLevelDb()); rightMeter.setLevelDb(ballistics[1].getLevelDb());
        lastMeterTime=juce::Time::getMillisecondCounterHiRes();
        addAndMakeVisible(sizeChoice);
        sizeChoice.setComponentID("uiSize"); sizeChoice.setName("Interface size");
        sizeChoice.setTooltip("Interface size: Small 75%, Medium 100%, or Large 125%.");
        sizeChoice.addItem("Small (75%)",1);
        sizeChoice.addItem("Medium (100%)",2);
        sizeChoice.addItem("Large (125%)",3);
        sizeChoice.setColour(juce::ComboBox::backgroundColourId,style::ink);
        sizeChoice.setColour(juce::ComboBox::textColourId,style::gold);
        sizeChoice.setColour(juce::ComboBox::outlineColourId,style::gold.withAlpha(.4f));
        sizeChoice.setSelectedId(processor.uiSize.load(),juce::dontSendNotification);
        sizeChoice.onChange=[this] {
            processor.uiSize=sizeChoice.getSelectedId();
            processor.updateHostDisplay(juce::AudioProcessor::ChangeDetails{}.withNonParameterStateChanged(true));
            if(sizeChanged) sizeChanged();
        };
        setSize(980,830); startTimerHz(60);
    }
    ~VerbPanel() override { stopTimer(); setLookAndFeel(nullptr); }
    std::function<void()> sizeChanged;
    void resized() override {
        sizeChoice.setBounds(750,10,180,26);
        const auto columns=controls::primaryCount(shownModel);
        const auto spacing=columns>4?884/columns:190;
        const auto width=columns>4?spacing-6:174;
        const auto start=(980-(columns-1)*spacing-width)/2;
        for(int j=0;j<columns;++j) knobs[static_cast<size_t>(controls::primaryIndex(shownModel,j))].setBounds(start+j*spacing,383,width,150);
        inputSlider.setBounds(110,327,270,26); outputSlider.setBounds(665,327,267,26);
        leftMeter.setBounds(48,139,365,182);
        rightMeter.setBounds(567,139,365,182);
        linkToggle.setBounds(400,327,180,26);
        selector.setBounds(540,42,180,30);
        reset.setBounds(750,42,180,30);
        learnButton.setBounds(48,729,140,28); resetMatchButton.setBounds(200,729,110,28);
        enableMatchButton.setBounds(322,729,110,28); originalButton.setBounds(444,729,140,28);
        matchReadout.setBounds(602,729,330,28); matchStatus.setBounds(48,764,884,20);
    }
    void paint(juce::Graphics& g) override {
        using namespace style;
        g.fillAll(cream);
        g.setColour(ink); g.setFont(juce::FontOptions(11.f));
        g.drawText("YODO AUDIO    /    AIRWINDOWS DSP",32,17,650,20,juce::Justification::left);
        g.setFont(juce::FontOptions(40.f)); g.drawText("AirVerb",30,40,240,55,juce::Justification::left);
        g.setFont(juce::FontOptions(13.f)); g.drawText("S T U D I O   /   V E R B",248,61,300,20,juce::Justification::left);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff343d36),0,108,juce::Colour(0xff1b2420),0,790,false));
        g.fillRoundedRectangle(16,108,948,680,16);
        g.setColour(gold.withAlpha(.2f)); g.drawRoundedRectangle({17.f,109.f,946.f,678.f},15,1);
        for(float x:{30.f,950.f}) for(float y:{122.f,774.f}) {
            g.setColour(juce::Colour(0xff111a15)); g.fillEllipse(x-4,y-4,8,8);
            g.setColour(juce::Colour(0xff646c5b)); g.drawLine(x-2,y+1,x+2,y-1,1);
        }
        g.setColour(juce::Colour(0xff101713)); g.fillRoundedRectangle(426,139,128,182,10);
        g.setColour(gold); g.setFont(juce::FontOptions(13.f));
        g.drawText(controls::modelName(shownModel),426,158,128,22,juce::Justification::centred);
        g.setFont(juce::FontOptions(36.f));
        g.drawText("VERB",426,193,128,54,juce::Justification::centred);
        g.setColour(muted); g.setFont(juce::FontOptions(10.f));
        g.drawText("STEREO",426,256,128,18,juce::Justification::centred);
        g.drawText("SAMPLE PEAK",426,275,128,18,juce::Justification::centred);
        g.setColour(gold); g.setFont(juce::FontOptions(10.f));
        g.drawText("INPUT",48,327,62,26,juce::Justification::left);
        g.drawText("OUTPUT",600,327,65,26,juce::Justification::left);
        g.setColour(cream.withAlpha(.12f)); g.drawHorizontalLine(362,48,932);
        g.drawHorizontalLine(713,48,932);
        const auto columns=controls::primaryCount(shownModel);
        const auto spacing=columns>4?884/columns:190;
        const auto width=columns>4?spacing-6:174;
        const auto start=(980-(columns-1)*spacing-width)/2;
        g.setColour(gold); g.setFont(juce::FontOptions(11.f));
        for(int j=0;j<columns;++j) g.drawText(juce::String(controls::name(shownModel,controls::primaryIndex(shownModel,j))).toUpperCase(),start+j*spacing,367,width,22,juce::Justification::centred);
        g.setColour(cream.withAlpha(.1f)); g.drawHorizontalLine(555,138,842);
        g.setColour(gold); g.setFont(juce::FontOptions(16.f));
        g.drawText(controls::description(shownModel),100,580,780,28,juce::Justification::centred);
        g.setColour(cream.withAlpha(.65f)); g.setFont(juce::FontOptions(13.f));
        g.drawText("Original Airwindows space. Independent studio interface.",100,615,780,24,juce::Justification::centred);
        g.drawText(controls::defaultsText(shownModel),70,647,840,24,juce::Justification::centred);
        g.setColour(ink.withAlpha(.65f)); g.setFont(juce::FontOptions(11.f));
        g.drawText(controls::modelName(shownModel)+" by Airwindows  /  Independent studio edition",32,796,710,24,juce::Justification::left);
        g.drawText("STEREO  /  FLOAT / DOUBLE",700,796,248,24,juce::Justification::right);
    }
private:
    void bindModel(int model) {
        if(model==shownModel) return;
        gainLinkAttachment.reset();
        enableMatchAttachment.reset(); originalAttachment.reset();
        for(auto& attachment:attachments) attachment.reset();
        shownModel=model;
        linkToggle.setVisible(true);
        for(int i=0;i<controls::maxParameters;++i) {
            auto& slider=knobs[static_cast<size_t>(i)];
            slider.setVisible(i<controls::count(model));
            if(i>=controls::count(model)) continue;
            slider.setName(controls::name(model,i));
            slider.setTooltip(juce::String(controls::tip(model,i))+" Double-click to reset.");
            attachments[static_cast<size_t>(i)]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,controls::id(model,i),slider);
            slider.textFromValueFunction=[i](double v) { return controls::display(i,v); };
            slider.valueFromTextFunction=[i](const juce::String& text) { return controls::parse(i,text); };
            slider.setDoubleClickReturnValue(true,controls::defaultValue(model,i)); slider.updateText();
        }
        gainLinkAttachment=std::make_unique<GainLinkAttachment>(
            *processor.state.getParameter(controls::inputId(model)),
            *processor.state.getParameter(controls::outputId(model)),
            *processor.state.getParameter(controls::linkId(model)),inputSlider,outputSlider,linkToggle);
        enableMatchAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,controls::matchId(model),enableMatchButton);
        originalAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,controls::originalId(model),originalButton);
        refreshMatch(); resized(); repaint();
    }
    void refreshMatch() {
        const auto& match=processor.matches[static_cast<size_t>(shownModel)];
        const bool learned=match.learned.load();
        const bool active=processor.learning.load() && processor.learningModel.load()==shownModel;
        const float db=match.db.load();
        const auto trim=(db>=0?juce::String("+"):juce::String{})+juce::String(db,1)+" dB";
        matchReadout.setText(learned?"Match "+trim:"Match -- dB",juce::dontSendNotification);
        learnButton.setButtonText(active?"Finish Learning":"Learn Match");
        enableMatchButton.setButtonText(enableMatchButton.getToggleState()?"Match ON":"Match OFF");
        originalButton.setButtonText(originalButton.getToggleState()?"A: Original":"B: Processed");
        juce::String text;
        if(active) text=controls::modelName(shownModel)+" learning: "+juce::String(processor.progress.load(),1)+" / 15 s of audio";
        else if(processor.learnResult.load()==static_cast<int>(LoudnessMatch::Result::tooShort)) text="Not enough audio. Play at least 3 seconds and retry.";
        else if(processor.learnResult.load()==static_cast<int>(LoudnessMatch::Result::noOutput)) text="No measurable reverb output. Check settings and retry.";
        else if(learned) {
            text="Input "+juce::String(match.inputLufs.load(),1)+" / reverb "+juce::String(match.outputLufs.load(),1)+" LUFS";
            if(std::abs(match.inputLufs.load()-match.outputLufs.load()-db)>.1f) text+=" | Gain capped at 24 dB";
            if(originalButton.getToggleState()) text+=" | Listening to original";
            else if(!enableMatchButton.getToggleState()) text+=" | Compensation disabled";
            else text+=" | Fixed gain applied";
        } else text="Play a representative section, then Learn Match for "+controls::modelName(shownModel)+".";
        matchStatus.setText(text,juce::dontSendNotification);
    }
    void timerCallback() override {
        if(sizeChoice.getSelectedId()!=processor.uiSize.load()) {
            sizeChoice.setSelectedId(processor.uiSize.load(),juce::dontSendNotification);
            if(sizeChanged) sizeChanged();
        }
        bindModel(processor.selectedModel());
        refreshMatch();
        const auto now=juce::Time::getMillisecondCounterHiRes();
        const auto elapsed=(now-lastMeterTime)*.001;
        lastMeterTime=now;
        const auto sequence=processor.meterSequence.load(std::memory_order_acquire);
        if(sequence!=lastMeterSequence) {
            lastMeterSequence=sequence; idleSeconds=0;
            for(int c=0;c<2;++c) targets[static_cast<size_t>(c)]=processor.peaks[c].exchange(0,std::memory_order_relaxed);
        } else {
            idleSeconds+=elapsed;
            if(idleSeconds>processor.meterTimeoutSeconds.load(std::memory_order_relaxed)) targets.fill(0);
        }
        for(size_t c=0;c<2;++c) ballistics[c].advance(targets[c],elapsed);
        leftMeter.setLevelDb(ballistics[0].getLevelDb()); rightMeter.setLevelDb(ballistics[1].getLevelDb());
        repaint(48,139,884,183);
    }
    OutputMeter leftMeter{"LEFT"},rightMeter{"RIGHT"};
    VerbProcessor& processor;
    style::Look look;
    std::array<juce::Slider,controls::maxParameters> knobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,controls::maxParameters> attachments;
    juce::Slider inputSlider,outputSlider;
    juce::ToggleButton linkToggle;
    std::unique_ptr<GainLinkAttachment> gainLinkAttachment;
    juce::ComboBox selector,sizeChoice;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> selectorAttachment;
    int shownModel=-1;
    juce::TextButton reset;
    juce::TextButton learnButton{"Learn Match"},resetMatchButton{"Reset Match"},enableMatchButton{"Match ON"},originalButton{"B: Processed"};
    juce::Label matchReadout,matchStatus;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enableMatchAttachment,originalAttachment;
    juce::TooltipWindow tooltip{this,600};
    std::array<MeterBallistics,2> ballistics;
    std::array<float,2> targets{};
    uint32_t lastMeterSequence=0;
    double lastMeterTime=0,idleSeconds=0;
};
// Keep logical layout and hit testing together; host/HiDPI scaling remains independent.
class VerbEditor final : public juce::AudioProcessorEditor {
public:
    explicit VerbEditor(VerbProcessor& p) : AudioProcessorEditor(p), processor(p), panel(p) {
        addAndMakeVisible(panel);
        setResizable(false,false);
        panel.sizeChanged=[this] { applySize(); };
        applySize();
    }
private:
    void applySize() {
        const float scale=processor.uiSize.load()==1?.75f:processor.uiSize.load()==3?1.25f:1.f;
        panel.setTransform(juce::AffineTransform::scale(scale));
        panel.setBounds(0,0,980,830);
        setSize(juce::roundToInt(980*scale),juce::roundToInt(830*scale));
    }
    VerbProcessor& processor;
    VerbPanel panel;
};
juce::AudioProcessorEditor* VerbProcessor::createEditor() { return new VerbEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VerbProcessor; }
