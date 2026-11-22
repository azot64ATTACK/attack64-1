#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <cstdint>

// ---------- Голоса ----------
void Attack64Voice::startNote(int note, float velocity, juce::SynthesiserSound*, int)
{
    freq = juce::MidiMessage::getMidiNoteInHertz(note) * std::pow(2.0, std::round(P.octave));
    vel = velocity;
    ph1 = ph2 = 0.0;
    lp = bp = 0.0f;
    const double sr = getSampleRate();
    ampEnv.setSampleRate(sr);
    filtEnv.setSampleRate(sr);
    ampEnv.setParameters({ P.aAtk, P.aDec, P.aSus, P.aRel });
    filtEnv.setParameters({ P.bAtk, P.bDec, P.bSus, P.bRel });
    ampEnv.noteOn();
    filtEnv.noteOn();
}

void Attack64Voice::stopNote(float, bool allowTail)
{
    ampEnv.noteOff();
    filtEnv.noteOff();
    if (!allowTail || !ampEnv.isActive())
        clearCurrentNote();
}

float Attack64Voice::osc(float type, double ph)
{
    switch (juce::roundToInt(type)) {
        case 0:  return (float) std::sin(juce::MathConstants<double>::twoPi * ph);
        case 1:  return (float) (2.0 * ph - 1.0);
        case 2:  return ph < 0.5 ? 1.0f : -1.0f;
        default: return (float) (4.0 * std::abs(ph - 0.5) - 1.0);
    }
}

void Attack64Voice::renderNextBlock(juce::AudioBuffer<float>& out, int start, int num)
{
    if (!ampEnv.isActive()) return;
    ampEnv.setParameters({ P.aAtk, P.aDec, P.aSus, P.aRel });
    filtEnv.setParameters({ P.bAtk, P.bDec, P.bSus, P.bRel });

    const double sr = getSampleRate();
    const double inc = freq / sr;
    const float q = std::max(0.05f, 1.5f * (1.0f - 0.97f * P.resonance));
    const int type = juce::roundToInt(P.filterType);

    for (int i = 0; i < num; ++i)
    {
        const float ae = ampEnv.getNextSample();
        const float fe = filtEnv.getNextSample();
        const float x = (osc(P.wave1, ph1) + 0.5f * osc(P.wave2, ph2)) * P.level * 0.6f;
        ph1 += inc; if (ph1 >= 1.0) ph1 -= 1.0;
        ph2 += inc; if (ph2 >= 1.0) ph2 -= 1.0;

        // Фильтр State Variable: огибающая B двигает частоту среза
        double fc = P.cutoff * std::pow(2.0, P.envAmt * 4.0 * fe);
        fc = juce::jlimit(20.0, sr * 0.16, fc);
        const float f = (float) (2.0 * std::sin(juce::MathConstants<double>::pi * fc / sr));
        const float hp = x - lp - q * bp;
        bp += f * hp;
        lp += f * bp;
        const float y = (type == 0) ? lp : (type == 1) ? hp : (type == 2) ? bp : (lp + hp);

        const float v = y * ae * vel * P.volume;
        for (int ch = 0; ch < out.getNumChannels(); ++ch)
            out.addSample(ch, start + i, v);
    }
    if (!ampEnv.isActive()) clearCurrentNote();
}

// ---------- Процессор ----------
Attack64Processor::Attack64Processor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createLayout())
{
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new Attack64Voice(sp));
    synth.addSound(new Attack64Sound());
}

juce::AudioProcessorValueTreeState::ParameterLayout Attack64Processor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int t = 0; t < 10; ++t)
        for (int i = 0; i < kTabs[t].count; ++i)
        {
            const auto& d = kTabs[t].p[i];
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { paramId(t, i), 1 }, d.name,
                juce::NormalisableRange<float>(d.min, d.max, 0.0f, d.skew), d.def));
        }
    return layout;
}

void Attack64Processor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate(sampleRate);
    dlL.assign((size_t) (2.0 * sampleRate) + 1, 0.0f);
    dlR.assign(dlL.size(), 0.0f);
    dlW = 0; lpL = lpR = 0.0f;
    reverb.setSampleRate(sampleRate);
    reverb.reset();
}

