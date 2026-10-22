#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Tampilan knob gelap dengan tick merah (desain sendiri, bukan artwork pihak lain).
class HavocLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HavocLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

// Footswitch: momentary secara default; klik kanan -> pilih "Latch".
class FootSwitch : public juce::Component, private juce::Timer
{
public:
    FootSwitch (juce::AudioProcessorValueTreeState& state, const juce::String& onId,
                const juce::String& latchId, const juce::String& labelText);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    static void setParam (juce::RangedAudioParameter* p, float v);

    juce::RangedAudioParameter* onParam;
    juce::RangedAudioParameter* latchParam;
    juce::String text;
    bool holding = false;
    bool lastOn = false, lastLatch = false;
};

class OctaveHavocAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OctaveHavocAudioProcessorEditor (OctaveHavocAudioProcessor&);
    ~OctaveHavocAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& name, const char* paramId);
    static void place (Knob&, juce::Rectangle<int>);

    OctaveHavocAudioProcessor& proc;
    HavocLookAndFeel lnf;

    Knob input, output, gate, blend, panic, chaos, speed, rise;
    juce::ToggleButton syncButton { "TEMPO SYNC" }, stereoButton { "STEREO" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAtt, stereoAtt;
    FootSwitch foot1, foot2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctaveHavocAudioProcessorEditor)
};
