#include "PluginEditor.h"

using namespace juce;

//==============================================================================
HavocLookAndFeel::HavocLookAndFeel()
{
    setColour (Label::textColourId, Colour (0xffd02830));
    setColour (Slider::textBoxTextColourId, Colour (0xffdddddd));
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (ToggleButton::textColourId, Colour (0xffdddddd));
    setColour (ToggleButton::tickColourId, Colour (0xffd02830));
    setColour (ToggleButton::tickDisabledColourId, Colour (0xff555555));
}

void HavocLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, Slider&)
{
    auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    g.setColour (Colour (0xffb01820));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i * (endAngle - startAngle) / 10.0f;
        g.drawLine (c.x + r * 0.86f * std::sin (a), c.y - r * 0.86f * std::cos (a),
                    c.x + r * 0.98f * std::sin (a), c.y - r * 0.98f * std::cos (a), 2.0f);
    }

    const float kr = r * 0.74f;
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3c), c.x, c.y - kr, Colour (0xff0c0c0d), c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (Colour (0xff6a6a6e));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.5f);

    Path p;
    p.addRoundedRectangle (-2.0f, -kr * 0.92f, 4.0f, kr * 0.5f, 1.5f);
    g.setColour (Colour (0xffeeeeee));
    g.fillPath (p, AffineTransform::rotation (angle).translated (c.x, c.y));
}

//==============================================================================
FootSwitch::FootSwitch (AudioProcessorValueTreeState& state, const String& onId,
                        const String& latchId, const String& labelText)
    : onParam (state.getParameter (onId)), latchParam (state.getParameter (latchId)), text (labelText)
{
    startTimerHz (30);
}

void FootSwitch::setParam (RangedAudioParameter* p, float v)
{
    p->beginChangeGesture();
    p->setValueNotifyingHost (v);
    p->endChangeGesture();
}

void FootSwitch::timerCallback()
{
    const bool on = onParam->getValue() > 0.5f;
    const bool latch = latchParam->getValue() > 0.5f;
    if (on != lastOn || latch != lastLatch)
    {
        lastOn = on;
        lastLatch = latch;
        repaint();
    }
}

void FootSwitch::paint (Graphics& g)
{
    const bool on = onParam->getValue() > 0.5f;
    const bool latch = latchParam->getValue() > 0.5f;

    auto b = getLocalBounds().toFloat();
    auto labelArea = b.removeFromBottom (b.getHeight() * 0.22f);
    const float d = jmin (b.getWidth(), b.getHeight()) * 0.9f;
    auto cir = Rectangle<float> (d, d).withCentre (b.getCentre());

    g.setColour (on ? Colour (0xffd02830) : Colour (0xff2a2a2c));
    g.fillEllipse (cir.expanded (4.0f));
    g.setGradientFill (ColourGradient (Colour (0xff9a9a9e), cir.getCentreX(), cir.getY(),
                                       Colour (0xff3c3c40), cir.getCentreX(), cir.getBottom(), false));
    g.fillEllipse (cir);
    g.setColour (Colour (0xff111111));
    g.drawEllipse (cir, 2.0f);

    g.setColour (on ? Colour (0xffff4a52) : Colour (0xffb01820));
    g.setFont (labelArea.getHeight() * 0.7f);
    g.drawText (text + (latch ? "  (LATCH)" : ""), labelArea, Justification::centred);
}

void FootSwitch::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        PopupMenu m;
        m.addItem (1, "Latch", true, latchParam->getValue() > 0.5f);
        SafePointer<FootSwitch> safe (this);
        m.showMenuAsync (PopupMenu::Options(), [safe] (int result)
        {
            if (safe != nullptr && result == 1)
                setParam (safe->latchParam, safe->latchParam->getValue() > 0.5f ? 0.0f : 1.0f);
        });
        return;
    }

    if (latchParam->getValue() > 0.5f)
    {
        setParam (onParam, onParam->getValue() > 0.5f ? 0.0f : 1.0f);
    }
    else
    {
        holding = true;
        setParam (onParam, 1.0f);
    }
}

void FootSwitch::mouseUp (const MouseEvent&)
{
    if (holding)
    {
        holding = false;
        setParam (onParam, 0.0f);
    }
}

