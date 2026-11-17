#include "AmpSim.h"

namespace gg
{

const std::vector<AmpModel>& AmpSim::models()
{
    static const std::vector<AmpModel> list {
        { "clean",  "Glass Clean",    "Sparkling, uncompressed clean with lots of headroom.",
          -4.0f, 22.0f, 1, 0.10f,  70.0f, 450.0f, -4.0f, 3.0f, 0.15f,  2.0f, 0xffd9d2c3 },
        { "tweed",  "Tweed Breakup",  "Fat low-wattage grind that cleans up with your volume knob.",
          4.0f, 34.0f, 2, 0.35f,  90.0f, 700.0f,  1.0f, 1.0f, 0.55f, -1.0f, 0xffc9a467 },
        { "crunch", "Brit Crunch",    "Chimey, punchy crunch for chords and classic rock.",
          8.0f, 40.0f, 2, 0.20f, 100.0f, 820.0f,  2.0f, 2.0f, 0.30f, -3.0f, 0xff2e2a26 },
        { "lead",   "Plexi Lead",     "Singing, mid-heavy lead tone with long sustain.",
          16.0f, 48.0f, 3, 0.25f, 110.0f, 760.0f,  3.0f, 2.5f, 0.30f, -5.0f, 0xff7d6233 },
        { "higain", "Modern Hi-Gain", "Tight, saturated and scooped for heavy riffs.",
          24.0f, 60.0f, 3, 0.15f, 140.0f, 550.0f, -5.0f, 1.0f, 0.12f, -6.0f, 0xff34383e },
    };
    return list;
}

const std::vector<CabModel>& AmpSim::cabs()
{
    static const std::vector<CabModel> list {
        { "1x12", "1x12 Open back",  95.0f, 115.0f, 3.0f, 2600.0f, 3.0f, 6400.0f, 480.0f, -2.0f },
        { "2x12", "2x12 Combo",      85.0f, 105.0f, 4.0f, 2300.0f, 4.0f, 5600.0f, 420.0f, -3.0f },
        { "4x12", "4x12 Closed back", 70.0f, 95.0f, 5.0f, 2000.0f, 5.0f, 4900.0f, 380.0f, -4.0f },
        { "none", "No cabinet (DI)",  20.0f, 100.0f, 0.0f, 2000.0f, 0.0f, 18000.0f, 400.0f, 0.0f },
    };
    return list;
}

AmpSim::AmpSim()
{
    juce::StringArray modelNames, cabNames;
    for (auto& m : models()) modelNames.add (m.name);
    for (auto& c : cabs()) cabNames.add (c.name);

    addParam ({ "model", "Amp", 0.0f, (float) modelNames.size() - 1, 2.0f, "", modelNames });
    addParam ({ "gain", "Gain", 0.0f, 10.0f, 5.0f });
    addParam ({ "bass", "Bass", 0.0f, 10.0f, 5.0f });
    addParam ({ "mid", "Middle", 0.0f, 10.0f, 5.0f });
    addParam ({ "treble", "Treble", 0.0f, 10.0f, 5.5f });
    addParam ({ "presence", "Presence", 0.0f, 10.0f, 5.0f });
    addParam ({ "master", "Master", 0.0f, 10.0f, 6.0f });
    addParam ({ "cab", "Cabinet", 0.0f, (float) cabNames.size() - 1, 2.0f, "", cabNames });
    addParam ({ "mic", "Mic", 0.0f, 1.0f, 0.3f, "%" });
    addParam ({ "room", "Room", 0.0f, 1.0f, 0.15f, "%" });
    for (auto& c : cache) c = -999.0f;
}

void AmpSim::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false);
    oversampler->initProcessing ((size_t) maxBlockSize);
    osRate = sampleRate * 4.0;

    room.setSampleRate (sampleRate);
    roomBuf.assign ((size_t) maxBlockSize, 0.0f);

    preGain.reset (sampleRate, 0.05);
    master.reset (sampleRate, 0.05);
    for (auto& c : cache) c = -999.0f;
    currentModel = -1;
    updateFilters();
    preGain.setCurrentAndTargetValue (preGain.getTargetValue());
    master.setCurrentAndTargetValue (master.getTargetValue());
    reset();
}

void AmpSim::reset()
{
    if (oversampler) oversampler->reset();
    for (auto* b : { &inputHp, &brightShelf, &bass, &mid, &treble, &presence, &voicing,
                     &cabLowCut, &cabRes, &cabDip, &cabPresence, &cabHighCut1, &cabHighCut2 })
        b->reset();
    for (int i = 0; i < 3; ++i) { interStageHp[i].reset(); interStageLp[i].reset(); }
    room.reset();
    sagEnv = 0.0f;
}

