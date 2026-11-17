#include "LearnViews.h"

namespace gg
{

namespace
{
    juce::String notesOf (const Chord& c)
    {
        juce::StringArray names;
        const int root = c.rootPitchClass();
        const int mask = c.pitchClassMask();
        for (int i = 0; i < 12; ++i)
        {
            const int pc = (root + i) % 12;
            if (mask & (1 << pc)) names.add (pitchClassName (pc));
        }
        return names.joinIntoString ("  ");
    }
}

FlashcardsView::FlashcardsView (AppContext& c, juce::StringArray ids) : ctx (c), deck (std::move (ids))
{
    shuffle = ctx.settings.getBoolValue ("flashShuffle", true);
    startWithDiagram = ctx.settings.getBoolValue ("flashDiagramFirst", false);

    learningButton.setActiveColour (Theme::Colours::bad);
    knowButton.setActiveColour (Theme::Colours::good);
    for (auto* b : { &learningButton, &knowButton, &hearButton })
    {
        variant (*b, "solid");
        addAndMakeVisible (b);
    }
    learningButton.setTooltip ("Still learning (Left arrow)");
    knowButton.setTooltip ("Know it (Right arrow)");
    hearButton.setTooltip ("Hear the chord (H)");
    backButton.setTooltip ("Previous card (Backspace)");
    shuffleButton.setTooltip ("Shuffle cards");
    shuffleButton.setClickingTogglesState (true);
    shuffleButton.setToggleState (shuffle, juce::dontSendNotification);
    addAndMakeVisible (backButton);
    addAndMakeVisible (shuffleButton);

    variant (frontChip, "chip");
    frontChip.setClickingTogglesState (true);
    frontChip.setButtonText ("Diagram first");
    frontChip.setToggleState (startWithDiagram, juce::dontSendNotification);
    frontChip.setTooltip ("Show the chord shape on the front and the name on the back");
    addAndMakeVisible (frontChip);

    learningButton.onClick = [this] { mark (false); };
    knowButton.onClick = [this] { mark (true); };
    backButton.onClick = [this]
    {
        if (session.back()) { showingBack = false; flipProgress = 1.0f; slide = -1.0f; startTimerHz (60); updateButtons(); repaint(); }
    };
    hearButton.onClick = [this] { hearChord (ctx, session.current()); };
    shuffleButton.onClick = [this]
    {
        shuffle = shuffleButton.getToggleState();
        ctx.settings.setValue ("flashShuffle", shuffle);
        restart (deck);
    };
    frontChip.onClick = [this]
    {
        startWithDiagram = frontChip.getToggleState();
        ctx.settings.setValue ("flashDiagramFirst", startWithDiagram);
        showingBack = false;
        repaint();
    };

    variant (reviewButton, "primary");
    variant (restartButton, "ghost");
    variant (learnModeButton, "ghost");
    reviewButton.onClick = [this] { restart (session.learningCards()); };
    restartButton.onClick = [this] { restart (deck); };
    learnModeButton.onClick = [this] { if (onSwitchToLearn) onSwitchToLearn(); };
    addChildComponent (reviewButton);
    addChildComponent (restartButton);
    addChildComponent (learnModeButton);

    setWantsKeyboardFocus (true);
    restart (deck);
}

void FlashcardsView::restart (juce::StringArray ids)
{
    session = FlashcardSession (std::move (ids), shuffle);
    showingBack = false;
    flipProgress = 1.0f;
    slide = 1.0f;
    startTimerHz (60);
    updateButtons();
    resized();
    repaint();
}

void FlashcardsView::finishForScreenshot()
{
    int i = 0;
    while (! session.finished()) session.mark ((i++ % 3) != 0);
    slide = 0;
    updateButtons();
    resized();
    repaint();
}

void FlashcardsView::updateButtons()
{
    const bool done = session.finished();
    for (juce::Component* c : { (juce::Component*) &learningButton, (juce::Component*) &knowButton,
                                (juce::Component*) &backButton, (juce::Component*) &hearButton,
                                (juce::Component*) &shuffleButton, (juce::Component*) &frontChip })
        c->setVisible (! done);

    backButton.setEnabled (session.position() > 0);
    const int learning = session.learningCards().size();
    reviewButton.setButtonText ("Keep reviewing " + juce::String (learning) + (learning == 1 ? " chord" : " chords"));
    reviewButton.setVisible (done && learning > 0);
    restartButton.setVisible (done);
    learnModeButton.setVisible (done);
}

void FlashcardsView::flip()
{
    if (session.finished()) return;
    flipTarget = ! showingBack;
    flipProgress = 0.0f;
    startTimerHz (60);
}

void FlashcardsView::mark (bool known)
{
    if (session.finished()) return;
    ctx.progress.recordAnswer (session.current(), known);
    session.mark (known);
    showingBack = false;
    flipProgress = 1.0f;
    slide = known ? 1.0f : -1.0f;
    startTimerHz (60);
    updateButtons();
    resized();
    repaint();
}

void FlashcardsView::timerCallback()
{
    bool animating = false;
    if (flipProgress < 1.0f)
    {
        const float before = flipProgress;
        flipProgress = juce::jmin (1.0f, flipProgress + 1.0f / (0.26f * 60.0f));
        if (before < 0.5f && flipProgress >= 0.5f) showingBack = flipTarget;
        animating = true;
    }
    if (std::abs (slide) > 0.001f)
    {
        slide *= 0.78f;
        if (std::abs (slide) < 0.01f) slide = 0.0f;
        animating = true;
    }
    if (! animating) stopTimer();
    repaint();
}

void FlashcardsView::drawFace (juce::Graphics& g, juce::Rectangle<float> r, bool back, const Chord& chord)
{
    using namespace Theme::Colours;
    const bool diagramFace = back != startWithDiagram;
    drawPaperCard (g, r, Theme::radiusLg, true);

    auto inner = r.reduced (r.getWidth() * 0.06f, r.getHeight() * 0.08f);
    if (! diagramFace)
    {
        g.setColour (cardInk);
        g.setFont (Theme::cond (r.getHeight() * 0.40f, Theme::Weight::bold));
        g.drawText (chord.symbol, inner.withTrimmedBottom (inner.getHeight() * 0.18f), juce::Justification::centred, false);
        g.setColour (cardInk2);
        g.setFont (Theme::sans (r.getHeight() * 0.075f, Theme::Weight::medium));
        g.drawText (chord.name, inner.removeFromBottom (inner.getHeight() * 0.22f), juce::Justification::centredTop, false);
    }
    else
    {
        auto left = inner.removeFromLeft (inner.getWidth() * (back ? 0.5f : 1.0f));
        ChordDiagram::Style st;
        st.showStringNames = true;
        ChordDiagram::draw (g, left.reduced (8.0f, 0.0f), chord, st);

        if (back)
        {
            inner.removeFromLeft (inner.getWidth() * 0.06f);
            g.setColour (cardInk);
            g.setFont (Theme::cond (r.getHeight() * 0.22f, Theme::Weight::bold));
            g.drawText (chord.symbol, inner.removeFromTop (inner.getHeight() * 0.38f), juce::Justification::bottomLeft, false);
            g.setFont (Theme::sans (r.getHeight() * 0.065f, Theme::Weight::semibold));
            g.drawText (chord.name, inner.removeFromTop (r.getHeight() * 0.11f), juce::Justification::centredLeft, false);
            inner.removeFromTop (r.getHeight() * 0.05f);
            g.setColour (cardInk2);
            g.setFont (Theme::sans (r.getHeight() * 0.05f, Theme::Weight::medium));
            g.drawText ("Notes", inner.removeFromTop (r.getHeight() * 0.07f), juce::Justification::centredLeft, false);
            g.setColour (cardInk);
            g.setFont (Theme::cond (r.getHeight() * 0.075f, Theme::Weight::semibold));
            g.drawText (notesOf (chord), inner.removeFromTop (r.getHeight() * 0.09f), juce::Justification::centredLeft, false);
        }
    }
}

void FlashcardsView::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;

