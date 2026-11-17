#pragma once

#include "StudioViews.h"
#include "ui/Shootable.h"

namespace gg
{

/** Practice studio: transport on top, tracks in the middle, the rig (pedals + amp) below. */
class StudioPage : public juce::Component, public Shootable
{
public:
    explicit StudioPage (AppContext&);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    int numShots() const override { return 2; }
    void prepareShot (int index) override;

private:
    AppContext& ctx;
    TransportBar transport;
    TrackArea tracks;
    PedalboardView pedalboard;
    AmpPanel amp;
    juce::TextButton rigToggle { "Hide rig" };
    bool rigVisible = true;
};

} // namespace gg
