#include "LearnViews.h"

namespace gg
{

PlayAlongView::PlayAlongView (AppContext& c, juce::StringArray ids)
    : ctx (c), deck (std::move (ids)), recognizer (std::make_unique<ChordRecognizer>())
{
    buffer.assign ((size_t) ChordRecognizer::fftSize, 0.0f);

    variant (skipButton, "solid");
    variant (hearButton, "solid");
    skipButton.onClick = [this] { streak = 0; nextChord(); };
    hearButton.onClick = [this] { hearChord (ctx, target); };
    addAndMakeVisible (skipButton);
    addAndMakeVisible (hearButton);

    variant (settingsButton, "primary");
    variant (demoButton, "ghost");
    settingsButton.onClick = [this] { if (ctx.showPage) ctx.showPage (3); };
    demoButton.onClick = [this] { ctx.audio.demoInput = true; repaint(); };
    addChildComponent (settingsButton);
    addChildComponent (demoButton);

    nextChord();
}

void PlayAlongView::visibilityChanged()
{
    if (isVisible()) startTimerHz (15);
    else stopTimer();
}

void PlayAlongView::nextChord()
{
    if (deck.isEmpty()) return;
    auto prev = target;
    for (int tries = 0; tries < 8 && (target == prev); ++tries)
        target = deck[rng.nextInt (deck.size())];
    heardFrames = 0;
    successAt = 0.0;
    heardChord.clear();
    repaint();
}

void PlayAlongView::timerCallback()
{
    const bool hasInput = ctx.audio.hasInputDevice() || ctx.audio.demoInput.load();
    settingsButton.setVisible (! hasInput);
    demoButton.setVisible (! hasInput);

    const double now = juce::Time::getMillisecondCounterHiRes();
    if (successAt > 0.0)
    {
        if (now - successAt > 1400.0) nextChord();
        repaint();
        return;
    }

    ctx.audio.copyRecentInput (buffer.data(), (int) buffer.size());
    auto* chord = ChordLibrary::get().find (target);
    if (chord == nullptr) return;

    if (recognizer->analyse (buffer.data(), (int) buffer.size(), ctx.audio.sampleRate()))
    {
        const auto& ch = recognizer->chroma();
        float mx = 0.0f;
        for (auto v : ch) mx = juce::jmax (mx, v);
        for (size_t i = 0; i < 12; ++i)
            shownChroma[i] = shownChroma[i] * 0.5f + 0.5f * (mx > 0 ? ch[i] / mx : 0.0f);

        targetScore = recognizer->scoreFor (*chord);
        const auto best = recognizer->best();
        heardChord = best.score > 0.7f ? best.chordId : juce::String();

        if (recognizer->matches (*chord))
        {
            if (++heardFrames >= 3)
            {
                successAt = now;
                ++completed;
                ++streak;
                ctx.progress.recordAnswer (target, true);
            }
        }
        else
            heardFrames = juce::jmax (0, heardFrames - 1);
    }
    else
    {
        for (auto& v : shownChroma) v *= 0.8f;
        targetScore *= 0.8f;
        heardFrames = 0;
        heardChord.clear();
    }
    repaint();
}

void PlayAlongView::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (32);
    r.removeFromBottom (32);

    auto* chord = ChordLibrary::get().find (target);
    if (chord == nullptr) return;
    const bool success = successAt > 0.0;

    // Target cardRect
    auto cardCol = r.removeFromLeft (juce::jmin (400, r.getWidth() / 2 - 20));
    auto cardRect = cardCol.toFloat().withHeight (juce::jmin ((float) cardCol.getHeight() - 70.0f, cardCol.getWidth() * 1.2f));
    drawPaperCard (g, cardRect, Theme::radiusLg, true);
    g.setColour (cardInk);
    g.setFont (Theme::cond (cardRect.getHeight() * 0.13f, Theme::Weight::bold));
    g.drawText (chord->symbol, cardRect.withHeight (cardRect.getHeight() * 0.18f).translated (0, cardRect.getHeight() * 0.03f), juce::Justification::centred, false);
    ChordDiagram::Style st;
    st.showStringNames = true;
    ChordDiagram::draw (g, cardRect.withTrimmedTop (cardRect.getHeight() * 0.22f).reduced (cardRect.getWidth() * 0.12f, cardRect.getHeight() * 0.06f), *chord, st);
    if (success)
    {
        g.setColour (good.withAlpha (0.9f));
        g.drawRoundedRectangle (cardRect.expanded (3.0f), Theme::radiusLg + 3.0f, 4.0f);
    }

    r.removeFromLeft (48);

