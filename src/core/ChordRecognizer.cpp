#include "ChordRecognizer.h"

namespace gg
{

namespace
{
    float cosine (const std::array<float, 12>& a, const std::array<float, 12>& b)
    {
        float dot = 0, na = 0, nb = 0;
        for (size_t i = 0; i < 12; ++i) { dot += a[i] * b[i]; na += a[i] * a[i]; nb += b[i] * b[i]; }
        if (na <= 0 || nb <= 0) return 0.0f;
        return dot / std::sqrt (na * nb);
    }
}

ChordRecognizer::ChordRecognizer()
    : fft (fftOrder),
      window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false)
{
    work.assign ((size_t) fftSize * 2, 0.0f);
}

std::array<float, 12> ChordRecognizer::templateFor (const Chord& chord)
{
    std::array<float, 12> t {};
    for (auto note : chord.midiNotes())
    {
        // Fundamental plus the first few harmonics, which a guitar produces strongly
        static const float harmonicWeights[] { 1.0f, 0.6f, 0.45f, 0.3f, 0.2f };
        for (int h = 1; h <= 5; ++h)
        {
            const int semis = (int) std::lround (12.0 * std::log2 ((double) h));
            t[(size_t) ((note + semis) % 12)] += harmonicWeights[h - 1];
        }
    }
    return t;
}

bool ChordRecognizer::analyse (const float* samples, int numSamples, double sampleRate)
{
    valid = false;
    std::fill (work.begin(), work.end(), 0.0f);

    const int n = juce::jmin (numSamples, fftSize);
    const float* src = samples + (numSamples - n);
    double energy = 0.0;
    for (int i = 0; i < n; ++i)
    {
        work[(size_t) i] = src[i];
        energy += (double) src[i] * src[i];
    }
    if (energy / juce::jmax (1, n) < 2.0e-6)
        return false;

    window.multiplyWithWindowingTable (work.data(), (size_t) fftSize);
    fft.performFrequencyOnlyForwardTransform (work.data(), true);

    chromaVec.fill (0.0f);
    const double binHz = sampleRate / fftSize;
    const int lo = (int) (70.0 / binHz), hi = juce::jmin (fftSize / 2 - 1, (int) (1600.0 / binHz));

    // Spectral peak picking: only local maxima contribute, weighted by compressed magnitude.
    float maxMag = 0.0f;
    for (int k = lo; k <= hi; ++k) maxMag = juce::jmax (maxMag, work[(size_t) k]);
    if (maxMag <= 0.0f) return false;

    for (int k = lo + 1; k < hi; ++k)
    {
        const float m = work[(size_t) k];
        if (m < maxMag * 0.04f || m < work[(size_t) k - 1] || m < work[(size_t) k + 1]) continue;

        const double freq = k * binHz;
        const double midi = 69.0 + 12.0 * std::log2 (freq / 440.0);
        const int pc = ((int) std::lround (midi) % 12 + 12) % 12;
        chromaVec[(size_t) pc] += std::sqrt (m / maxMag);
    }

    valid = true;
    return true;
}

float ChordRecognizer::scoreFor (const Chord& chord) const
{
    if (! valid) return 0.0f;
    return cosine (chromaVec, templateFor (chord));
}

ChordRecognizer::Match ChordRecognizer::best() const
{
    Match m;
    if (! valid) return m;
    for (auto& c : ChordLibrary::get().all())
    {
        auto s = scoreFor (c);
        if (s > m.score) { m.score = s; m.chordId = c.id; }
    }
    return m;
}

bool ChordRecognizer::matches (const Chord& target, float minScore) const
{
    if (! valid) return false;
    const float t = scoreFor (target);
    if (t < minScore) return false;

    // Accept unless an unrelated chord scores clearly higher. Chords whose notes are a subset or
    // superset of the target (G vs Gmaj7, A vs Asus2's open strings) are "compatible": guitar overtones
    // naturally add sevenths and ninths, so they can't be told apart reliably from a strum.
    const int tm = target.pitchClassMask();
    for (auto& c : ChordLibrary::get().all())
    {
        const int cm = c.pitchClassMask();
        const bool related = (cm & tm) == tm || (cm & tm) == cm;
        if (! related && scoreFor (c) > t + 0.03f)
            return false;
    }
    return true;
}

} // namespace gg
