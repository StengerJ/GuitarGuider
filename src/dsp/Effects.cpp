#include "Effects.h"

namespace gg
{

//==============================================================================
juce::String Parameter::toText() const
{
    if (! spec.choices.isEmpty())
        return spec.choices[juce::jlimit (0, spec.choices.size() - 1, (int) std::lround (get()))];

    const auto v = get();
    if (spec.unit == "ms")  return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s" : juce::String (juce::roundToInt (v)) + " ms";
    if (spec.unit == "Hz")  return juce::String (v, v < 10.0f ? 2 : 1) + " Hz";
    if (spec.unit == "dB")  return (v > 0 ? "+" : "") + juce::String (v, 1) + " dB";
    if (spec.unit == "%")   return juce::String (juce::roundToInt (v * 100.0f)) + "%";
    return juce::String (v, 1);
}

Parameter* Effect::param (const juce::String& id) const
{
    for (auto& p : parameters)
        if (p->spec.id == id) return p.get();
    return nullptr;
}

juce::var Effect::toVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("type", typeId());
    o->setProperty ("on", enabled.load());
    auto* ps = new juce::DynamicObject();
    for (auto& p : parameters)
        ps->setProperty (p->spec.id, p->get());
    o->setProperty ("params", juce::var (ps));
    return juce::var (o);
}

void Effect::fromVar (const juce::var& v)
{
    if (v.hasProperty ("on"))
        enabled = (bool) v["on"];
    if (auto* ps = v["params"].getDynamicObject())
        for (auto& prop : ps->getProperties())
            if (auto* p = param (prop.name.toString()))
                p->set ((float) prop.value);
}

namespace
{
    inline float knobToLevelDb (float knob) noexcept { return -24.0f + knob * 3.0f; } // 8 = unity, 10 = +6 dB
    inline float expMap (float norm, float lo, float hi) noexcept { return lo * std::pow (hi / lo, juce::jlimit (0.0f, 1.0f, norm)); }

    juce::dsp::AudioBlock<float> monoBlock (float*& ptr, int n)
    {
        return juce::dsp::AudioBlock<float> (&ptr, 1, (size_t) n);
    }
}

//==============================================================================
NoiseGate::NoiseGate()
{
    addParam ({ "threshold", "Threshold", -90.0f, -20.0f, -62.0f, "dB" });
    addParam ({ "release", "Release", 20.0f, 500.0f, 120.0f, "ms" });
}

void NoiseGate::prepare (double sampleRate, int) { sr = sampleRate; reset(); }

void NoiseGate::process (float* x, int n) noexcept
{
    const float thr = dbToGain (value (0));
    const float envDecay = std::exp (-1.0f / (float) (0.010 * sr));
    const float openCoef = std::exp (-1.0f / (float) (0.001 * sr));
    const float closeCoef = std::exp (-1.0f / (float) (value (1) * 0.001 * sr));

    for (int i = 0; i < n; ++i)
    {
        const float a = std::abs (x[i]);
        env = a > env ? a : env * envDecay;
        const float target = env > thr ? 1.0f : 0.0f;
        const float c = target > gain ? openCoef : closeCoef;
        gain = target + c * (gain - target);
        x[i] *= gain;
    }
}

//==============================================================================
CompressorFx::CompressorFx()
{
    addParam ({ "sustain", "Sustain", 0.0f, 10.0f, 5.0f });
    addParam ({ "attack", "Attack", 1.0f, 50.0f, 10.0f, "ms" });
    addParam ({ "level", "Level", -12.0f, 12.0f, 0.0f, "dB" });
}

void CompressorFx::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    comp.prepare ({ sampleRate, (juce::uint32) maxBlock, 1 });
    level.reset (sampleRate, 0.05);
}

void CompressorFx::process (float* x, int n) noexcept
{
    const float sustain = value (0);
    comp.setThreshold (-8.0f - sustain * 3.5f);
    comp.setRatio (2.0f + sustain * 0.8f);
    comp.setAttack (value (1));
    comp.setRelease (180.0f);

    auto block = monoBlock (x, n);
    comp.process (juce::dsp::ProcessContextReplacing<float> (block));

    // Make-up gain follows the amount of squash
    level.setTargetValue (dbToGain (value (2) + sustain * 1.6f));
    for (int i = 0; i < n; ++i) x[i] *= level.getNextValue();
}

