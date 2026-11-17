#pragma once

#include "AmpSim.h"

namespace gg
{

/** Input -> gate -> pedalboard (user ordered) -> amp/cab -> output.
    Structural edits happen on the message thread; the audio thread never blocks
    (it skips a block if an edit is mid-swap). */
class SignalChain
{
public:
    SignalChain();

    void prepare (double sampleRate, int maxBlockSize);
    void process (float* mono, int numSamples) noexcept;

    // Pedalboard editing (message thread) ---------------------------------
    std::shared_ptr<Effect> addPedal (const juce::String& typeId, int index = -1);
    void removePedal (int index);
    void movePedal (int from, int to);
    void clearPedals();
    std::vector<std::shared_ptr<Effect>> pedals() const;
    int numPedals() const;

    AmpSim& amp() noexcept { return *ampSim; }
    NoiseGate& gate() noexcept { return *noiseGate; }

    std::atomic<float> inputGainDb { 0.0f };
    std::atomic<float> outputGainDb { 0.0f };
    std::atomic<bool> ampEnabled { true };
    std::atomic<bool> gateEnabled { true };

    // Presets ---------------------------------------------------------------
    juce::var toVar() const;
    void fromVar (const juce::var& v);

    /** Incremented on every structural change so views know to rebuild. */
    std::atomic<int> structureVersion { 0 };

private:
    struct Slot { std::shared_ptr<Effect> fx; float mix = 1.0f; };

    mutable juce::SpinLock lock;
    std::vector<Slot> slots;
    std::unique_ptr<AmpSim> ampSim;
    std::unique_ptr<NoiseGate> noiseGate;
    std::vector<float> dry;
    double sr = 48000.0;
    int maxBlock = 512;
    float ampMix = 1.0f, gateMix = 1.0f;
    juce::SmoothedValue<float> inGain, outGain;

    void processChunk (float* x, int n) noexcept;
    void runWithBypass (Effect& fx, float& mix, bool on, float* x, int n) noexcept;
};

} // namespace gg
