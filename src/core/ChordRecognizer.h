#pragma once

#include "ChordLibrary.h"
#include <juce_dsp/juce_dsp.h>

namespace gg
{

/** Chroma-based chord recognizer used by "Play along" mode.
    Feed it a block of guitar audio; it scores every library chord by cosine similarity. */
class ChordRecognizer
{
public:
    static constexpr int fftOrder = 13;              // 8192-point FFT
    static constexpr int fftSize = 1 << fftOrder;

    ChordRecognizer();

    struct Match { juce::String chordId; float score = 0.0f; };

    /** Analyses the most recent fftSize samples. Returns false if the input is too quiet. */
    bool analyse (const float* samples, int numSamples, double sampleRate);

    const std::array<float, 12>& chroma() const noexcept { return chromaVec; }

    /** Similarity (0..1) of the last analysis to the given chord. */
    float scoreFor (const Chord& chord) const;

    /** Best matching library chord, considering only chords whose symbol differs from others' pitch sets. */
    Match best() const;

    /** Is `target` the best (or tied-best by pitch-class set) match with a decent score? */
    bool matches (const Chord& target, float minScore = 0.72f) const;

    static std::array<float, 12> templateFor (const Chord& chord);

private:
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> work;
    std::array<float, 12> chromaVec {};
    bool valid = false;
};

} // namespace gg