    // Sorting tallies, Quizlet style: still learning on the left, known on the right
    auto bar = cardArea.withY (cardArea.getY() - 54.0f).withHeight (28.0f);
    const int learning = session.learningCards().size(), known = session.knownCards().size();
    drawBadge (g, bar.removeFromLeft (48.0f).reduced (0, 2), juce::String (learning), bad.withAlpha (0.18f), bad);
    drawBadge (g, bar.removeFromRight (48.0f).reduced (0, 2), juce::String (known), good.withAlpha (0.18f), good);
    g.setColour (ink2);
    g.setFont (Theme::sans (15.0f, Theme::Weight::semibold));
    if (! session.finished())
        g.drawText (juce::String (session.position() + 1) + " / " + juce::String (session.size()), bar, juce::Justification::centred, false);

    auto track = cardArea.withY (cardArea.getY() - 18.0f).withHeight (4.0f);
    g.setColour (line);
    g.fillRoundedRectangle (track, 2.0f);
    g.setColour (brass);
    g.fillRoundedRectangle (track.withWidth (track.getWidth() * (float) session.position() / (float) juce::jmax (1, session.size())), 2.0f);

    if (session.finished())
    {
        // Summary
        auto r = cardArea;
        const float pct = session.size() > 0 ? (float) known / (float) session.size() : 0.0f;
        auto ring = juce::Rectangle<float> (150.0f, 150.0f).withCentre ({ r.getCentreX(), r.getY() + 90.0f });
        juce::Path bgArc, fgArc;
        bgArc.addCentredArc (ring.getCentreX(), ring.getCentreY(), 66, 66, 0, 0, juce::MathConstants<float>::twoPi, true);
        fgArc.addCentredArc (ring.getCentreX(), ring.getCentreY(), 66, 66, 0, 0, juce::MathConstants<float>::twoPi * pct, true);
        g.setColour (line);
        g.strokePath (bgArc, juce::PathStrokeType (10.0f));
        g.setColour (good);
        g.strokePath (fgArc, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (ink);
        g.setFont (Theme::cond (48.0f, Theme::Weight::bold));
        g.drawText (juce::String (juce::roundToInt (pct * 100.0f)) + "%", ring, juce::Justification::centred, false);

        g.setFont (Theme::sans (26.0f, Theme::Weight::bold));
        auto msg = learning == 0 ? juce::String ("You know every chord in this deck.")
                                 : "You know " + juce::String (known) + " of " + juce::String (session.size()) + " chords.";
        g.drawText (msg, r.withY (ring.getBottom() + 24.0f).withHeight (36.0f), juce::Justification::centred, false);
        g.setColour (ink2);
        g.setFont (Theme::sans (16.0f));
        g.drawText (learning == 0 ? juce::String ("Lock it in with a Learn quiz, or prove it with Play along.")
                                  : juce::String ("Run through the ones you're still learning until they stick."),
                    r.withY (ring.getBottom() + 62.0f).withHeight (24.0f), juce::Justification::centred, false);
        return;
    }

    auto* chord = ChordLibrary::get().find (session.current());
    if (chord == nullptr) return;

    // Flip: squash horizontally through the edge; slide: enter from the side
    const float sx = juce::jmax (0.02f, std::abs (std::cos (juce::MathConstants<float>::pi * flipProgress)));
    auto r = cardArea.translated (slide * 60.0f, 0.0f);
    juce::Graphics::ScopedSaveState ss (g);
    g.setOpacity (1.0f - std::abs (slide) * 0.8f);
    g.addTransform (juce::AffineTransform::scale (sx, 1.0f, r.getCentreX(), r.getCentreY()));
    drawFace (g, r, showingBack, *chord);

    if (! showingBack && flipProgress >= 1.0f)
    {
        g.setColour (cardInk2.withAlpha (0.7f));
        g.setFont (Theme::sans (14.0f, Theme::Weight::medium));
        g.drawText ("Click or press Space to flip", r.removeFromBottom (36.0f), juce::Justification::centred, false);
    }
}

void FlashcardsView::resized()
{
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (88);
    const float w = (float) juce::jmin (680, r.getWidth() - 120);
    const float h = juce::jmin (w * 0.58f, (float) r.getHeight() - 150.0f);
    cardArea = juce::Rectangle<float> (w, h).withCentre (r.toFloat().getCentre()).withY ((float) r.getY());
    auto controls = r.withTop ((int) cardArea.getBottom() + 16).withHeight (120);

    auto row = controls.withSizeKeepingCentre ((int) w, 52).withY (controls.getY() + 12);
    learningButton.setBounds (row.removeFromLeft (200));
    knowButton.setBounds (row.removeFromRight (200));
    backButton.setBounds (row.withSizeKeepingCentre (48, 48));

    auto under = controls.withSizeKeepingCentre ((int) w, 40).withY (row.getBottom() + 14);
    hearButton.setBounds (under.removeFromLeft (130));
    shuffleButton.setBounds (under.removeFromRight (40));
    under.removeFromRight (8);
    frontChip.setBounds (under.removeFromRight (130).reduced (0, 3));

    auto summary = cardArea.toNearestInt().withTrimmedTop (300).withHeight (46);
    auto buttons = summary.withSizeKeepingCentre (juce::jmin ((int) w, 640), 46);
    if (reviewButton.isVisible())
    {
        reviewButton.setBounds (buttons.removeFromLeft (240));
        buttons.removeFromLeft (12);
    }
    else
        buttons = buttons.withSizeKeepingCentre (400, 46);
    restartButton.setBounds (buttons.removeFromLeft (130));
    buttons.removeFromLeft (12);
    learnModeButton.setBounds (buttons.removeFromLeft (230));
}

void FlashcardsView::mouseUp (const juce::MouseEvent& e)
{
    if (cardArea.contains (e.position)) flip();
}

bool FlashcardsView::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey || k == juce::KeyPress::upKey || k == juce::KeyPress::downKey) { flip(); return true; }
    if (k == juce::KeyPress::leftKey || k.getTextCharacter() == '1')  { mark (false); return true; }
    if (k == juce::KeyPress::rightKey || k.getTextCharacter() == '2') { mark (true); return true; }
    if (k == juce::KeyPress::backspaceKey) { backButton.triggerClick(); return true; }
    if (k.getTextCharacter() == 'h' || k.getTextCharacter() == 'H') { hearButton.triggerClick(); return true; }
    if (k == juce::KeyPress::returnKey && session.finished())
    {
        (reviewButton.isVisible() ? reviewButton : restartButton).triggerClick();
        return true;
    }
    return false;
}

} // namespace gg