    // Listening panel
    auto status = r.removeFromTop (110);
    g.setFont (Theme::sans (30.0f, Theme::Weight::bold));
    if (success)
    {
        g.setColour (good);
        g.drawText ("Got it. That's " + chord->symbol + ".", status.removeFromTop (44), juce::Justification::centredLeft, false);
    }
    else
    {
        g.setColour (ink);
        g.drawText ("Strum " + chord->symbol + " and let it ring", status.removeFromTop (44), juce::Justification::centredLeft, false);
    }
    g.setFont (Theme::sans (16.0f));
    g.setColour (ink2);
    juce::String sub;
    if (success) sub = "Next chord coming up.";
    else if (heardChord.isNotEmpty() && heardChord != target && targetScore < 0.72f)
    {
        auto* h = ChordLibrary::get().find (heardChord);
        sub = "Sounds closer to " + (h ? h->symbol : heardChord) + ". Check each string rings clearly.";
    }
    else if (targetScore > 0.05f) sub = "Listening...";
    else sub = "Waiting for sound from your guitar.";
    g.drawText (sub, status.removeFromTop (26), juce::Justification::centredLeft, false);

    // Note activity bars, with the chord's notes marked
    auto bars = r.removeFromTop (juce::jmin (260, r.getHeight() - 120)).toFloat();
    const int mask = chord->pitchClassMask();
    const float bw = bars.getWidth() / 12.0f;
    for (int pc = 0; pc < 12; ++pc)
    {
        auto col = juce::Rectangle<float> (bars.getX() + pc * bw, bars.getY(), bw, bars.getHeight()).reduced (5.0f, 0);
        auto label = col.removeFromBottom (26.0f);
        const bool inChord = (mask & (1 << pc)) != 0;
        g.setColour (raised);
        g.fillRoundedRectangle (col, 6.0f);
        auto fill = col.withTop (col.getBottom() - col.getHeight() * juce::jlimit (0.0f, 1.0f, shownChroma[(size_t) pc]));
        g.setColour (inChord ? (success ? good : brass) : ink3);
        g.fillRoundedRectangle (fill, 6.0f);
        if (inChord)
        {
            g.setColour (brass.withAlpha (0.8f));
            g.drawRoundedRectangle (col, 6.0f, 1.5f);
        }
        g.setColour (inChord ? ink : ink3);
        g.setFont (Theme::cond (17.0f, inChord ? Theme::Weight::bold : Theme::Weight::medium));
        g.drawText (pitchClassName (pc), label, juce::Justification::centred, false);
    }

    // Match meter
    r.removeFromTop (22);
    auto meter = r.removeFromTop (34).toFloat();
    g.setColour (ink2);
    g.setFont (Theme::sans (14.0f, Theme::Weight::semibold));
    g.drawText ("Match", meter.removeFromLeft (64.0f), juce::Justification::centredLeft, false);
    auto track = meter.withSizeKeepingCentre (meter.getWidth(), 8.0f);
    g.setColour (line);
    g.fillRoundedRectangle (track, 4.0f);
    g.setColour (success ? good : (targetScore > 0.72f ? brass : ink3));
    g.fillRoundedRectangle (track.withWidth (track.getWidth() * juce::jlimit (0.0f, 1.0f, targetScore)), 4.0f);
    g.setColour (ink);
    g.fillRect (juce::Rectangle<float> (2.0f, 16.0f).withCentre ({ track.getX() + track.getWidth() * 0.72f, track.getCentreY() }));

    r.removeFromTop (18);
    g.setColour (ink2);
    g.setFont (Theme::sans (16.0f, Theme::Weight::medium));
    g.drawText (juce::String (completed) + " played   Streak " + juce::String (streak), r.removeFromTop (26), juce::Justification::centredLeft, false);

    if (settingsButton.isVisible())
    {
        auto overlay = getLocalBounds().toFloat();
        g.setColour (bg.withAlpha (0.88f));
        g.fillRect (overlay);
        auto c = overlay.getCentre();
        g.setColour (ink);
        g.setFont (Theme::sans (26.0f, Theme::Weight::bold));
        g.drawText ("Plug in your guitar to play along", juce::Rectangle<float> (700, 36).withCentre (c.translated (0, -60)), juce::Justification::centred, false);
        g.setColour (ink2);
        g.setFont (Theme::sans (16.0f));
        g.drawText ("Connect an audio interface and pick it as the input in Settings.",
                    juce::Rectangle<float> (700, 24).withCentre (c.translated (0, -22)), juce::Justification::centred, false);
    }
}

void PlayAlongView::resized()
{
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (32);
    r.removeFromBottom (32);
    auto cardCol = r.removeFromLeft (juce::jmin (400, r.getWidth() / 2 - 20));
    const float cardH = juce::jmin ((float) cardCol.getHeight() - 70.0f, cardCol.getWidth() * 1.2f);
    auto buttons = cardCol.withTrimmedTop ((int) cardH + 20).withHeight (44);
    hearButton.setBounds (buttons.removeFromLeft (140));
    skipButton.setBounds (buttons.removeFromRight (120));

    auto c = getLocalBounds().getCentre();
    settingsButton.setBounds (juce::Rectangle<int> (220, 46).withCentre (c.translated (-120, 30)));
    demoButton.setBounds (juce::Rectangle<int> (200, 46).withCentre (c.translated (110, 30)));
}

} // namespace gg
