#include "LearnViews.h"

namespace gg
{

struct MatchView::Tile : public juce::Button
{
    Tile (const MatchGame::Tile& t) : juce::Button (t.chordId), data (t)
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setWantsKeyboardFocus (true);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        using namespace Theme::Colours;
        if (fade <= 0.0f) return;
        auto r = getLocalBounds().toFloat().reduced (4.0f);
        auto* chord = ChordLibrary::get().find (data.chordId);
        if (chord == nullptr) return;

        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (fade);
        const float shake = wrongFlash > 0.0f ? std::sin (wrongFlash * 40.0f) * 4.0f * wrongFlash : 0.0f;
        r = r.translated (shake, down ? 1.0f : 0.0f);

        if (data.isDiagram)
        {
            drawPaperCard (g, r, 12.0f, true);
            ChordDiagram::Style st;
            st.showFingers = false;
            ChordDiagram::draw (g, r.reduced (r.getWidth() * 0.14f, r.getHeight() * 0.1f), *chord, st);
        }
        else
        {
            g.setColour (over ? raised.brighter (0.07f) : raised);
            g.fillRoundedRectangle (r, 12.0f);
            g.setColour (ink);
            g.setFont (Theme::cond (r.getHeight() * 0.32f, Theme::Weight::bold));
            g.drawText (chord->symbol, r.withTrimmedBottom (r.getHeight() * 0.2f), juce::Justification::centred, false);
            g.setColour (ink2);
            g.setFont (Theme::sans (14.0f, Theme::Weight::medium));
            g.drawText (chord->name, r.withTrimmedTop (r.getHeight() * 0.6f), juce::Justification::centredTop, false);
        }

        juce::Colour edge = selected ? brass : (matchedFlash > 0.0f ? good : (wrongFlash > 0.0f ? bad : line));
        g.setColour (edge);
        g.drawRoundedRectangle (r, 12.0f, selected || matchedFlash > 0 || wrongFlash > 0 ? 3.0f : 1.0f);
        if (hasKeyboardFocus (false) && ! selected)
        {
            g.setColour (brass.withAlpha (0.7f));
            g.drawRoundedRectangle (r.expanded (2.0f), 14.0f, 1.5f);
        }
    }

    MatchGame::Tile data;
    bool selected = false;
    float fade = 1.0f, wrongFlash = 0.0f, matchedFlash = 0.0f;
};

MatchView::MatchView (AppContext& c, juce::StringArray ids, juce::String id)
    : ctx (c), deck (std::move (ids)), deckId (std::move (id))
{
    variant (startButton, "primary");
    startButton.onClick = [this] { startGame(); };
    addAndMakeVisible (startButton);
}

MatchView::~MatchView() = default;

void MatchView::startGame()
{
    game = MatchGame (deck, 6);
    tiles.clear();
    for (size_t i = 0; i < game.tiles().size(); ++i)
    {
        auto* t = tiles.add (new Tile (game.tiles()[i]));
        const int idx = (int) i;
        t->onClick = [this, idx] { tileClicked (idx); };
        addAndMakeVisible (t);
    }
    selected = -1;
    penalty = 0.0;
    newRecord = false;
    state = State::playing;
    startTime = juce::Time::getMillisecondCounterHiRes();
    startButton.setVisible (false);
    startTimerHz (30);
    resized();
    repaint();
}

void MatchView::finishForScreenshot()
{
    state = State::done;
    finalTime = 14.3;
    tiles.clear();
    startButton.setButtonText ("Play again");
    startButton.setVisible (true);
    resized();
    repaint();
}

void MatchView::tileClicked (int index)
{
    if (state != State::playing || game.tiles()[(size_t) index].matched) return;

    if (selected < 0)
    {
        selected = index;
        tiles[index]->selected = true;
        tiles[index]->repaint();
        return;
    }
    if (selected == index)
    {
        tiles[index]->selected = false;
        selected = -1;
        tiles[index]->repaint();
        return;
    }

    auto* a = tiles[selected];
    auto* b = tiles[index];
    a->selected = false;
    if (game.tryMatch (selected, index))
    {
        a->matchedFlash = b->matchedFlash = 1.0f;
        a->data.matched = b->data.matched = true;
    }
    else
    {
        a->wrongFlash = b->wrongFlash = 1.0f;
        penalty += 1.0;
    }
    selected = -1;

    if (game.complete())
    {
        finalTime = (juce::Time::getMillisecondCounterHiRes() - startTime) / 1000.0 + penalty;
        newRecord = ctx.progress.submitMatchTime (deckId, finalTime);
        juce::Timer::callAfterDelay (450, [safe = juce::Component::SafePointer<MatchView> (this)]
        {
            if (safe == nullptr) return;
            safe->state = State::done;
            safe->tiles.clear();
            safe->startButton.setButtonText ("Play again");
            safe->startButton.setVisible (true);
            safe->resized();
            safe->repaint();
        });
    }
}

