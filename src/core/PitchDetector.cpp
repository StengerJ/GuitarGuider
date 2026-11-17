#include "PitchDetector.h"
#include "ChordLibrary.h"

namespace gg
{

PitchDetector::Result PitchDetector::detect (const float* x, int n, double sr, float minHz, float maxHz)
{
    Result r;
    const int maxTau = juce::jmin (n / 2, (int) (sr / minHz) + 2);
    const int minTau = juce::jmax (2, (int) (sr / maxHz));
    if (maxTau <= minTau + 2) return r;

    // Signal level gate
    double energy = 0.0;
    for (int i = 0; i < n; ++i) energy += (double) x[i] * x[i];
    if (energy / n < 1.0e-7) return r;

    const int w = n - maxTau;
    diff.assign ((size_t) maxTau + 1, 0.0f);

    for (int tau = 1; tau <= maxTau; ++tau)
    {
        double sum = 0.0;
        for (int i = 0; i < w; ++i)
        {
            const double d = (double) x[i] - x[i + tau];
            sum += d * d;
        }
        diff[(size_t) tau] = (float) sum;
    }

    // Cumulative mean normalised difference
    diff[0] = 1.0f;
    double running = 0.0;
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        running += diff[(size_t) tau];
        diff[(size_t) tau] = running > 0.0 ? (float) (diff[(size_t) tau] * tau / running) : 1.0f;
    }

    const float threshold = 0.15f;
    int best = -1;
    for (int tau = minTau; tau < maxTau; ++tau)
    {
        if (diff[(size_t) tau] < threshold)
        {
            while (tau + 1 < maxTau && diff[(size_t) tau + 1] < diff[(size_t) tau]) ++tau;
            best = tau;
            break;
        }
    }

    if (best < 0)
    {
        // Fallback: global minimum, but only trust it if reasonably periodic
        float m = 1.0f;
        for (int tau = minTau; tau < maxTau; ++tau)
            if (diff[(size_t) tau] < m) { m = diff[(size_t) tau]; best = tau; }
        if (m > 0.35f) return r;
    }

    // Parabolic interpolation
    float betterTau = (float) best;
    if (best > 1 && best < maxTau)
    {
        const float s0 = diff[(size_t) best - 1], s1 = diff[(size_t) best], s2 = diff[(size_t) best + 1];
        const float denom = 2.0f * (2.0f * s1 - s2 - s0);
        if (std::abs (denom) > 1.0e-9f)
            betterTau = best + (s2 - s0) / denom;
    }

    r.frequency = (float) (sr / betterTau);
    r.confidence = juce::jlimit (0.0f, 1.0f, 1.0f - diff[(size_t) best]);
    return r;
}

PitchDetector::Note PitchDetector::toNote (float f, float a4)
{
    Note note;
    if (f <= 0.0f) return note;
    const float midiF = 69.0f + 12.0f * std::log2 (f / a4);
    note.midi = (int) std::lround (midiF);
    note.cents = (midiF - (float) note.midi) * 100.0f;
    note.name = pitchClassName (note.midi % 12);
    note.octave = note.midi / 12 - 1;
    return note;
}

} // namespace gg