//==============================================================================
Boost::Boost()
{
    addParam ({ "gain", "Gain", 0.0f, 20.0f, 8.0f, "dB" });
    addParam ({ "bright", "Bright", 0.0f, 10.0f, 3.0f });
}

void Boost::prepare (double sampleRate, int)
{
    sr = sampleRate; lastBright = -1; tilt.reset();
    gain.reset (sampleRate, 0.05);
    gain.setCurrentAndTargetValue (dbToGain (value (0)));
}

void Boost::process (float* x, int n) noexcept
{
    if (value (1) != lastBright)
    {
        lastBright = value (1);
        tilt.set (Biquad::Type::highShelf, sr, 2200.0, 0.7, lastBright * 0.8);
    }
    gain.setTargetValue (dbToGain (value (0)));
    for (int i = 0; i < n; ++i)
        x[i] = tilt.process (x[i]) * gain.getNextValue();
}

//==============================================================================
Drive::Drive (Flavour f) : flavour (f)
{
    addParam ({ "drive", flavour == Flavour::fuzz ? "Fuzz" : (flavour == Flavour::distortion ? "Dist" : "Drive"), 0.0f, 10.0f,
                flavour == Flavour::overdrive ? 5.0f : 6.0f });
    addParam ({ "tone", "Tone", 0.0f, 10.0f, 5.0f });
    addParam ({ "level", "Level", 0.0f, 10.0f, 7.0f });
}

juce::String Drive::typeId() const
{
    switch (flavour)
    {
        case Flavour::overdrive:  return "overdrive";
        case Flavour::distortion: return "distortion";
        case Flavour::fuzz:       return "fuzz";
    }
    return {};
}

void Drive::prepare (double sampleRate, int)
{
    sr = sampleRate;
    reset();
    switch (flavour)
    {
        case Flavour::overdrive:  preHp.set (Biquad::Type::highPass, sr, 380.0, 0.6);
                                  midHump.set (Biquad::Type::peak, sr, 720.0, 0.8, 4.0); break;
        case Flavour::distortion: preHp.set (Biquad::Type::highPass, sr, 140.0, 0.7);
                                  midHump.set (Biquad::Type::peak, sr, 1000.0, 0.7, 2.0); break;
        case Flavour::fuzz:       preHp.set (Biquad::Type::highPass, sr, 60.0, 0.7);
                                  midHump.set (Biquad::Type::peak, sr, 200.0, 0.7, 3.0); break;
    }
    dcBlock.set (Biquad::Type::highPass, sr, 25.0, 0.7);
    drive.reset (sampleRate, 0.05);
    level.reset (sampleRate, 0.05);
    lastTone = -1;
}

void Drive::reset()
{
    preHp.reset(); midHump.reset(); toneLp.reset(); toneHp.reset(); dcBlock.reset();
}