//==============================================================================
OctaveHavocAudioProcessorEditor::OctaveHavocAudioProcessorEditor (OctaveHavocAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      foot1 (p.apvts, ids::oct1, ids::latch1, "OCTAVE +1"),
      foot2 (p.apvts, ids::oct2, ids::latch2, "OCTAVE +2")
{
    setLookAndFeel (&lnf);

    setupKnob (input,  "INPUT",  ids::input);
    setupKnob (gate,   "GATE",   ids::gate);
    setupKnob (blend,  "BLEND",  ids::blend);
    setupKnob (output, "OUTPUT", ids::output);
    setupKnob (panic,  "PANIC",  ids::panic);
    setupKnob (chaos,  "CHAOS",  ids::chaos);
    setupKnob (speed,  "SPEED",  ids::speed);
    setupKnob (rise,   "RISE",   ids::rise);

    // pembacaan nilai (dipasang setelah attachment agar tidak tertimpa)
    auto dbFn = [] (double v) { return String (v, 1) + " dB"; };
    auto numFn = [] (const String& t) { return t.getDoubleValue(); };
    for (auto* k : { &input, &output })
    {
        k->slider.textFromValueFunction = dbFn;
        k->slider.valueFromTextFunction = numFn;
        k->slider.updateText();
    }

    gate.slider.textFromValueFunction = [] (double v) { return v <= -79.9 ? String ("OFF") : String (v, 1) + " dB"; };
    gate.slider.valueFromTextFunction = numFn;
    gate.slider.updateText();

    auto pctFn = [] (double v) { return String (roundToInt (v * 100.0)) + "%"; };
    auto pctIn = [] (const String& t) { return t.getDoubleValue() / 100.0; };
    for (auto* k : { &blend, &panic, &chaos, &speed })
    {
        k->slider.textFromValueFunction = pctFn;
        k->slider.valueFromTextFunction = pctIn;
        k->slider.updateText();
    }

    rise.slider.textFromValueFunction = [this] (double v) -> String
    {
        if (*proc.apvts.getRawParameterValue (ids::riseSync) > 0.5f)
            return String (riseDivNames[jlimit (0, numRiseDivs - 1, roundToInt (v * (numRiseDivs - 1)))]);
        const double ms = 5.0 * std::pow (400.0, v);
        return ms >= 1000.0 ? String (ms / 1000.0, 2) + " s" : String (roundToInt (ms)) + " ms";
    };
    rise.slider.valueFromTextFunction = [this] (const String&) { return rise.slider.getValue(); };
    rise.slider.updateText();

    addAndMakeVisible (syncButton);
    addAndMakeVisible (stereoButton);
    syncAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ids::riseSync, syncButton);
    stereoAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ids::stereo, stereoButton);
    syncButton.onClick = [this] { rise.slider.updateText(); };

    addAndMakeVisible (foot1);
    addAndMakeVisible (foot2);

    setResizable (true, true);
    setResizeLimits (360, 480, 900, 1200);
    getConstrainer()->setFixedAspectRatio (0.75);
    setSize (480, 640);
}

OctaveHavocAudioProcessorEditor::~OctaveHavocAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void OctaveHavocAudioProcessorEditor::setupKnob (Knob& k, const String& name, const char* paramId)
{
    k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 16);
    k.slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);

    k.label.setText (name, dontSendNotification);
    k.label.setJustificationType (Justification::centred);

    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
    k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
}

void OctaveHavocAudioProcessorEditor::place (Knob& k, Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (18));
    k.slider.setBounds (r);
}

void OctaveHavocAudioProcessorEditor::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff1b1b1d), 0.0f, 0.0f, Colour (0xff060607), 0.0f, b.getBottom(), false));
    g.fillAll();

    g.setColour (Colour (0xff7a1016));
    g.drawRect (b.reduced (6.0f), 1.5f);

    const float h = (float) getHeight();
    g.setColour (Colour (0xffe8e8e8));
    g.setFont (h * 0.07f);
    g.drawText ("OCTAVE HAVOC", Rectangle<float> (0.0f, h * 0.68f, (float) getWidth(), h * 0.1f), Justification::centred);
}

void OctaveHavocAudioProcessorEditor::resized()
{
    const int w = getWidth(), h = getHeight();
    auto area = getLocalBounds().reduced (jmax (10, w / 24));

    // baris atas: input, gate, blend, output
    auto top = area.removeFromTop (roundToInt (h * 0.15f));
    const int q = top.getWidth() / 4;
    place (input,  top.removeFromLeft (q));
    place (gate,   top.removeFromLeft (q));
    place (blend,  top.removeFromLeft (q));
    place (output, top);

    // saklar mono/stereo
    auto tog = area.removeFromTop (roundToInt (h * 0.05f));
    stereoButton.setBounds (tog.withSizeKeepingCentre (jmin (140, tog.getWidth()), tog.getHeight()));

    // area pedal: panic, chaos, speed, rise
    auto mid = area.removeFromTop (roundToInt (h * 0.46f));
    const int third = mid.getWidth() / 3;
    auto row = mid.removeFromTop (roundToInt (mid.getHeight() * 0.62f));
    const int drop = roundToInt (row.getHeight() * 0.22f);

    place (panic, row.removeFromLeft (third).withTrimmedTop (drop));
    place (chaos, row.removeFromLeft (third));
    place (speed, row.withTrimmedTop (drop));

    auto riseR = Rectangle<int> (mid.getCentreX() - third / 2, mid.getY(), third, mid.getHeight());
    syncButton.setBounds (riseR.removeFromBottom (24));
    place (rise, riseR);

    // judul digambar di paint() (0.68 - 0.78), footswitch di bawah
    auto feet = Rectangle<int> (area.getX(), roundToInt (h * 0.78f), area.getWidth(), area.getBottom() - roundToInt (h * 0.78f));
    foot1.setBounds (feet.removeFromLeft (feet.getWidth() / 2).reduced (8));
    foot2.setBounds (feet.reduced (8));
}