void Attack64Processor::readParams()
{
    auto v = [this](int t, int i) { return apvts.getRawParameterValue(paramId(t, i))->load(); };

    sp.wave1 = v(0, 0); sp.wave2 = v(0, 1); sp.octave = v(0, 2); sp.level = v(0, 3) / 100.0f;
    sp.filterType = v(1, 0); sp.cutoff = v(1, 1); sp.resonance = v(1, 2) / 100.0f; sp.envAmt = v(1, 3) / 100.0f;
    sp.aAtk = v(2, 0) / 1000.0f; sp.aDec = v(2, 1) / 1000.0f; sp.aSus = v(2, 2) / 100.0f; sp.aRel = v(2, 3) / 1000.0f;
    sp.bAtk = v(3, 0) / 1000.0f; sp.bDec = v(3, 1) / 1000.0f; sp.bSus = v(3, 2) / 100.0f; sp.bRel = v(3, 3) / 1000.0f;
    sp.volume = juce::Decibels::decibelsToGain(v(9, 0));

    lfoRate = v(4, 1);
    lfoDepth = v(4, 2) / 100.0f;

    fxTime = v(7, 0);
    fxFb = juce::jmin(0.95f, v(7, 1) / 100.0f);
    fxMix = v(7, 2) / 100.0f;
    fxTone = v(7, 3) / 100.0f;

    revP.roomSize = (v(7, 4) * 0.6f + v(7, 5) * 0.4f) / 100.0f;
    revP.damping = v(7, 6) / 100.0f;
    revP.wetLevel = v(7, 7) / 100.0f;
    revP.dryLevel = 1.0f;
    revP.width = 1.0f;
    reverb.setParameters(revP);
}

// Ping-pong задержка: левый канал получает отражение правого и наоборот
void Attack64Processor::processDelay(float* L, float* R, int n)
{
    const int size = (int) dlL.size();
    const int d = juce::jlimit(1, size - 1, (int) (fxTime * currentSampleRate / 1000.0));
    const float a = 0.05f + 0.95f * fxTone;

    for (int i = 0; i < n; ++i)
    {
        const int r = (dlW - d + size) % size;
        const float dl = dlL[(size_t) r], dr = dlR[(size_t) r];

        lpL += a * (dr * fxFb - lpL);
        lpR += a * (dl * fxFb - lpR);

        const float inL = L[i], inR = R[i];
        dlL[(size_t) dlW] = inL + lpL;
        dlR[(size_t) dlW] = inR + lpR;
        dlW = (dlW + 1) % size;

        L[i] = inL * (1.0f - fxMix) + dl * fxMix;
        R[i] = inR * (1.0f - fxMix) + dr * fxMix;
    }
}

void Attack64Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    readParams();
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());

    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer(0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;

    // LFO 1: тремоло (громкость плавно колеблется)
    const float inc = (float) (lfoRate / currentSampleRate);
    for (int i = 0; i < n; ++i)
    {
        const float lfo = 0.5f + 0.5f * std::sin(juce::MathConstants<float>::twoPi * lfoPhase);
        const float g = 1.0f - lfoDepth * lfo;
        L[i] *= g; R[i] *= g;
        lfoPhase += inc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }

    processDelay(L, R, n);
    reverb.processStereo(L, R, n);
}

void Attack64Processor::setParam(const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter(id))
        p->setValueNotifyingHost(p->convertTo0to1(value));
}

void Attack64Processor::applyDelayPreset(int i)
{
    const auto& p = kDelayPresets[juce::jlimit(0, 19, i)];
    setParam(paramId(7, 0), p.a); setParam(paramId(7, 1), p.b);
    setParam(paramId(7, 2), p.c); setParam(paramId(7, 3), p.d);
}

void Attack64Processor::applyReverbPreset(int i)
{
    const auto& p = kReverbPresets[juce::jlimit(0, 19, i)];
    setParam(paramId(7, 4), p.a); setParam(paramId(7, 5), p.b);
    setParam(paramId(7, 6), p.c); setParam(paramId(7, 7), p.d);
}

