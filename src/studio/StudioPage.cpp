#include "StudioPage.h"
#include "core/StrumSynth.h"

namespace gg
{

StudioPage::StudioPage (AppContext& c)
    : ctx (c), transport (c), tracks (c), pedalboard (c), amp (c)
{
    transport.getArmedTrack = [this] { return tracks.armedTrack(); };
    addAndMakeVisible (transport);
    addAndMakeVisible (tracks);
    addAndMakeVisible (pedalboard);
    addAndMakeVisible (amp);

    variant (rigToggle, "ghost");
    rigToggle.setTooltip ("Show or hide the pedalboard and amp to make room for tracks");
    rigToggle.onClick = [this]
    {
        rigVisible = ! rigVisible;
        rigToggle.setButtonText (rigVisible ? "Hide rig" : "Show rig");
        pedalboard.setVisible (rigVisible);
        amp.setVisible (rigVisible);
        resized();
    };
    addAndMakeVisible (rigToggle);
    setWantsKeyboardFocus (true);
}

void StudioPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colours::bg);
}

void StudioPage::resized()
{
    auto r = getLocalBounds();
    transport.setBounds (r.removeFromTop (76));

    if (rigVisible)
    {
        auto rig = r.removeFromBottom (juce::jmin (500, juce::jmax (420, r.getHeight() / 2 + 60)));
        amp.setBounds (rig.removeFromBottom (rig.getHeight() * 46 / 100).reduced (Theme::s5, 8));
        pedalboard.setBounds (rig.reduced (Theme::s5, 4));
    }
    tracks.setBounds (r.reduced (Theme::s5, 6));
    rigToggle.setBounds (tracks.getRight() - 104, tracks.getY() + 6, 104, 32);
}

bool StudioPage::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey) { ctx.audio.togglePlay(); return true; }
    if (k.getTextCharacter() == 'r' || k.getTextCharacter() == 'R')
    {
        if (ctx.audio.isRecording()) ctx.audio.stop();
        else ctx.audio.record (tracks.armedTrack());
        return true;
    }
    if (k == juce::KeyPress::homeKey || k == juce::KeyPress::returnKey) { ctx.audio.setPosition (0); return true; }
    if (k.getTextCharacter() == 'm' || k.getTextCharacter() == 'M') { ctx.audio.metronomeOn = ! ctx.audio.metronomeOn.load(); return true; }
    return false;
}

void StudioPage::prepareShot (int index)
{
    if (index == 1 && ctx.audio.session.lengthSamples() == 0)
    {
        // Put a couple of takes on the timeline so the waveform view can be checked
        const double sr = ctx.audio.sampleRate();
        auto riff = StrumSynth::renderDemoRiff (sr);
        SignalChain chain;
        chain.prepare (sr, 512);
        chain.fromVar (ctx.audio.chain.toVar());
        std::vector<float> audio;
        for (int rep = 0; rep < 4; ++rep)
        {
            juce::AudioBuffer<float> b (riff);
            chain.process (b.getWritePointer (0), b.getNumSamples());
            audio.insert (audio.end(), b.getReadPointer (0), b.getReadPointer (0) + b.getNumSamples());
        }
        ctx.audio.session.writeAudio (0, 0, audio.data(), (int) audio.size());
        auto t = ctx.audio.session.addTrack ("Lead");
        auto chord = StrumSynth::renderChord (*ChordLibrary::get().find ("Em"), sr, 4.0);
        ctx.audio.session.writeAudio (t, (juce::int64) (sr * 4.8), chord.getReadPointer (0), chord.getNumSamples());
        ctx.audio.setPosition ((juce::int64) (sr * 3.1));
        tracks.rebuildLanes();
    }
}

} // namespace gg
