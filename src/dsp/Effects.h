#pragma once

#include "Biquad.h"
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>

namespace gg
{

/** A user-facing knob on a pedal or amp. Values are stored in natural units. */
struct ParamSpec
{
    juce::String id;
    juce::String name;
    float min = 0.0f, max = 1.0f, def = 0.5f;
    juce::String unit;              // "", "dB", "ms", "Hz", "%"
    juce::StringArray choices;      // non-empty => stepped choice parameter
};

class Parameter
{
public:
    explicit Parameter (ParamSpec s) : spec (std::move (s)), value (spec.def) {}

    const ParamSpec spec;
    float get() const noexcept { return value.load (std::memory_order_relaxed); }
    void set (float v) noexcept { value.store (juce::jlimit (spec.min, spec.max, v), std::memory_order_relaxed); }

    float getNormalised() const noexcept { return (get() - spec.min) / (spec.max - spec.min); }
    void setNormalised (float n) noexcept { set (spec.min + juce::jlimit (0.0f, 1.0f, n) * (spec.max - spec.min)); }

    juce::String toText() const;

private:
    std::atomic<float> value;
};

/** Base class for every processor in the rig. Processes mono audio in place. */
class Effect
{
public:
    virtual ~Effect() = default;

    virtual juce::String typeId() const = 0;
    virtual void prepare (double sampleRate, int maxBlockSize) = 0;
    virtual void reset() {}
    virtual void process (float* data, int numSamples) noexcept = 0;

    std::vector<std::unique_ptr<Parameter>>& params() noexcept { return parameters; }
    const std::vector<std::unique_ptr<Parameter>>& params() const noexcept { return parameters; }
    Parameter* param (const juce::String& id) const;
    float value (int index) const noexcept { return parameters[(size_t) index]->get(); }

    std::atomic<bool> enabled { true };

    juce::var toVar() const;
    void fromVar (const juce::var& v);

protected:
    void addParam (ParamSpec s) { parameters.push_back (std::make_unique<Parameter> (std::move (s))); }
    double sr = 48000.0;

private:
    std::vector<std::unique_ptr<Parameter>> parameters;
};

/** Metadata for a pedal type shown in the "add pedal" browser. */
struct PedalInfo
{
    juce::String typeId;
    juce::String name;         // printed on the enclosure
    juce::String kind;         // "Overdrive", "Delay" ...
    juce::String blurb;
    juce::uint32 colour;       // enclosure colour (ARGB)
    std::function<std::unique_ptr<Effect>()> create;
};

namespace PedalRegistry
{
    const std::vector<PedalInfo>& all();
    const PedalInfo* find (const juce::String& typeId);
    std::unique_ptr<Effect> create (const juce::String& typeId);
}

//==============================================================================
// Concrete effects (declared here so the amp and tests can use them directly)

class NoiseGate : public Effect
{
public:
    NoiseGate();
    juce::String typeId() const override { return "gate"; }
    void prepare (double, int) override;
    void reset() override { env = 0; gain = 1; }
    void process (float*, int) noexcept override;
private:
    float env = 0, gain = 1;
};

class CompressorFx : public Effect
{
public:
    CompressorFx();
    juce::String typeId() const override { return "comp"; }
    void prepare (double, int) override;
    void reset() override { comp.reset(); }
    void process (float*, int) noexcept override;
private:
    juce::dsp::Compressor<float> comp;
    juce::SmoothedValue<float> level;
};

class Boost : public Effect
{
public:
    Boost();
    juce::String typeId() const override { return "boost"; }
    void prepare (double, int) override;
    void reset() override { tilt.reset(); }
    void process (float*, int) noexcept override;
private:
    Biquad tilt; float lastBright = -1;
    juce::SmoothedValue<float> gain;
};

/** Overdrive, distortion and fuzz share one engine with different curves. */
class Drive : public Effect
{
public:
    enum class Flavour { overdrive, distortion, fuzz };
    explicit Drive (Flavour);
    juce::String typeId() const override;
    void prepare (double, int) override;
    void reset() override;
    void process (float*, int) noexcept override;
private:
    Flavour flavour;
    Biquad preHp, midHump, toneLp, toneHp, dcBlock;
    float lastTone = -1;
    juce::SmoothedValue<float> drive, level;
};

class Modulation : public Effect   // chorus / flanger
{
public:
    explicit Modulation (bool flanger);
    juce::String typeId() const override { return isFlanger ? "flanger" : "chorus"; }
    void prepare (double, int) override;
    void reset() override { chorus.reset(); }
    void process (float*, int) noexcept override;
private:
    bool isFlanger;
    juce::dsp::Chorus<float> chorus;
};

class PhaserFx : public Effect
{
public:
    PhaserFx();
    juce::String typeId() const override { return "phaser"; }
    void prepare (double, int) override;
    void reset() override { phaser.reset(); }
    void process (float*, int) noexcept override;
private:
    juce::dsp::Phaser<float> phaser;
};

class Tremolo : public Effect
{
public:
    Tremolo();
    juce::String typeId() const override { return "tremolo"; }
    void prepare (double, int) override;
    void reset() override { phase = 0; }
    void process (float*, int) noexcept override;
private:
    double phase = 0;
};

class DelayFx : public Effect
{
public:
    DelayFx();
    juce::String typeId() const override { return "delay"; }
    void prepare (double, int) override;
    void reset() override;
    void process (float*, int) noexcept override;
private:
    std::vector<float> buffer;
    int writePos = 0;
    float smoothedDelay = 0;
    Biquad damp, lowCut;
    float lastTone = -1;
};

class ReverbFx : public Effect
{
public:
    ReverbFx();
    juce::String typeId() const override { return "reverb"; }
    void prepare (double, int) override;
    void reset() override { reverb.reset(); }
    void process (float*, int) noexcept override;
private:
    juce::Reverb reverb;
    std::vector<float> wet;
};

class AutoWah : public Effect
{
public:
    AutoWah();
    juce::String typeId() const override { return "wah"; }
    void prepare (double, int) override;
    void reset() override { env = 0; filter.reset(); }
    void process (float*, int) noexcept override;
private:
    float env = 0;
    juce::dsp::StateVariableTPTFilter<float> filter;
};

class GraphicEq : public Effect
{
public:
    GraphicEq();
    juce::String typeId() const override { return "eq"; }
    void prepare (double, int) override;
    void reset() override { low.reset(); mid.reset(); high.reset(); }
    void process (float*, int) noexcept override;
private:
    Biquad low, mid, high;
    float cache[4] { -99, -99, -99, -99 };
};

class Octaver : public Effect
{
public:
    Octaver();
    juce::String typeId() const override { return "octave"; }
    void prepare (double, int) override;
    void reset() override;
    void process (float*, int) noexcept override;
private:
    Biquad inLp, subLp, upHp;
    bool flip = false; float lastSign = 0;
};

} // namespace gg
