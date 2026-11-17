#pragma once

#include "ChordLibrary.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace gg
{

/** Offline Karplus-Strong guitar renderer: used to "hear" chords and as a demo DI input. */
namespace StrumSynth
{
    /** Renders a downstrum of the chord into a mono buffer. */
    juce::AudioBuffer<float> renderChord (const Chord& chord, double sampleRate,
                                          double seconds = 2.8, double strumMs = 18.0);

    /** Renders a single plucked note (MIDI number) into dest at offset, mixing additively. */
    void pluck (juce::AudioBuffer<float>& dest, int channel, int startSample, int midiNote,
                double sampleRate, float velocity, double decaySeconds, juce::Random& rng);

    /** A looping, dry "DI" riff used when no guitar is plugged in. */
    juce::AudioBuffer<float> renderDemoRiff (double sampleRate);
}

} // namespace gg
