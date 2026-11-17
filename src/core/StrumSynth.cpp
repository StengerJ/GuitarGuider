#include "StrumSynth.h"

namespace gg::StrumSynth
{

void pluck (juce::AudioBuffer<float>& dest, int channel, int startSample, int midiNote,
            double sampleRate, float velocity, double decaySeconds, juce::Random& rng)
{
    const double freq = juce::MidiMessage::getMidiNoteInHertz (midiNote);
    // The two-point averaging filter adds half a sample of delay; the allpass supplies the fraction.
    const double period = sampleRate / freq - 0.5;
    const int n = juce::jmax (2, (int) std::floor (period - 0.1));
    const float frac = (float) (period - n);           // in [0.1, 1.1): keeps the allpass stable
    const float apC = (1.0f - frac) / (1.0f + frac);

    std::vector<float> line ((size_t) n);
    // Excitation: filtered noise burst (softer, more like a pick on a string)
    float lp = 0.0f;
    for (auto& s : line)
    {
        lp += 0.55f * ((rng.nextFloat() * 2.0f - 1.0f) - lp);
        s = lp;
    }
    // Remove DC from the excitation
    float mean = 0.0f;
    for (auto s : line) mean += s;
    mean /= (float) n;
    for (auto& s : line) s -= mean;

    // Loop gain so the note decays ~60 dB over decaySeconds
    const double loopsPerSec = freq;
    const float g = (float) std::pow (10.0, -3.0 / (loopsPerSec * decaySeconds));

    auto* out = dest.getWritePointer (channel);
    const int total = dest.getNumSamples();
    int idx = 0;
    float prev = 0.0f, apState = 0.0f, apIn = 0.0f;
    const float bright = 0.5f; // averaging filter weight

    for (int i = startSample; i < total; ++i)
    {
        const float cur = line[(size_t) idx];
        float filtered = g * (bright * cur + (1.0f - bright) * prev);
        prev = cur;

        // first-order allpass for fine tuning
        float ap = apC * filtered + apIn - apC * apState;
        apIn = filtered;
        apState = ap;

        line[(size_t) idx] = ap;
        if (i >= 0)
            out[i] += velocity * cur;
        idx = (idx + 1) % n;
    }
}

juce::AudioBuffer<float> renderChord (const Chord& chord, double sampleRate, double seconds, double strumMs)
{
    const int total = (int) (sampleRate * seconds);
    juce::AudioBuffer<float> buf (1, total);
    buf.clear();

    juce::Random rng (chord.id.hashCode64());
    int stringIdx = 0;
    for (size_t s = 0; s < 6; ++s)
    {
        if (chord.frets[s] < 0) continue;
        const int start = (int) (sampleRate * (strumMs / 1000.0) * stringIdx);
        const float vel = 0.22f * (0.85f + 0.15f * rng.nextFloat());
        pluck (buf, 0, start, standardTuning[s] + chord.frets[s], sampleRate, vel, 2.2, rng);
        ++stringIdx;
    }

    // Gentle fade out at the end and normalise to a comfortable peak
    buf.applyGainRamp (0, (int) (total * 0.8), (int) (total * 0.2), 1.0f, 0.0f);
    auto peak = buf.getMagnitude (0, total);
    if (peak > 0.0f)
        buf.applyGain (0.5f / peak);
    return buf;
}

juce::AudioBuffer<float> renderDemoRiff (double sampleRate)
{
    // A simple 2-bar blues-rock riff at 100 BPM in E, as single notes and a couple of double-stops.
    const double bpm = 100.0;
    const double eighth = 60.0 / bpm / 2.0;
    struct Ev { double beat8; std::vector<int> notes; double len; };
    const std::vector<Ev> riff {
        { 0, { 40, 47 }, 1 }, { 1, { 40, 47 }, 1 }, { 2, { 40, 49 }, 1 }, { 3, { 40, 47 }, 1 },
        { 4, { 40, 50 }, 1 }, { 5, { 40, 49 }, 1 }, { 6, { 40, 47 }, 1 }, { 7, { 43 }, 1 },
        { 8, { 45, 52 }, 1 }, { 9, { 45, 52 }, 1 }, { 10, { 45, 54 }, 1 }, { 11, { 45, 52 }, 1 },
        { 12, { 52, 55, 59, 64 }, 4 },
    };

    const int total = (int) (sampleRate * eighth * 16);
    juce::AudioBuffer<float> buf (1, total);
    buf.clear();
    juce::Random rng (1234);

    for (auto& e : riff)
    {
        const int start = (int) (sampleRate * eighth * e.beat8);
        int k = 0;
        for (auto n : e.notes)
        {
            // Render into a temp buffer so we can cut the note off (palm-mute style)
            const int len = juce::jmin (total - start, (int) (sampleRate * eighth * e.len * 1.05));
            juce::AudioBuffer<float> tmp (1, len);
            tmp.clear();
            pluck (tmp, 0, k * (int) (sampleRate * 0.008), n, sampleRate, 0.3f, e.len > 2 ? 2.0 : 0.6, rng);
            tmp.applyGainRamp (0, juce::jmax (0, len - 256), juce::jmin (256, len), 1.0f, 0.0f);
            buf.addFrom (0, start, tmp, 0, 0, len);
            ++k;
        }
    }

    auto peak = buf.getMagnitude (0, total);
    if (peak > 0.0f)
        buf.applyGain (0.45f / peak);
    return buf;
}

} // namespace gg::StrumSynth
