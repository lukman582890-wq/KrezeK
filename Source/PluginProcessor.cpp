#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout OctaveHavocAudioProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pid = [] (const char* id) { return ParameterID { id, 1 }; };

    l.add (std::make_unique<AudioParameterFloat> (pid (ids::input),  "Input",  NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (pid (ids::output), "Output", NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    l.add (std::make_unique<AudioParameterFloat> (pid (ids::gate),   "Gate",   NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -80.0f));
    l.add (std::make_unique<AudioParameterFloat> (pid (ids::blend),  "Blend",  NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

    l.add (std::make_unique<AudioParameterFloat> (pid (ids::panic), "Panic", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    l.add (std::make_unique<AudioParameterFloat> (pid (ids::chaos), "Chaos", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    l.add (std::make_unique<AudioParameterFloat> (pid (ids::speed), "Speed", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
    l.add (std::make_unique<AudioParameterFloat> (pid (ids::rise),  "Rise",  NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    l.add (std::make_unique<AudioParameterBool> (pid (ids::riseSync), "Rise Tempo Sync", false));
    l.add (std::make_unique<AudioParameterBool> (pid (ids::oct1),     "Octave +1", false));
    l.add (std::make_unique<AudioParameterBool> (pid (ids::oct2),     "Octave +2", false));
    l.add (std::make_unique<AudioParameterBool> (pid (ids::latch1),   "Octave +1 Latch", false));
    l.add (std::make_unique<AudioParameterBool> (pid (ids::latch2),   "Octave +2 Latch", false));
    l.add (std::make_unique<AudioParameterBool> (pid (ids::stereo),   "Stereo", true));
    return l;
}

OctaveHavocAudioProcessor::OctaveHavocAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pInput    = apvts.getRawParameterValue (ids::input);
    pOutput   = apvts.getRawParameterValue (ids::output);
    pGate     = apvts.getRawParameterValue (ids::gate);
    pBlend    = apvts.getRawParameterValue (ids::blend);
    pPanic    = apvts.getRawParameterValue (ids::panic);
    pChaos    = apvts.getRawParameterValue (ids::chaos);
    pSpeed    = apvts.getRawParameterValue (ids::speed);
    pRise     = apvts.getRawParameterValue (ids::rise);
    pRiseSync = apvts.getRawParameterValue (ids::riseSync);
    pOct1     = apvts.getRawParameterValue (ids::oct1);
    pOct2     = apvts.getRawParameterValue (ids::oct2);
    pStereo   = apvts.getRawParameterValue (ids::stereo);
}

bool OctaveHavocAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void OctaveHavocAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    for (int v = 0; v < 2; ++v)
        for (int ch = 0; ch < 2; ++ch)
            shifters[v][ch].prepare (sr);

    allpass[0][0].prepare ((int) (0.0053 * sr));
    allpass[0][1].prepare ((int) (0.0079 * sr));
    allpass[1][0].prepare ((int) (0.0057 * sr));
    allpass[1][1].prepare ((int) (0.0083 * sr));

    inGain.reset (sr, 0.02);
    outGain.reset (sr, 0.02);
    blendSm.reset (sr, 0.02);
    inGain.setCurrentAndTargetValue (Decibels::decibelsToGain (pInput->load()));
    outGain.setCurrentAndTargetValue (Decibels::decibelsToGain (pOutput->load()));
    blendSm.setCurrentAndTargetValue (pBlend->load());

    voiceOct[0] = voiceOct[1] = 0.0f;
    voiceGain[0] = voiceGain[1] = 0.0f;
    gateEnv = 0.0f;
    gateGain = 1.0f;
    chaosState = chaosTarget = 0.0f;
    chaosCountdown = 0;
}

void OctaveHavocAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int numCh = jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
    if (numCh == 0)
        return;

    // --- parameter ---
    const float panic = pPanic->load();
    const float chaos = pChaos->load();
    const float speed = pSpeed->load();
    const float riseKnob = pRise->load();
    const bool riseSync = pRiseSync->load() > 0.5f;
    const bool engaged[2] = { pOct1->load() > 0.5f, pOct2->load() > 0.5f };
    const bool stereo = (pStereo->load() > 0.5f) && numCh == 2;
    const float gateDb = pGate->load();
    const bool gateOff = gateDb <= -79.9f;
    const float gateThresh = Decibels::decibelsToGain (gateDb);

    inGain.setTargetValue (Decibels::decibelsToGain (pInput->load()));
    outGain.setTargetValue (Decibels::decibelsToGain (pOutput->load()));
    blendSm.setTargetValue (pBlend->load());

    // --- waktu RISE ---
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    double riseMs;
    if (riseSync)
    {
        const int idx = jlimit (0, numRiseDivs - 1, roundToInt (riseKnob * (float) (numRiseDivs - 1)));
        riseMs = (double) riseBeats[idx] * 60000.0 / jmax (20.0, bpm);
    }
    else
    {
        riseMs = 5.0 * std::pow (400.0, (double) riseKnob); // 5 ms .. 2 s
    }
    const float riseSamples = (float) jmax (1.0, riseMs * 0.001 * sr);

    // --- koefisien ---
    const float gateEnvRel = std::exp (-1.0f / (0.05f * (float) sr));
    const float gateAtk = 1.0f - std::exp (-1.0f / (0.002f * (float) sr));
    const float gateRel = 1.0f - std::exp (-1.0f / (0.08f * (float) sr));
    const float voiceAtk = 1.0f - std::exp (-1.0f / (0.008f * (float) sr));
    const float voiceRel = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
    const float chaosCoef = 1.0f - std::exp (-1.0f / (0.015f * (float) sr));

    const float lfoHz = 0.3f + speed * speed * 20.0f;
    const float lfoInc = lfoHz / (float) sr;
    const float ampDepth = speed * 0.8f;
    const float apG = speed * 0.75f;
    const float apMix = speed * 0.6f;

    const float detuneCents = panic * 100.0f;
    const float chaosCentsMax = chaos * 500.0f;

    float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int i = 0; i < numSamples; ++i)
    {
        const float gIn = inGain.getNextValue();
        const float gOut = outGain.getNextValue();
        const float blend = blendSm.getNextValue();

        float x[2] = { data[0][i] * gIn, numCh > 1 ? data[1][i] * gIn : 0.0f };
        if (numCh == 2 && ! stereo)
            x[0] = x[1] = 0.5f * (x[0] + x[1]);

        // noise gate
        float level = std::abs (x[0]);
        if (numCh > 1)
            level = jmax (level, std::abs (x[1]));
        gateEnv = level > gateEnv ? level : level + (gateEnv - level) * gateEnvRel;
        const bool open = gateOff || gateEnv > gateThresh;
        gateGain += ((open ? 1.0f : 0.0f) - gateGain) * (open ? gateAtk : gateRel);
        for (int ch = 0; ch < numCh; ++ch)
            x[ch] *= gateGain;

        // CHAOS: random walk pitch yang di-smoothing
        if (--chaosCountdown <= 0)
        {
            chaosTarget = rng.nextFloat() * 2.0f - 1.0f;
            chaosCountdown = (int) (sr * (0.03 + 0.10 * (double) rng.nextFloat()));
        }
        chaosState += (chaosTarget - chaosState) * chaosCoef;
        const float chaosCents = chaosState * chaosCentsMax;

        // voice oktaf: glide RISE + detune PANIC
        float ratio[2];
        for (int v = 0; v < 2; ++v)
        {
            const float n = (float) (v + 1);
            const float target = engaged[v] ? n : 0.0f;
            const float step = n / riseSamples;
            if (voiceOct[v] < target)      voiceOct[v] = jmin (voiceOct[v] + step, target);
            else if (voiceOct[v] > target) voiceOct[v] = jmax (voiceOct[v] - step, target);

            voiceGain[v] += ((engaged[v] ? 1.0f : 0.0f) - voiceGain[v]) * (engaged[v] ? voiceAtk : voiceRel);

            const float cents = (v == 0 ? detuneCents : -detuneCents) + chaosCents;
            ratio[v] = std::exp2 (voiceOct[v] + cents / 1200.0f);
        }

        // SPEED: LFO amplitude
        lfoPhase += lfoInc;
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
        const float lfo = 0.5f + 0.5f * std::sin (MathConstants<float>::twoPi * lfoPhase);
        const float ampMod = 1.0f - ampDepth * lfo;

        const float gsum = voiceGain[0] + voiceGain[1];
        const float eff = jmin (1.0f, gsum);
        const float norm = eff / jmax (gsum, 1.0e-4f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float v1 = shifters[0][ch].process (x[ch], ratio[0]);
            const float v2 = shifters[1][ch].process (x[ch], ratio[1]);

            float wet = (voiceGain[0] * v1 + voiceGain[1] * v2) * norm * ampMod;

            float ap = allpass[ch][0].process (wet, apG);
            ap = allpass[ch][1].process (ap, apG);
            wet += (ap - wet) * apMix;

            const float processed = x[ch] * (1.0f - eff) + wet;
            data[ch][i] = (x[ch] * (1.0f - blend) + processed * blend) * gOut;
        }
    }
}

AudioProcessorEditor* OctaveHavocAudioProcessor::createEditor()
{
    return new OctaveHavocAudioProcessorEditor (*this);
}

void OctaveHavocAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OctaveHavocAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctaveHavocAudioProcessor();
}
