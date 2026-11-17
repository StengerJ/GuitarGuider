#pragma once

#include "Effects.h"

namespace gg
{

/** Description of one amp voicing. */
struct AmpModel
{
    juce::String id;
    juce::String name;
    juce::String blurb;
    float minGainDb, maxGainDb;  // preamp gain range
    int stages;                  // number of clipping stages
    float asymmetry;             // 0 = symmetric
    float lowCutHz;              // input coupling
    float midHz;                 // tone stack mid centre
    float midScoopDb;            // fixed voicing scoop (negative) or hump
    float brightDb;              // bright cap / top end voicing
    float sag;                   // power amp compression 0..1
    float outputTrimDb;
    juce::uint32 faceColour;     // panel colour for the UI
};

struct CabModel
{
    juce::String id;
    juce::String name;
    float lowCut, resonanceHz, resonanceDb, presenceHz, presenceDb, highCut, boxDipHz, boxDipDb;
};

/** Preamp -> tone stack -> power amp -> cabinet -> room. Mono. */
class AmpSim : public Effect
{
public:
    AmpSim();

    static const std::vector<AmpModel>& models();
    static const std::vector<CabModel>& cabs();

    juce::String typeId() const override { return "amp"; }
    void prepare (double sampleRate, int maxBlockSize) override;
    void reset() override;
    void process (float* data, int numSamples) noexcept override;

    enum ParamIndex { pModel, pGain, pBass, pMid, pTreble, pPresence, pMaster, pCab, pMic, pRoom };

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    double osRate = 96000.0;

    Biquad inputHp, brightShelf, interStageHp[3], interStageLp[3];
    Biquad bass, mid, treble, presence, voicing;
    Biquad cabLowCut, cabRes, cabDip, cabPresence, cabHighCut1, cabHighCut2;
    juce::Reverb room;
    std::vector<float> roomBuf;
    float sagEnv = 0.0f;

    juce::SmoothedValue<float> preGain, master;
    float cache[10];
    int currentModel = -1;

    void updateFilters();
};

} // namespace gg