// ---------- 300 синт-пресетов: 10 категорий по 30, детерминированная генерация ----------
namespace {
struct Cat { const char* name; int w0, w1, o0, o1; float c0, c1, a0, a1, d0, d1, s0, s1, r0, r1; };
const Cat kCats[10] = {
    { "Lead",    1, 2, 0, 1,  3000,14000,    0, 40,  100, 600,  40, 80,  100, 500 },
    { "Bass",    1, 2,-2,-1,   200, 2500,    0, 10,   80, 400,  30, 70,   40, 200 },
    { "Pad",     0, 1, 0, 0,  1500, 9000,  600,2500,  800,2000,  60, 90, 1500,4000 },
    { "Pluck",   1, 2, 0, 1,  1000, 8000,    0,  5,  150, 500,   0, 10,   80, 300 },
    { "Keys",    0, 3, 0, 0,  4000,12000,    5, 30,  400,1200,  40, 70,  300, 800 },
    { "Brass",   1, 2, 0, 0,  1200, 6000,   80,250,  200, 500,  70, 90,  150, 400 },
    { "Bell",    0, 3, 1, 2,  6000,16000,    0,  5, 1200,3000,   0, 15, 1500,3500 },
    { "Strings", 1, 1, 0, 0,  2000, 8000,  400,1200,  500,1000,  75, 95,  800,2000 },
    { "Arp",     2, 1, 0, 1,  2500,10000,    0,  8,  100, 300,  20, 50,   50, 200 },
    { "FX",      0, 3,-1, 2,   300,15000,  100,2000,  500,3000,   0, 60,  500,3000 },
};
const char* kNames[30] = { "Silk","Glow","Dusk","Steel","Velvet","Neon","Ember","Frost","Drift","Pulse",
    "Haze","Prism","Chrome","Ocean","Dream","Rust","Halo","Vapor","Crystal","Night",
    "Solar","Echo","Shade","Wave","Bloom","Storm","Amber","Lunar","Cobalt","Fable" };

struct Gen {
    uint32_t s;
    float rf(float a, float b) {
        s += 0x6D2B79F5u;
        uint32_t t = (s ^ (s >> 15)) * (1u | s);
        t = (t + (t ^ (t >> 7)) * (61u | t)) ^ t;
        const float r = (float) ((t ^ (t >> 14)) / 4294967296.0);
        return a + (b - a) * r;
    }
    float ri(float a, float b) { return std::round(rf(a, b)); }
};

// v[tab][param] для вкладок OSC..MOD (0..6)
void buildSynthPreset(int index, float v[7][4])
{
    const int cat = index / 30, k = index % 30;
    const Cat& c = kCats[cat];
    Gen g { (uint32_t) (cat * 100 + k + 1) };

    v[0][0] = g.ri((float) c.w0, (float) c.w1); v[0][1] = g.ri(0, 3);
    v[0][2] = g.ri((float) c.o0, (float) c.o1); v[0][3] = g.ri(70, 100);

    v[1][0] = g.ri(0, 3); v[1][1] = g.rf(c.c0, c.c1); v[1][2] = g.ri(0, 60); v[1][3] = g.ri(-40, 80);

    v[2][0] = g.ri(c.a0, c.a1); v[2][1] = g.ri(c.d0, c.d1); v[2][2] = g.ri(c.s0, c.s1); v[2][3] = g.ri(c.r0, c.r1);

    v[3][0] = g.ri(0, 300); v[3][1] = g.ri(200, 1500); v[3][2] = g.ri(20, 80); v[3][3] = g.ri(200, 1500);

    v[4][0] = g.ri(0, 3); v[4][1] = g.rf(0.2f, 8); v[4][2] = g.ri(0, 60); v[4][3] = g.ri(0, 359);
    v[5][0] = g.ri(0, 3); v[5][1] = g.rf(0.2f, 8); v[5][2] = g.ri(0, 60); v[5][3] = g.ri(0, 359);

    v[6][0] = g.ri(50, 100); v[6][1] = g.ri(-40, 60); v[6][2] = g.ri(0, 60); v[6][3] = g.ri(0, 40);
}
} // namespace

juce::String Attack64Processor::getSynthPresetName(int index)
{
    const int cat = index / 30, k = index % 30;
    return juce::String(kCats[cat].name) + " " + juce::String(k + 1).paddedLeft('0', 2) + " " + kNames[k];
}

void Attack64Processor::applySynthPreset(int index)
{
    float v[7][4];
    buildSynthPreset(juce::jlimit(0, 299, index), v);
    for (int t = 0; t < 7; ++t)
        for (int i = 0; i < 4; ++i)
            setParam(paramId(t, i), v[t][i]);
}

void Attack64Processor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Attack64Processor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* Attack64Processor::createEditor()
{
    return new Attack64Editor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Attack64Processor();
}
