#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>

namespace gg
{

struct Track
{
    juce::String name;
    juce::uint32 colour = 0xffd9a441;
    std::vector<float> audio;              // mono, session sample rate, starts at sample 0
    std::atomic<float> volume { 0.8f };    // linear 0..1.5
    std::atomic<float> pan { 0.0f };       // -1..1
    std::atomic<bool> mute { false }, solo { false };
};

/** A small multitrack project: mono tracks that start at bar 1. */
class Session
{
public:
    static constexpr int maxTracks = 8;

    Session();

    // Structure (message thread) -----------------------------------------
    int addTrack (const juce::String& name = {});
    void removeTrack (int index);
    int numTracks() const;
    Track* track (int index) const;

    /** Writes (replaces) audio into a track starting at startSample, growing it as needed. */
    void writeAudio (int trackIndex, juce::int64 startSample, const float* data, int numSamples);
    void clearTrack (int trackIndex);

    juce::int64 lengthSamples() const;
    double sampleRate() const noexcept { return rate; }
    void setSampleRate (double newRate);   // resamples existing audio

    std::atomic<double> bpm { 100.0 };
    std::atomic<int> beatsPerBar { 4 };

    // Audio thread ----------------------------------------------------------
    /** Adds the mix of all tracks at `position` into L/R. Never blocks; skips if busy. */
    void mixInto (float* left, float* right, int numSamples, juce::int64 position) const noexcept;

    // Offline -------------------------------------------------------------------
    juce::AudioBuffer<float> renderMix() const;
    bool exportWav (const juce::File& file) const;
    bool importAudioFile (int trackIndex, const juce::File& file, juce::AudioFormatManager& formats);

    std::atomic<int> version { 0 };   // bumped on any audio/structure change

private:
    mutable juce::SpinLock lock;
    std::vector<std::unique_ptr<Track>> tracks;
    double rate = 48000.0;
    int nameCounter = 0;

    static void resample (std::vector<float>& audio, double ratio);
};

} // namespace gg
