#include "SignalChain.h"

namespace gg
{

SignalChain::SignalChain()
    : ampSim (std::make_unique<AmpSim>()),
      noiseGate (std::make_unique<NoiseGate>())
{
    prepare (48000.0, 512);
}

void SignalChain::prepare (double sampleRate, int maxBlockSize)
{
    const juce::SpinLock::ScopedLockType sl (lock);
    sr = sampleRate;
    maxBlock = juce::jmax (32, maxBlockSize);
    dry.assign ((size_t) maxBlock, 0.0f);
    ampSim->prepare (sr, maxBlock);
    noiseGate->prepare (sr, maxBlock);
    for (auto& s : slots)
        s.fx->prepare (sr, maxBlock);
    inGain.reset (sr, 0.03);
    outGain.reset (sr, 0.03);
    inGain.setCurrentAndTargetValue (dbToGain (inputGainDb.load()));
    outGain.setCurrentAndTargetValue (dbToGain (outputGainDb.load()));
}

void SignalChain::process (float* x, int n) noexcept
{
    const juce::SpinLock::ScopedTryLockType sl (lock);
    if (! sl.isLocked())
    {
        juce::FloatVectorOperations::clear (x, n);
        return;
    }

    for (int start = 0; start < n; start += maxBlock)
        processChunk (x + start, juce::jmin (maxBlock, n - start));
}

void SignalChain::runWithBypass (Effect& fx, float& mix, bool on, float* x, int n) noexcept
{
    const float target = on ? 1.0f : 0.0f;
    if (mix == 0.0f && target == 0.0f)
        return;

    if (mix == 1.0f && target == 1.0f)
    {
        fx.process (x, n);
        return;
    }

    // Crossfade over ~20 ms so toggling a footswitch never clicks
    std::copy (x, x + n, dry.data());
    fx.process (x, n);
    const float step = 1.0f / (float) (0.02 * sr);
    for (int i = 0; i < n; ++i)
    {
        mix = target > mix ? juce::jmin (target, mix + step) : juce::jmax (target, mix - step);
        x[i] = dry[(size_t) i] + (x[i] - dry[(size_t) i]) * mix;
    }
    if (mix == 0.0f) fx.reset();
}

void SignalChain::processChunk (float* x, int n) noexcept
{
    inGain.setTargetValue (dbToGain (inputGainDb.load (std::memory_order_relaxed)));
    outGain.setTargetValue (dbToGain (outputGainDb.load (std::memory_order_relaxed)));

    for (int i = 0; i < n; ++i) x[i] *= inGain.getNextValue();

    runWithBypass (*noiseGate, gateMix, gateEnabled.load (std::memory_order_relaxed), x, n);

    for (auto& s : slots)
        runWithBypass (*s.fx, s.mix, s.fx->enabled.load (std::memory_order_relaxed), x, n);

    runWithBypass (*ampSim, ampMix, ampEnabled.load (std::memory_order_relaxed), x, n);

    for (int i = 0; i < n; ++i)
    {
        float v = x[i] * outGain.getNextValue();
        if (! std::isfinite (v)) v = 0.0f;
        x[i] = juce::jlimit (-1.5f, 1.5f, v);
    }
}

std::shared_ptr<Effect> SignalChain::addPedal (const juce::String& typeId, int index)
{
    std::shared_ptr<Effect> fx (PedalRegistry::create (typeId));
    if (fx == nullptr) return {};

    double rate; int block;
    { const juce::SpinLock::ScopedLockType sl (lock); rate = sr; block = maxBlock; }
    fx->prepare (rate, block);   // allocate outside the lock

    {
        const juce::SpinLock::ScopedLockType sl (lock);
        Slot s { fx, fx->enabled ? 1.0f : 0.0f };
        if (index < 0 || index > (int) slots.size())
            slots.reserve (slots.size() + 1), slots.push_back (std::move (s));
        else
            slots.insert (slots.begin() + index, std::move (s));
    }
    ++structureVersion;
    return fx;
}

void SignalChain::removePedal (int index)
{
    std::shared_ptr<Effect> doomed;   // destroyed after the lock is released
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        if (index < 0 || index >= (int) slots.size()) return;
        doomed = slots[(size_t) index].fx;
        slots.erase (slots.begin() + index);
    }
    ++structureVersion;
}

void SignalChain::movePedal (int from, int to)
{
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        if (from < 0 || from >= (int) slots.size()) return;
        to = juce::jlimit (0, (int) slots.size() - 1, to);
        if (from == to) return;
        auto s = slots[(size_t) from];
        slots.erase (slots.begin() + from);
        slots.insert (slots.begin() + to, s);
    }
    ++structureVersion;
}

void SignalChain::clearPedals()
{
    std::vector<Slot> doomed;
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        doomed.swap (slots);
    }
    ++structureVersion;
}

std::vector<std::shared_ptr<Effect>> SignalChain::pedals() const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    std::vector<std::shared_ptr<Effect>> out;
    for (auto& s : slots) out.push_back (s.fx);
    return out;
}

int SignalChain::numPedals() const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    return (int) slots.size();
}

juce::var SignalChain::toVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("inputGain", inputGainDb.load());
    o->setProperty ("outputGain", outputGainDb.load());
    o->setProperty ("gateOn", gateEnabled.load());
    o->setProperty ("gate", noiseGate->toVar());
    o->setProperty ("ampOn", ampEnabled.load());
    o->setProperty ("amp", ampSim->toVar());

    juce::Array<juce::var> arr;
    for (auto& p : pedals())
        arr.add (p->toVar());
    o->setProperty ("pedals", arr);
    return juce::var (o);
}

void SignalChain::fromVar (const juce::var& v)
{
    if (v.hasProperty ("inputGain"))  inputGainDb = (float) v["inputGain"];
    if (v.hasProperty ("outputGain")) outputGainDb = (float) v["outputGain"];
    if (v.hasProperty ("gateOn"))     gateEnabled = (bool) v["gateOn"];
    if (v.hasProperty ("ampOn"))      ampEnabled = (bool) v["ampOn"];
    noiseGate->fromVar (v["gate"]);
    ampSim->fromVar (v["amp"]);

    // Build the new pedal list off-lock, then swap it in
    double rate; int block;
    { const juce::SpinLock::ScopedLockType sl (lock); rate = sr; block = maxBlock; }

    std::vector<Slot> fresh;
    if (auto* arr = v["pedals"].getArray())
    {
        for (auto& pv : *arr)
        {
            std::shared_ptr<Effect> fx (PedalRegistry::create (pv["type"].toString()));
            if (fx == nullptr) continue;
            fx->fromVar (pv);
            fx->prepare (rate, block);
            fresh.push_back ({ fx, fx->enabled ? 1.0f : 0.0f });
        }
    }

    {
        const juce::SpinLock::ScopedLockType sl (lock);
        fresh.swap (slots);
    }
    ++structureVersion;
}

} // namespace gg
