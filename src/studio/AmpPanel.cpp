#include "StudioViews.h"

namespace gg
{

AmpPanel::AmpPanel (AppContext& c) : ctx (c)
{
    auto& amp = ctx.audio.chain.amp();
    for (int idx : { AmpSim::pGain, AmpSim::pBass, AmpSim::pMid, AmpSim::pTreble, AmpSim::pPresence, AmpSim::pMaster })
    {
        auto* k = mainKnobs.add (new ParamKnob (*amp.params()[(size_t) idx], "amp", faceInk()));
        k->setCaptionFont (Theme::cond (15.0f, Theme::Weight::bold));
        addAndMakeVisible (k);
    }

    micKnob = std::make_unique<ParamKnob> (*amp.params()[AmpSim::pMic], "pedal", Theme::Colours::ink);
    roomKnob = std::make_unique<ParamKnob> (*amp.params()[AmpSim::pRoom], "pedal", Theme::Colours::ink);
    gateKnob = std::make_unique<ParamKnob> (*ctx.audio.chain.gate().params()[0], "pedal", Theme::Colours::ink);
    micKnob->slider.setTooltip ("Mic position: close and bright, or off-axis and darker");
    roomKnob->slider.setTooltip ("Room ambience");
    gateKnob->slider.setTooltip ("Noise gate threshold: higher cuts more hum between notes");
    for (auto* k : { micKnob.get(), roomKnob.get(), gateKnob.get() })
    {
        k->setCaptionFont (Theme::cond (13.0f, Theme::Weight::semibold));
        addAndMakeVisible (k);
    }

    for (auto& cab : AmpSim::cabs()) cabBox.addItem (cab.name, cabBox.getNumItems() + 1);
    cabBox.setTooltip ("Speaker cabinet");
    cabBox.onChange = [this]
    {
        ctx.audio.chain.amp().params()[AmpSim::pCab]->set ((float) cabBox.getSelectedItemIndex());
    };
    addAndMakeVisible (cabBox);

    prevModel.onClick = [this] { setModel ((model() + (int) AmpSim::models().size() - 1) % (int) AmpSim::models().size()); };
    nextModel.onClick = [this] { setModel ((model() + 1) % (int) AmpSim::models().size()); };
    addAndMakeVisible (prevModel);
    addAndMakeVisible (nextModel);

    powerButton.setTooltip ("Amp on/off: turn off to hear just the pedals");
    powerButton.onClick = [this] { ctx.audio.chain.ampEnabled = ! ctx.audio.chain.ampEnabled.load(); repaint(); };
    addAndMakeVisible (powerButton);

    gateToggle.setButtonText ("Gate");
    gateToggle.setTooltip ("Noise gate on/off");
    gateToggle.onClick = [this] { ctx.audio.chain.gateEnabled = gateToggle.getToggleState(); };
    addAndMakeVisible (gateToggle);

    refresh();
    startTimerHz (10);
}

int AmpPanel::model() const
{
    return juce::jlimit (0, (int) AmpSim::models().size() - 1,
                         (int) std::lround (ctx.audio.chain.amp().value (AmpSim::pModel)));
}

void AmpPanel::setModel (int m)
{
    ctx.audio.chain.amp().params()[AmpSim::pModel]->set ((float) m);
    refresh();
}

juce::Colour AmpPanel::faceInk() const
{
    const juce::Colour faceCol (AmpSim::models()[(size_t) model()].faceColour);
    return faceCol.getPerceivedBrightness() > 0.5f ? Theme::Colours::cardInk : juce::Colour (0xffe9dcc0);
}

void AmpPanel::refresh()
{
    const auto faceText = faceInk();
    for (auto* k : mainKnobs)
    {
        k->setInk (faceText);
        k->refresh();
    }
    for (auto* b : { &prevModel, &nextModel, &powerButton })
    {
        b->getProperties().set ("fg", (juce::int64) faceText.getARGB());
        b->repaint();
    }
    for (auto* k : { micKnob.get(), roomKnob.get(), gateKnob.get() }) k->refresh();
    cabBox.setSelectedItemIndex ((int) std::lround (ctx.audio.chain.amp().value (AmpSim::pCab)), juce::dontSendNotification);
    gateToggle.setToggleState (ctx.audio.chain.gateEnabled.load(), juce::dontSendNotification);
    lastModel = model();
    lastVersion = ctx.audio.chain.structureVersion.load();
    repaint();
}

void AmpPanel::timerCallback()
{
    if (ctx.audio.chain.structureVersion.load() != lastVersion || model() != lastModel)
        refresh();
}

void AmpPanel::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds().toFloat();
    const auto& m = AmpSim::models()[(size_t) model()];
    const juce::Colour faceCol (m.faceColour);
    const auto faceText = faceInk();
    const bool on = ctx.audio.chain.ampEnabled.load();