void MatchView::timerCallback()
{
    for (auto* t : tiles)
    {
        bool dirty = false;
        if (t->wrongFlash > 0) { t->wrongFlash = juce::jmax (0.0f, t->wrongFlash - 0.06f); dirty = true; }
        if (t->matchedFlash > 0) { t->matchedFlash = juce::jmax (0.0f, t->matchedFlash - 0.08f); dirty = true; }
        if (t->data.matched && t->matchedFlash <= 0.3f && t->fade > 0)
        {
            t->fade = juce::jmax (0.0f, t->fade - 0.15f);
            if (t->fade <= 0.0f) t->setEnabled (false);
            dirty = true;
        }
        if (dirty) t->repaint();
    }
    if (state == State::playing)
        repaint (getLocalBounds().removeFromTop (110));
    else if (tiles.isEmpty())
        stopTimer();
}

void MatchView::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (110);
    const double best = ctx.progress.bestMatchTime (deckId);

    if (state == State::playing)
    {
        const double t = (juce::Time::getMillisecondCounterHiRes() - startTime) / 1000.0 + penalty;
        auto top = getLocalBounds().reduced (Theme::s6, 0).withTrimmedTop (24).withHeight (60);
        g.setColour (ink);
        g.setFont (Theme::cond (44.0f, Theme::Weight::bold));
        g.drawText (juce::String (t, 1) + " s", top, juce::Justification::centredLeft, false);
        g.setColour (ink2);
        g.setFont (Theme::sans (15.0f));
        g.drawText ((penalty > 0 ? "+" + juce::String ((int) penalty) + " s penalty   " : juce::String())
                        + (best > 0 ? "Best " + juce::String (best, 1) + " s" : juce::String ("No best time yet")),
                    top, juce::Justification::centredRight, false);
        return;
    }

    auto centre = getLocalBounds().toFloat().getCentre().translated (0, -40);
    if (state == State::intro)
    {
        g.setColour (ink);
        g.setFont (Theme::sans (30.0f, Theme::Weight::bold));
        g.drawText ("Match names to shapes", juce::Rectangle<float> (700, 40).withCentre (centre.translated (0, -70)), juce::Justification::centred, false);
        g.setColour (ink2);
        g.setFont (Theme::sans (17.0f));
        g.drawText ("Pair each chord name with its diagram as fast as you can. A wrong pair adds a second.",
                    juce::Rectangle<float> (760, 26).withCentre (centre.translated (0, -28)), juce::Justification::centred, false);
        if (best > 0)
        {
            g.setColour (brass);
            g.setFont (Theme::sans (16.0f, Theme::Weight::semibold));
            g.drawText ("Your best: " + juce::String (best, 1) + " s", juce::Rectangle<float> (400, 24).withCentre (centre.translated (0, 4)),
                        juce::Justification::centred, false);
        }
    }
    else
    {
        g.setColour (newRecord ? brass : ink);
        g.setFont (Theme::cond (88.0f, Theme::Weight::bold));
        g.drawText (juce::String (finalTime, 1) + " s", juce::Rectangle<float> (500, 96).withCentre (centre.translated (0, -80)),
                    juce::Justification::centred, false);
        g.setColour (ink);
        g.setFont (Theme::sans (24.0f, Theme::Weight::bold));
        g.drawText (newRecord ? "New personal best" : "Nice matching",
                    juce::Rectangle<float> (500, 34).withCentre (centre.translated (0, -14)), juce::Justification::centred, false);
        g.setColour (ink2);
        g.setFont (Theme::sans (16.0f));
        if (best > 0 && ! newRecord)
            g.drawText ("Your best is " + juce::String (best, 1) + " s", juce::Rectangle<float> (500, 24).withCentre (centre.translated (0, 18)),
                        juce::Justification::centred, false);
    }
}

void MatchView::resized()
{
    auto r = getLocalBounds().reduced (Theme::s6, 0);
    r.removeFromTop (96);
    r.removeFromBottom (Theme::s5);

    if (! tiles.isEmpty())
    {
        const int cols = 4, rows = (tiles.size() + cols - 1) / cols;
        const int gap = 14;
        int tw = (r.getWidth() - gap * (cols - 1)) / cols;
        int th = (r.getHeight() - gap * (rows - 1)) / rows;
        th = juce::jmin (th, (int) (tw * 0.9f));
        tw = juce::jmin (tw, (int) (th * 1.25f));
        auto grid = r.withSizeKeepingCentre (tw * cols + gap * (cols - 1), th * rows + gap * (rows - 1));
        for (int i = 0; i < tiles.size(); ++i)
            tiles[i]->setBounds (grid.getX() + (i % cols) * (tw + gap), grid.getY() + (i / cols) * (th + gap), tw, th);
    }

    startButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, 48).translated (0, 40));
}

} // namespace gg