void Drive::process (float* x, int n) noexcept
{
    const float d = value (0) / 10.0f;
    const float tone = value (1) / 10.0f;

    if (tone != lastTone)
    {
        lastTone = tone;
        if (flavour == Flavour::fuzz)
        {
            toneLp.set (Biquad::Type::lowPass, sr, 650.0, 0.6);
            toneHp.set (Biquad::Type::highPass, sr, 1100.0, 0.6);
        }
        else
        {
            toneLp.set (Biquad::Type::lowPass, sr, expMap (tone, 900.0f, 9000.0f), 0.65);
        }
    }

    float gainDb = 0.0f;
    switch (flavour)
    {
        case Flavour::overdrive:  gainDb = 6.0f + d * 36.0f; break;
        case Flavour::distortion: gainDb = 14.0f + d * 42.0f; break;
        case Flavour::fuzz:       gainDb = 20.0f + d * 40.0f; break;
    }
    drive.setTargetValue (dbToGain (gainDb));
    level.setTargetValue (dbToGain (knobToLevelDb (value (2))) * 0.35f);

    for (int i = 0; i < n; ++i)
    {
        const float in = x[i];
        float s = midHump.process (preHp.process (in)) * drive.getNextValue();
        float y = 0.0f;

        switch (flavour)
        {
            case Flavour::overdrive:
            {
                // Asymmetric soft clip with some clean signal blended in (classic green-pedal recipe)
                const float c = s >= 0.0f ? std::tanh (s) : std::tanh (s * 1.25f) * 0.8f;
                y = toneLp.process (c + in * 0.6f);
                break;
            }
            case Flavour::distortion:
            {
                const float c = juce::jlimit (-1.5f, 1.5f, s);
                y = toneLp.process (c - (c * c * c) / 6.75f);   // cubic knee into hard ceiling
                break;
            }
            case Flavour::fuzz:
            {
                const float bias = 0.35f;
                float c = std::tanh (s + bias) - std::tanh (bias);
                c = juce::jlimit (-0.8f, 0.8f, c * 1.6f);
                const float lp = toneLp.process (c), hp = toneHp.process (c);
                y = lp * (1.0f - tone) + hp * tone * 1.4f;
                break;
            }
        }
        x[i] = dcBlock.process (y) * level.getNextValue();
    }
}

//==============================================================================
Modulation::Modulation (bool flanger) : isFlanger (flanger)
{
    if (isFlanger)
    {
        addParam ({ "rate", "Rate", 0.05f, 2.0f, 0.25f, "Hz" });
        addParam ({ "depth", "Depth", 0.0f, 1.0f, 0.7f, "%" });
        addParam ({ "feedback", "Regen", 0.0f, 0.9f, 0.55f, "%" });
        addParam ({ "mix", "Mix", 0.0f, 1.0f, 0.5f, "%" });
    }
    else
    {
        addParam ({ "rate", "Rate", 0.1f, 5.0f, 0.8f, "Hz" });
        addParam ({ "depth", "Depth", 0.0f, 1.0f, 0.4f, "%" });
        addParam ({ "mix", "Mix", 0.0f, 1.0f, 0.5f, "%" });
    }
}

void Modulation::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    chorus.prepare ({ sampleRate, (juce::uint32) maxBlock, 1 });
    chorus.setCentreDelay (isFlanger ? 2.0f : 8.0f);
}

void Modulation::process (float* x, int n) noexcept
{
    chorus.setRate (value (0));
    chorus.setDepth (value (1) * (isFlanger ? 1.0f : 0.6f));
    if (isFlanger)
    {
        chorus.setFeedback (value (2));
        chorus.setMix (value (3));
    }
    else
    {
        chorus.setFeedback (0.0f);
        chorus.setMix (value (2));
    }
    auto block = monoBlock (x, n);
    chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
}

//==============================================================================
PhaserFx::PhaserFx()
{
    addParam ({ "rate", "Rate", 0.05f, 4.0f, 0.4f, "Hz" });
    addParam ({ "depth", "Depth", 0.0f, 1.0f, 0.6f, "%" });
    addParam ({ "feedback", "Feedback", 0.0f, 0.9f, 0.4f, "%" });
    addParam ({ "mix", "Mix", 0.0f, 1.0f, 0.5f, "%" });
}

void PhaserFx::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    phaser.prepare ({ sampleRate, (juce::uint32) maxBlock, 1 });
    phaser.setCentreFrequency (900.0f);
}

void PhaserFx::process (float* x, int n) noexcept
{
    phaser.setRate (value (0));
    phaser.setDepth (value (1));
    phaser.setFeedback (value (2));
    phaser.setMix (value (3));
    auto block = monoBlock (x, n);
    phaser.process (juce::dsp::ProcessContextReplacing<float> (block));
}

//==============================================================================
Tremolo::Tremolo()
{
    addParam ({ "rate", "Speed", 1.0f, 12.0f, 5.0f, "Hz" });
    addParam ({ "depth", "Depth", 0.0f, 1.0f, 0.6f, "%" });
    addParam ({ "shape", "Shape", 0.0f, 1.0f, 0.25f, "%" });
}

