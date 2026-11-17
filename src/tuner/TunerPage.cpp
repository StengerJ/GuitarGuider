#include "TunerPage.h"

namespace gg
{

TunerPage::TunerPage (AppContext& c) : ctx (c)
{
    buffer.assign (4096, 0.0f);
    a4 = (float) ctx.settings.getDoubleValue ("tunerA4", 440.0);

    for (auto* b : { &a4Down, &a4Up })
    {
        variant (*b, "solid");
        addAndMakeVisible (b);
    }
    a4Down.onClick = [this] { a4 = juce::jmax (430.0f, a4 - 1.0f); ctx.settings.setValue ("tunerA4", a4); repaint(); };
    a4Up.onClick = [this] { a4 = juce::jmin (450.0f, a4 + 1.0f); ctx.settings.setValue ("tunerA4", a4); repaint(); };

    muteToggle.setToggleState (ctx.settings.getBoolValue ("tunerMute", true), juce::dontSendNotification);
    muteToggle.onClick = [this]
    {
        ctx.settings.setValue ("tunerMute", muteToggle.getToggleState());
        ctx.audio.monitorOn = muteToggle.getToggleState() ? false : wasMonitoring;
    };
    addAndMakeVisible (muteToggle);
}

TunerPage::~TunerPage()
{
    if (isVisible()) ctx.audio.monitorOn = wasMonitoring;
}

void TunerPage::visibilityChanged()
{
    if (isVisible())
    {
        wasMonitoring = ctx.audio.monitorOn.load();
        if (muteToggle.getToggleState()) ctx.audio.monitorOn = false;
        startTimerHz (30);
    }
    else
    {
        ctx.audio.monitorOn = wasMonitoring;
        stopTimer();
    }
}

void TunerPage::timerCallback()
{
    ctx.audio.copyRecentInput (buffer.data(), (int) buffer.size());
    auto r = detector.detect (buffer.data(), (int) buffer.size(), ctx.audio.sampleRate(), 60.0f, 1100.0f);

    if (r.frequency > 0.0f && r.confidence > 0.8f)
    {
        recentHz.push_back (r.frequency);
        if (recentHz.size() > 5) recentHz.erase (recentHz.begin());
        auto sorted = recentHz;
        std::sort (sorted.begin(), sorted.end());
        hz = sorted[sorted.size() / 2];
        auto n = PitchDetector::toNote (hz, a4);
        if (n.midi != note.midi) shownCents = n.cents;
        note = n;
        shownCents += 0.3f * (note.cents - shownCents);
        silentFrames = 0;
    }
    else if (++silentFrames > 20)
    {
        recentHz.clear();
    }
    repaint();
}

void TunerPage::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    g.fillAll (bg);
    drawHeading (g, getLocalBounds().reduced (Theme::s6, 0).withTrimmedTop (34).withHeight (70), "Tuner",
                 "Standard tuning, low to high: E A D G B E. Pick one string at a time.");

    const bool active = silentFrames < 20;
    const bool inTune = active && std::abs (shownCents) < 3.0f;

    auto ga = gaugeArea.toFloat();
    auto centre = juce::Point<float> (ga.getCentreX(), ga.getBottom() - 30.0f);
    const float radius = juce::jmin (ga.getWidth() * 0.45f, ga.getHeight() - 50.0f);
    const float span = juce::degreesToRadians (65.0f);