void AmpSim::updateFilters()
{
    bool changed = false;
    for (int i = 0; i < 10; ++i)
        if (value (i) != cache[i]) { cache[i] = value (i); changed = true; }
    if (! changed) return;

    const int modelIdx = juce::jlimit (0, (int) models().size() - 1, (int) std::lround (cache[pModel]));
    const auto& m = models()[(size_t) modelIdx];
    const auto& cab = cabs()[(size_t) juce::jlimit (0, (int) cabs().size() - 1, (int) std::lround (cache[pCab]))];

    if (modelIdx != currentModel)
    {
        currentModel = modelIdx;
        for (int i = 0; i < 3; ++i)
        {
            interStageHp[i].set (Biquad::Type::highPass, osRate, 25.0 + 15.0 * i, 0.7);
            interStageLp[i].set (Biquad::Type::lowPass, osRate, 11000.0 - 1500.0 * i, 0.6);
        }
    }

    inputHp.set (Biquad::Type::highPass, sr, m.lowCutHz, 0.7);
    // Bright cap: more top end at low gain, like a real amp's bright switch
    brightShelf.set (Biquad::Type::highShelf, sr, 1800.0, 0.7, m.brightDb * (1.0f - cache[pGain] / 14.0f));

    bass.set (Biquad::Type::lowShelf, sr, 140.0, 0.7, (cache[pBass] - 5.0f) * 2.6f);
    mid.set (Biquad::Type::peak, sr, m.midHz, 0.7, (cache[pMid] - 5.0f) * 2.4f);
    treble.set (Biquad::Type::highShelf, sr, 2200.0, 0.7, (cache[pTreble] - 5.0f) * 2.6f);
    presence.set (Biquad::Type::highShelf, sr, 4500.0, 0.7, (cache[pPresence] - 5.0f) * 2.0f);
    voicing.set (Biquad::Type::peak, sr, m.midHz * 0.9, 0.9, m.midScoopDb);

    const float mic = cache[pMic];
    cabLowCut.set (Biquad::Type::highPass, sr, cab.lowCut, 0.75);
    cabRes.set (Biquad::Type::peak, sr, cab.resonanceHz, 1.4, cab.resonanceDb);
    cabDip.set (Biquad::Type::peak, sr, cab.boxDipHz, 1.0, cab.boxDipDb);
    cabPresence.set (Biquad::Type::peak, sr, cab.presenceHz, 1.1, cab.presenceDb * (1.0f - 0.7f * mic));
    const double hc = cab.highCut * (1.0 - 0.45 * mic);
    cabHighCut1.set (Biquad::Type::lowPass, sr, hc, 0.6);
    cabHighCut2.set (Biquad::Type::lowPass, sr, hc * 1.15, 0.9);

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.32f; rp.damping = 0.55f; rp.wetLevel = 1.0f; rp.dryLevel = 0.0f; rp.width = 0.0f;
    room.setParameters (rp);

    preGain.setTargetValue (dbToGain (m.minGainDb + (cache[pGain] / 10.0f) * (m.maxGainDb - m.minGainDb)));
    master.setTargetValue (cache[pMaster] / 10.0f);
}

void AmpSim::process (float* x, int n) noexcept
{
    // Process in chunks that fit the oversampler's prepared size
    const int maxChunk = (int) roomBuf.size();
    if (n > maxChunk)
    {
        for (int start = 0; start < n; start += maxChunk)
            process (x + start, juce::jmin (maxChunk, n - start));
        return;
    }

    updateFilters();
    const auto& m = models()[(size_t) juce::jmax (0, currentModel)];
    const bool hasCab = juce::jlimit (0, (int) cabs().size() - 1, (int) std::lround (cache[pCab])) != (int) cabs().size() - 1;

    // 1. Input stage and gain at the base rate
    for (int i = 0; i < n; ++i)
        x[i] = brightShelf.process (inputHp.process (x[i])) * preGain.getNextValue();

    // 2. Clipping stages, oversampled 4x to keep aliasing down
    {
        float* ptr = x;
        juce::dsp::AudioBlock<float> block (&ptr, 1, (size_t) n);
        auto up = oversampler->processSamplesUp (block);
        auto* d = up.getChannelPointer (0);
        const int un = (int) up.getNumSamples();
        const float asym = m.asymmetry;
        const float stageGain = m.stages > 1 ? 3.2f : 1.0f;

        for (int i = 0; i < un; ++i)
        {
            float s = d[i];
            for (int st = 0; st < m.stages; ++st)
            {
                const float in = st == 0 ? s : s * stageGain;
                s = in >= 0.0f ? std::tanh (in) : std::tanh (in * (1.0f + asym)) / (1.0f + asym);
                s = interStageLp[st].process (interStageHp[st].process (s));
            }
            d[i] = s;
        }
        oversampler->processSamplesDown (block);
    }

    // 3. Tone stack, power amp, cabinet
    const float sagAtt = 1.0f - std::exp (-1.0f / (float) (0.015 * sr));
    const float sagRel = 1.0f - std::exp (-1.0f / (float) (0.25 * sr));
    const float trim = dbToGain (m.outputTrimDb);

    for (int i = 0; i < n; ++i)
    {
        float s = voicing.process (presence.process (treble.process (mid.process (bass.process (x[i])))));

        const float mv = master.getNextValue();
        const float drive = 0.6f + mv * 2.4f;
        const float a = std::abs (s);
        sagEnv += (a > sagEnv ? sagAtt : sagRel) * (a - sagEnv);
        const float sagGain = 1.0f - m.sag * juce::jmin (0.5f, sagEnv * 0.5f);
        s = std::tanh (s * drive * sagGain) / std::tanh (drive);

        if (hasCab)
            s = cabHighCut2.process (cabHighCut1.process (cabPresence.process (cabDip.process (cabRes.process (cabLowCut.process (s))))));

        const float volume = mv * mv * 1.4f;
        x[i] = s * volume * trim * 0.5f;
    }

    // 4. Room ambience
    const float roomMix = cache[pRoom];
    if (roomMix > 0.001f)
    {
        std::copy (x, x + n, roomBuf.data());
        room.processMono (roomBuf.data(), n);
        for (int i = 0; i < n; ++i)
            x[i] = x[i] * (1.0f - roomMix * 0.3f) + roomBuf[(size_t) i] * roomMix * 0.3f;
    }
}

} // namespace gg