void Tremolo::prepare (double sampleRate, int) { sr = sampleRate; phase = 0; }

void Tremolo::process (float* x, int n) noexcept
{
    const double inc = value (0) / sr;
    const float depth = value (1), shape = value (2);
    const float k = 1.0f + shape * 12.0f; // sine -> near-square

    for (int i = 0; i < n; ++i)
    {
        const float s = (float) std::sin (juce::MathConstants<double>::twoPi * phase);
        const float lfo = std::tanh (s * k) / std::tanh (k);           // -1..1
        const float g = 1.0f - depth * (0.5f - 0.5f * lfo);
        x[i] *= g;
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
    }
}

//==============================================================================
DelayFx::DelayFx()
{
    addParam ({ "time", "Time", 40.0f, 1200.0f, 380.0f, "ms" });
    addParam ({ "feedback", "Repeats", 0.0f, 0.95f, 0.35f, "%" });
    addParam ({ "mix", "Mix", 0.0f, 1.0f, 0.3f, "%" });
    addParam ({ "tone", "Tone", 0.0f, 10.0f, 6.0f });
}

void DelayFx::prepare (double sampleRate, int)
{
    sr = sampleRate;
    buffer.assign ((size_t) (sampleRate * 1.3) + 4, 0.0f);
    lowCut.set (Biquad::Type::highPass, sr, 90.0, 0.7);
    reset();
}

void DelayFx::reset()
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    writePos = 0;
    smoothedDelay = (float) (value (0) * 0.001 * sr);
    damp.reset(); lowCut.reset();
    lastTone = -1;
}

void DelayFx::process (float* x, int n) noexcept
{
    if (value (3) != lastTone)
    {
        lastTone = value (3);
        damp.set (Biquad::Type::lowPass, sr, expMap (lastTone / 10.0f, 1200.0f, 14000.0f), 0.6);
    }

    const int size = (int) buffer.size();
    const float target = juce::jlimit (1.0f, (float) size - 3.0f, (float) (value (0) * 0.001 * sr));
    const float fb = value (1), mix = value (2);
    const float smooth = 1.0f - std::exp (-1.0f / (float) (0.08 * sr));

    for (int i = 0; i < n; ++i)
    {
        smoothedDelay += smooth * (target - smoothedDelay);
        float readPos = (float) writePos - smoothedDelay;
        if (readPos < 0) readPos += (float) size;
        const int i0 = (int) readPos;
        const int i1 = (i0 + 1) % size;
        const float frac = readPos - (float) i0;
        const float delayed = buffer[(size_t) i0] + frac * (buffer[(size_t) i1] - buffer[(size_t) i0]);

        const float fbSig = lowCut.process (damp.process (delayed));
        buffer[(size_t) writePos] = x[i] + std::tanh (fbSig * fb);   // soft-limit runaway feedback
        writePos = (writePos + 1) % size;

        x[i] = x[i] + delayed * mix;
    }
}

//==============================================================================
ReverbFx::ReverbFx()
{
    addParam ({ "size", "Size", 0.0f, 1.0f, 0.55f, "%" });
    addParam ({ "tone", "Tone", 0.0f, 10.0f, 6.0f });
    addParam ({ "mix", "Mix", 0.0f, 1.0f, 0.3f, "%" });
}

void ReverbFx::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    reverb.setSampleRate (sampleRate);
    wet.assign ((size_t) juce::jmax (maxBlock, 512), 0.0f);
    reverb.reset();
}

void ReverbFx::process (float* x, int n) noexcept
{
    juce::Reverb::Parameters p;
    p.roomSize = 0.3f + value (0) * 0.68f;
    p.damping = 1.0f - value (1) / 10.0f;
    p.wetLevel = 1.0f;
    p.dryLevel = 0.0f;
    p.width = 0.0f;
    reverb.setParameters (p);

    const float mix = value (2);
    const int chunk = (int) wet.size();
    for (int start = 0; start < n; start += chunk)
    {
        const int len = juce::jmin (chunk, n - start);
        std::copy (x + start, x + start + len, wet.data());
        reverb.processMono (wet.data(), len);
        for (int i = 0; i < len; ++i)
            x[start + i] = x[start + i] * (1.0f - 0.45f * mix) + wet[(size_t) i] * mix * 0.35f;
    }
}