    // Arc band and in-tune zone
    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius, radius, 0, -span, span, true);
    g.setColour (raised);
    g.strokePath (arc, juce::PathStrokeType (14.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path zone;
    const float zoneSpan = span * (3.0f / 50.0f);
    zone.addCentredArc (centre.x, centre.y, radius, radius, 0, -zoneSpan, zoneSpan, true);
    g.setColour (good.withAlpha (inTune ? 1.0f : 0.45f));
    g.strokePath (zone, juce::PathStrokeType (14.0f));

    // Ticks every 10 cents
    g.setFont (Theme::cond (14.0f, Theme::Weight::semibold));
    for (int c = -50; c <= 50; c += 5)
    {
        const float a = span * (float) c / 50.0f;
        auto dir = juce::Point<float> (std::sin (a), -std::cos (a));
        const bool major = c % 10 == 0;
        g.setColour (major ? ink2 : ink3);
        g.drawLine (juce::Line<float> (centre + dir * (radius + 14.0f), centre + dir * (radius + (major ? 28.0f : 22.0f))), major ? 2.0f : 1.2f);
        if (major && c != 0)
            g.drawText ((c > 0 ? "+" : "") + juce::String (c), juce::Rectangle<float> (40, 18).withCentre (centre + dir * (radius + 44.0f)),
                        juce::Justification::centred, false);
    }

    // Needle
    if (active)
    {
        const float a = span * juce::jlimit (-50.0f, 50.0f, shownCents) / 50.0f;
        auto dir = juce::Point<float> (std::sin (a), -std::cos (a));
        const auto col = inTune ? good : (std::abs (shownCents) < 15.0f ? brass : glow);
        g.setColour (col.withAlpha (0.25f));
        g.drawLine (juce::Line<float> (centre, centre + dir * (radius + 6.0f)), 9.0f);
        g.setColour (col);
        g.drawLine (juce::Line<float> (centre, centre + dir * (radius + 6.0f)), 3.0f);
        g.fillEllipse (juce::Rectangle<float> (16, 16).withCentre (centre));
    }

    // Note readout
    auto noteArea = juce::Rectangle<float> (300.0f, 150.0f).withCentre ({ centre.x, centre.y - radius * 0.45f });
    g.setColour (active ? (inTune ? good : ink) : ink3.withAlpha (0.5f));
    g.setFont (Theme::cond (150.0f, Theme::Weight::bold));
    g.drawText (active ? note.name : juce::String ("E"), noteArea, juce::Justification::centred, false);
    if (active)
    {
        g.setFont (Theme::cond (34.0f, Theme::Weight::semibold));
        g.setColour (ink2);
        g.drawText (juce::String (note.octave), noteArea.translated (noteArea.getWidth() * 0.3f, 30.0f).withWidth (60.0f),
                    juce::Justification::centred, false);
    }

    auto info = juce::Rectangle<float> (500.0f, 30.0f).withCentre ({ centre.x, centre.y + 50.0f });
    g.setFont (Theme::sans (20.0f, Theme::Weight::semibold));
    if (active)
    {
        g.setColour (inTune ? good : ink);
        const int cents = juce::roundToInt (shownCents);
        g.drawText (inTune ? juce::String ("In tune")
                           : (cents > 0 ? "+" + juce::String (cents) + " cents, tune down" : juce::String (cents) + " cents, tune up"),
                    info, juce::Justification::centred, false);
        g.setColour (ink3);
        g.setFont (Theme::sans (15.0f));
        g.drawText (juce::String (hz, 1) + " Hz", info.translated (0, 28.0f), juce::Justification::centred, false);
    }
    else
    {
        g.setColour (ink2);
        g.drawText (ctx.audio.hasInputDevice() || ctx.audio.demoInput.load() ? juce::String ("Play a single string")
                                                                              : juce::String ("No guitar input. Choose your interface in Settings."),
                    info, juce::Justification::centred, false);
    }

    // String targets
    static const int strings[] { 40, 45, 50, 55, 59, 64 };
    auto row = juce::Rectangle<float> (6 * 70.0f, 56.0f).withCentre ({ centre.x, (float) getHeight() - 120.0f });
    for (int i = 0; i < 6; ++i)
    {
        auto chip = row.removeFromLeft (70.0f).reduced (6.0f, 0);
        const bool hit = active && note.midi == strings[i];
        g.setColour (hit ? (inTune ? good : brass) : raised);
        g.fillRoundedRectangle (chip, 10.0f);
        g.setColour (hit ? bg : ink);
        g.setFont (Theme::cond (24.0f, Theme::Weight::bold));
        g.drawText (pitchClassName (strings[i] % 12), chip.withTrimmedBottom (14.0f), juce::Justification::centred, false);
        g.setFont (Theme::cond (12.0f, Theme::Weight::semibold));
        g.setColour (hit ? bg : ink3);
        g.drawText (juce::String (6 - i) + (i == 0 ? " low" : (i == 5 ? " high" : "")), chip.withTrimmedTop (32.0f),
                    juce::Justification::centred, false);
    }

    // Reference pitch
    g.setColour (ink);
    g.setFont (Theme::cond (20.0f, Theme::Weight::bold));
    g.drawText ("A4 = " + juce::String ((int) a4) + " Hz", a4Down.getBounds().getUnion (a4Up.getBounds()).toFloat(),
                juce::Justification::centred, false);
}

void TunerPage::resized()
{
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (130);
    auto bottom = r.removeFromBottom (190);
    gaugeArea = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 820), r.getHeight());

    auto controls = bottom.removeFromBottom (56).withSizeKeepingCentre (560, 40);
    a4Down.setBounds (controls.removeFromLeft (40));
    controls.removeFromLeft (110);
    a4Up.setBounds (controls.removeFromLeft (40));
    controls.removeFromLeft (40);
    muteToggle.setBounds (controls);
}

} // namespace gg