    // Tolex shell
    Theme::dropShadow (g, r, 14.0f, 0.5f);
    g.setColour (juce::Colour (0xff15171a));
    g.fillRoundedRectangle (r, 14.0f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (r, 14.0f);
        g.reduceClipRegion (clip);
        g.setColour (juce::Colours::white.withAlpha (0.025f));
        for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += 5.0f)
            g.drawLine (x, r.getBottom(), x + r.getHeight(), r.getY(), 1.0f);
    }

    // Faceplate
    auto f = face.toFloat();
    if (m.id == "tweed")
    {
        g.setColour (faceCol);
        g.fillRoundedRectangle (f, 8.0f);
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (face);
        g.setColour (juce::Colour (0xff8a6a3a).withAlpha (0.18f));
        for (float x = f.getX() - f.getHeight(); x < f.getRight(); x += 6.0f)
        {
            g.drawLine (x, f.getBottom(), x + f.getHeight(), f.getY(), 1.6f);
            g.drawLine (x, f.getY(), x + f.getHeight(), f.getBottom(), 0.8f);
        }
    }
    else
    {
        juce::ColourGradient grad (faceCol.brighter (0.06f), f.getX(), f.getY(), faceCol.darker (0.1f), f.getX(), f.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (f, 8.0f);
    }
    g.setColour (brass.withAlpha (0.85f));
    g.drawRoundedRectangle (f.reduced (5.0f), 5.0f, 1.5f);

    // Model name & blurb
    auto ls = leftSection.toFloat();
    g.setColour (faceText);
    g.setFont (Theme::cond (28.0f, Theme::Weight::bold));
    g.drawFittedText (m.name, ls.withHeight (36.0f).withTrimmedLeft (36.0f).withTrimmedRight (36.0f).toNearestInt(),
                      juce::Justification::centred, 1, 0.7f);
    g.setColour (faceText.withAlpha (0.7f));
    g.setFont (Theme::sans (13.0f, Theme::Weight::medium));
    g.drawFittedText (m.blurb, ls.withTrimmedTop (42.0f).withHeight (36.0f).reduced (6.0f, 0).toNearestInt(), juce::Justification::centredTop, 2, 1.0f);

    // Jewel pilot light
    auto jewel = juce::Rectangle<float> (18.0f, 18.0f).withCentre ({ powerButton.getBounds().toFloat().getRight() + 22.0f,
                                                                    powerButton.getBounds().toFloat().getCentreY() });
    if (on)
    {
        g.setColour (glow.withAlpha (0.35f));
        g.fillEllipse (jewel.expanded (6.0f));
        juce::ColourGradient jg (juce::Colour (0xffffb27a), jewel.getCentreX() - 3, jewel.getCentreY() - 3,
                                 juce::Colour (0xffc2321a), jewel.getRight(), jewel.getBottom(), true);
        g.setGradientFill (jg);
    }
    else
        g.setColour (juce::Colour (0xff4a1c14));
    g.fillEllipse (jewel);
    g.setColour (juce::Colour (0xffb8bcc2));
    g.drawEllipse (jewel.expanded (2.0f), 2.0f);
    g.setColour (faceText.withAlpha (0.75f));
    g.setFont (Theme::cond (13.0f, Theme::Weight::bold));
    g.drawText (on ? "ON" : "OFF", jewel.translated (30.0f, 0.0f).withWidth (40.0f), juce::Justification::centredLeft, false);

    // Divider before the knobs
    g.setColour (faceText.withAlpha (0.25f));
    g.fillRect (juce::Rectangle<float> (1.0f, f.getHeight() - 40.0f).withPosition (ls.getRight() + 8.0f, f.getY() + 20.0f));

    // Cab & room back-panel inset
    auto cs = cabSection.toFloat();
    g.setColour (juce::Colour (0xff1b1e23));
    g.fillRoundedRectangle (cs, 8.0f);
    g.setColour (line);
    g.drawRoundedRectangle (cs.reduced (0.5f), 8.0f, 1.0f);
    g.setColour (ink2);
    g.setFont (Theme::sans (13.0f, Theme::Weight::semibold));
    g.drawText ("Cabinet & room", cs.reduced (12.0f, 8.0f).withHeight (18.0f), juce::Justification::centredLeft, false);

    if (! on)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (f.withRight (cs.getX() - 6.0f), 8.0f);
    }
}

void AmpPanel::resized()
{
    auto r = getLocalBounds();
    face = r.reduced (10);
    auto inner = face.reduced (14, 10);

    cabSection = inner.removeFromRight (juce::jmin (330, inner.getWidth() / 3));
    leftSection = inner.removeFromLeft (juce::jmin (250, inner.getWidth() / 4));
    inner.removeFromLeft (20);

    // Left: model name, arrows, power
    auto ls = leftSection;
    prevModel.setBounds (ls.getX(), ls.getY() + 2, 32, 32);
    nextModel.setBounds (ls.getRight() - 32, ls.getY() + 2, 32, 32);
    powerButton.setBounds (ls.getX() + 6, ls.getBottom() - 42, 38, 38);

    // Amp knobs
    const int n = mainKnobs.size();
    const int kw = inner.getWidth() / juce::jmax (1, n);
    const int ks = juce::jmin (kw, inner.getHeight());
    for (int i = 0; i < n; ++i)
        mainKnobs[i]->setBounds (juce::Rectangle<int> (kw, ks).withPosition (inner.getX() + i * kw, inner.getCentreY() - ks / 2));

    // Cab section
    auto cs = cabSection.reduced (12, 8);
    cs.removeFromTop (22);
    auto top = cs.removeFromTop (34);
    gateToggle.setBounds (top.removeFromRight (84));
    top.removeFromRight (8);
    cabBox.setBounds (top);
    cs.removeFromTop (6);
    const int sk = cs.getWidth() / 3;
    micKnob->setBounds (cs.removeFromLeft (sk));
    roomKnob->setBounds (cs.removeFromLeft (sk));
    gateKnob->setBounds (cs);
}

} // namespace gg