//==============================================================================
AutoWah::AutoWah()
{
    addParam ({ "sens", "Sens", 0.0f, 10.0f, 6.0f });
    addParam ({ "freq", "Range", 0.0f, 10.0f, 4.0f });
    addParam ({ "q", "Peak", 0.0f, 10.0f, 6.0f });
    addParam ({ "mix", "Mix", 0.0f, 1.0f, 0.85f, "%" });
}

void AutoWah::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    filter.prepare ({ sampleRate, (juce::uint32) maxBlock, 1 });
    filter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
    reset();
}

void AutoWah::process (float* x, int n) noexcept
{
    const float sens = value (0) / 10.0f;
    const float base = expMap (value (1) / 10.0f, 250.0f, 900.0f);
    const float q = 1.0f + value (2) * 0.9f;
    const float mix = value (3);
    filter.setResonance (q);

    const float att = 1.0f - std::exp (-1.0f / (float) (0.004 * sr));
    const float rel = 1.0f - std::exp (-1.0f / (float) (0.12 * sr));

    for (int i = 0; i < n; ++i)
    {
        const float a = std::abs (x[i]);
        env += (a > env ? att : rel) * (a - env);

        if ((i & 15) == 0)
        {
            const float sweep = juce::jlimit (0.0f, 1.0f, env * (2.0f + sens * 30.0f));
            filter.setCutoffFrequency (juce::jlimit (100.0f, (float) (sr * 0.45), base * std::pow (2.0f, sweep * 3.3f)));
        }

        const float wet = filter.processSample (0, x[i]) * (0.6f + q * 0.18f);
        x[i] = x[i] * (1.0f - mix) + wet * mix;
    }
}

//==============================================================================
GraphicEq::GraphicEq()
{
    addParam ({ "bass", "Bass", -12.0f, 12.0f, 0.0f, "dB" });
    addParam ({ "mid", "Mid", -12.0f, 12.0f, 0.0f, "dB" });
    addParam ({ "treble", "Treble", -12.0f, 12.0f, 0.0f, "dB" });
    addParam ({ "level", "Level", -12.0f, 12.0f, 0.0f, "dB" });
}

void GraphicEq::prepare (double sampleRate, int) { sr = sampleRate; for (auto& c : cache) c = -99; reset(); }

void GraphicEq::process (float* x, int n) noexcept
{
    if (value (0) != cache[0]) { cache[0] = value (0); low.set (Biquad::Type::lowShelf, sr, 120.0, 0.7, cache[0]); }
    if (value (1) != cache[1]) { cache[1] = value (1); mid.set (Biquad::Type::peak, sr, 750.0, 0.8, cache[1]); }
    if (value (2) != cache[2]) { cache[2] = value (2); high.set (Biquad::Type::highShelf, sr, 3200.0, 0.7, cache[2]); }
    const float g = dbToGain (value (3));
    for (int i = 0; i < n; ++i)
        x[i] = high.process (mid.process (low.process (x[i]))) * g;
}

//==============================================================================
Octaver::Octaver()
{
    addParam ({ "sub", "Sub", 0.0f, 1.0f, 0.6f, "%" });
    addParam ({ "up", "Up", 0.0f, 1.0f, 0.0f, "%" });
    addParam ({ "dry", "Dry", 0.0f, 1.0f, 0.8f, "%" });
}

void Octaver::prepare (double sampleRate, int)
{
    sr = sampleRate;
    inLp.set (Biquad::Type::lowPass, sr, 500.0, 0.7);
    subLp.set (Biquad::Type::lowPass, sr, 380.0, 0.7);
    upHp.set (Biquad::Type::highPass, sr, 150.0, 0.7);
    reset();
}

void Octaver::reset() { inLp.reset(); subLp.reset(); upHp.reset(); flip = false; lastSign = 0; }

