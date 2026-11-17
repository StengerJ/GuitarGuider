#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>
#include "dsp/SignalChain.h"
#include "audio/Session.h"

namespace gg
{

/** Owns the audio device and everything that runs in the audio callback:
    guitar input -> amp chain -> monitor, plus multitrack playback, recording,
    metronome and the chord "preview" player. */
class AudioEngine : private juce::AudioIODeviceCallback,
                    private juce::Timer
{
public:
    AudioEngine();
    ~AudioEngine() override;

    void initialise (juce::PropertiesFile& settings);
    void saveState (juce::PropertiesFile& settings);

    juce::AudioDeviceManager deviceManager;
    SignalChain chain;
    Session session;

    // Input ---------------------------------------------------------------
    std::atomic<bool> demoInput { false };      // loop a built-in DI riff instead of the interface input
    std::atomic<int> inputChannel { 0 };
    std::atomic<bool> monitorOn { true };       // hear the live amp
    std::atomic<float> masterVolume { 0.8f };

    float takeInputPeak() noexcept  { return inputPeak.exchange (0.0f); }
    float takeOutputPeak() noexcept { return outputPeak.exchange (0.0f); }

    /** Copies the most recent `n` raw (dry) input samples, for tuner and chord detection. */
    void copyRecentInput (float* dest, int n) const;

    bool hasInputDevice() const;
    juce::String deviceSummary() const;
    double sampleRate() const noexcept { return currentRate.load(); }
    double latencyMs() const;

    // Transport -------------------------------------------------------------
    void play();
    void stop();
    void togglePlay() { isPlaying() ? stop() : play(); }
    /** Starts recording onto trackIndex at the current position (with a one bar count-in if enabled). */
    void record (int trackIndex);
    bool isPlaying() const noexcept { return playing.load(); }
    bool isRecording() const noexcept { return recording.load(); }
    juce::int64 position() const noexcept { return playhead.load(); }
    void setPosition (juce::int64 samples);
    int recordingTrack() const noexcept { return recTrack; }
    juce::int64 recordingStart() const noexcept { return recStart; }
    const std::vector<float>& liveTake() const noexcept { return take; }

    std::atomic<bool> metronomeOn { false };
    std::atomic<bool> countIn { true };
    std::atomic<bool> loopPlayback { false };

    // Chord preview (Learn section) -----------------------------------------
    void playPreview (juce::AudioBuffer<float> buffer);

    std::function<void()> onTransportChanged;

private:
    void audioDeviceIOCallbackWithContext (const float* const* in, int numIn, float* const* out, int numOut,
                                           int numSamples, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void timerCallback() override;

    void finishRecording();
    void renderMetronome (float* l, float* r, int n, juce::int64 pos) noexcept;

    std::atomic<double> currentRate { 48000.0 };
    std::atomic<float> inputPeak { 0.0f }, outputPeak { 0.0f };

    // dry input analysis ring
    std::vector<float> ring;
    std::atomic<int> ringWrite { 0 };

    // scratch
    std::vector<float> mono, left, right;

    // demo riff
    juce::AudioBuffer<float> demoRiff;
    int demoPos = 0;

    // transport
    std::atomic<bool> playing { false }, recording { false };
    std::atomic<juce::int64> playhead { 0 };
    juce::int64 loopEnd = 0;
    int recTrack = -1;
    juce::int64 recStart = 0;
    std::vector<float> take;                   // message thread only
    juce::AbstractFifo recFifo { 1 << 18 };
    std::vector<float> recBuffer;
    std::atomic<bool> recOverflow { false };
    bool wasRecording = false;

    // metronome click state
    float clickPhase = 0.0f, clickEnv = 0.0f, clickFreq = 1000.0f;
    juce::int64 lastBeat = std::numeric_limits<juce::int64>::min();

    // preview player
    juce::SpinLock previewLock;
    juce::AudioBuffer<float> preview;
    int previewPos = 0;
};

} // namespace gg