void Octaver::process (float* x, int n) noexcept
{
    const float sub = value (0), up = value (1), dry = value (2);
    for (int i = 0; i < n; ++i)
    {
        const float in = x[i];
        const float filtered = inLp.process (in);
        const float sign = filtered >= 0.0f ? 1.0f : -1.0f;
        if (sign > 0.0f && lastSign < 0.0f) flip = ! flip;   // divide-by-two flip-flop on rising zero crossing
        lastSign = sign;

        const float subSig = subLp.process ((flip ? 1.0f : -1.0f) * std::abs (filtered)) * 1.6f;
        const float upSig = upHp.process (std::abs (in)) * 1.4f;
        x[i] = in * dry + subSig * sub + upSig * up;
    }
}

//==============================================================================
namespace PedalRegistry
{
    const std::vector<PedalInfo>& all()
    {
        static const std::vector<PedalInfo> list {
            { "comp", "SQUEEZE", "Compressor", "Evens out your picking and adds sustain.", 0xff2f6fb0,
              [] { return std::make_unique<CompressorFx>(); } },
            { "boost", "LIFT", "Clean boost", "Pushes the amp harder without changing your tone.", 0xffc9c2b0,
              [] { return std::make_unique<Boost>(); } },
            { "wah", "QUACK", "Auto-wah", "Envelope filter that sweeps as you dig in.", 0xff9a6a2f,
              [] { return std::make_unique<AutoWah>(); } },
            { "octave", "SUBTERRA", "Octaver", "Adds a note an octave below or above.", 0xff4b5a3a,
              [] { return std::make_unique<Octaver>(); } },
            { "overdrive", "GREENHOUSE", "Overdrive", "Warm, mid-forward breakup for blues and rock.", 0xff3c9a64,
              [] { return std::make_unique<Drive> (Drive::Flavour::overdrive); } },
            { "distortion", "HOTPLATE", "Distortion", "Tight, aggressive clipping for riffs.", 0xffe3a32b,
              [] { return std::make_unique<Drive> (Drive::Flavour::distortion); } },
            { "fuzz", "BIG FUZZ", "Fuzz", "Thick, woolly sustain with a scooped tone.", 0xffb23a48,
              [] { return std::make_unique<Drive> (Drive::Flavour::fuzz); } },
            { "eq", "SHAPER", "EQ", "Bass, mid and treble, plus output level.", 0xff98a1a8,
              [] { return std::make_unique<GraphicEq>(); } },
            { "chorus", "SHIMMER", "Chorus", "Lush doubled shimmer.", 0xff52b0c4,
              [] { return std::make_unique<Modulation> (false); } },
            { "flanger", "JETSTREAM", "Flanger", "Swooshing jet-plane sweeps.", 0xff7a5fb5,
              [] { return std::make_unique<Modulation> (true); } },
            { "phaser", "SWIRL", "Phaser", "Rotating, vocal phase swirl.", 0xffe06a2c,
              [] { return std::make_unique<PhaserFx>(); } },
            { "tremolo", "PULSE", "Tremolo", "Rhythmic volume throb, from vintage to choppy.", 0xffd06c95,
              [] { return std::make_unique<Tremolo>(); } },
            { "delay", "ECHO DECK", "Delay", "Repeats that sit behind your playing.", 0xffe9e5da,
              [] { return std::make_unique<DelayFx>(); } },
            { "reverb", "CATHEDRAL", "Reverb", "Room, hall and beyond.", 0xff26406a,
              [] { return std::make_unique<ReverbFx>(); } },
            { "gate", "HUSH", "Noise gate", "Silences hum and hiss between notes.", 0xff3d434c,
              [] { return std::make_unique<NoiseGate>(); } },
        };
        return list;
    }

    const PedalInfo* find (const juce::String& typeId)
    {
        for (auto& p : all())
            if (p.typeId == typeId) return &p;
        return nullptr;
    }

    std::unique_ptr<Effect> create (const juce::String& typeId)
    {
        if (auto* info = find (typeId))
            return info->create();
        return {};
    }
}

} // namespace gg
