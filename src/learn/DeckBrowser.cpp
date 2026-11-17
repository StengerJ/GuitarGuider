#include "LearnViews.h"

namespace gg
{

namespace
{
    /** One deck: a fanned stack of chord cards above the title and mastery bar. */
    struct DeckTile : public juce::Button
    {
        DeckTile (AppContext& c, Deck d) : juce::Button (d.title), ctx (c), deck (std::move (d))
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setWantsKeyboardFocus (true);
            setTitle (deck.title);
            if (! deck.builtIn)
            {
                editButton.setTooltip ("Edit this deck");
                editButton.onClick = [this] { if (onEdit) onEdit(); };
                variant (editButton, "ghost");
                addChildComponent (editButton);
            }
        }

        void resized() override { editButton.setBounds (getWidth() - 72, 12, 60, 30); }
        void mouseEnter (const juce::MouseEvent& e) override { Button::mouseEnter (e); editButton.setVisible (! deck.builtIn); }
        void mouseExit (const juce::MouseEvent& e) override
        {
            Button::mouseExit (e);
            if (! getLocalBounds().contains (e.getEventRelativeTo (this).getPosition())) editButton.setVisible (false);
        }

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            using namespace Theme::Colours;
            auto r = getLocalBounds().toFloat().reduced (2.0f);
            g.setColour (over ? panel.brighter (0.04f) : panel);
            g.fillRoundedRectangle (r, Theme::radiusLg);
            g.setColour (hasKeyboardFocus (false) ? brass : line.withAlpha (0.6f));
            g.drawRoundedRectangle (r.reduced (0.5f), Theme::radiusLg, hasKeyboardFocus (false) ? 1.5f : 1.0f);

            // Fanned stack of up to three chord cards
            auto stage = r.removeFromTop (r.getHeight() * 0.6f).reduced (16.0f, 14.0f);
            const int n = juce::jmin (3, deck.chordIds.size());
            const float cardH = stage.getHeight();
            const float cardW = cardH * 0.78f;
            const float lift = over ? -4.0f : 0.0f;
            for (int i = n - 1; i >= 0; --i)
            {
                const float spread = (float) (i - (n - 1) * 0.5f);
                const float angle = spread * 0.11f * (over ? 1.25f : 1.0f);
                auto cardRect = juce::Rectangle<float> (cardW, cardH).withCentre (stage.getCentre().translated (spread * cardW * 0.52f, lift));
                juce::Graphics::ScopedSaveState ss (g);
                g.addTransform (juce::AffineTransform::rotation (angle, cardRect.getCentreX(), cardRect.getBottom() + cardH * 0.4f));
                drawPaperCard (g, cardRect, 9.0f, true, true);
                if (auto* chord = ChordLibrary::get().find (deck.chordIds[i]))
                {
                    g.setColour (cardInk);
                    g.setFont (Theme::cond (cardH * 0.13f, Theme::Weight::bold));
                    g.drawText (chord->symbol, cardRect.withHeight (cardH * 0.2f).translated (0, 4.0f), juce::Justification::centred, false);
                    ChordDiagram::Style st;
                    st.showFingers = false;
                    ChordDiagram::draw (g, cardRect.withTrimmedTop (cardH * 0.22f).reduced (cardW * 0.08f, cardH * 0.05f), *chord, st);
                }
            }
            juce::ignoreUnused (down);

            // Title, description, mastery
            auto textArea = r.reduced (20.0f, 4.0f);
            g.setColour (ink);
            g.setFont (Theme::sans (19.0f, Theme::Weight::semibold));
            g.drawText (deck.title, textArea.removeFromTop (26.0f), juce::Justification::centredLeft, true);
            g.setColour (ink2);
            g.setFont (Theme::sans (14.5f));
            g.drawFittedText (deck.description.isNotEmpty() ? deck.description : juce::String ("Your own selection of chords."),
                              textArea.removeFromTop (38.0f).toNearestInt(), juce::Justification::topLeft, 2, 1.0f);

            auto footer = textArea.removeFromBottom (34.0f);
            g.setColour (ink3);
            g.setFont (Theme::sans (13.5f, Theme::Weight::medium));
            const int mastered = ctx.progress.countAtLevel (deck, 3);
            g.drawText (juce::String (deck.chordIds.size()) + " chords", footer.removeFromTop (16.0f), juce::Justification::centredLeft, false);
            g.drawText (mastered > 0 ? juce::String (mastered) + " mastered" : juce::String ("Not started"),
                        footer.withHeight (16.0f).translated (0, -16.0f), juce::Justification::centredRight, false);

            // Segmented mastery bar: mastered, familiar, learning, new
            auto bar = footer.withSizeKeepingCentre (footer.getWidth(), 6.0f).translated (0, 2.0f);
            const float total = (float) juce::jmax (1, deck.chordIds.size());
            const juce::Colour cols[] { good, brass, glow.withAlpha (0.8f) };
            g.setColour (line);
            g.fillRoundedRectangle (bar, 3.0f);
            float x = bar.getX();
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addRoundedRectangle (bar, 3.0f);
            g.reduceClipRegion (clip);
            for (int level = 3; level >= 1; --level)
            {
                const float w = bar.getWidth() * (float) ctx.progress.countAtLevel (deck, level) / total;
                g.setColour (cols[3 - level]);
                g.fillRect (juce::Rectangle<float> (x, bar.getY(), w, bar.getHeight()));
                x += w;
            }
        }

        AppContext& ctx;
        Deck deck;
        juce::TextButton editButton { "Edit" };
        std::function<void()> onEdit;
    };

    struct NewDeckTile : public juce::Button
    {
        NewDeckTile() : juce::Button ("New deck")
        {
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
            setWantsKeyboardFocus (true);
        }

        void paintButton (juce::Graphics& g, bool over, bool) override
        {
            using namespace Theme::Colours;
            auto r = getLocalBounds().toFloat().reduced (2.0f);
            juce::Path outline;
            outline.addRoundedRectangle (r.reduced (0.5f), Theme::radiusLg);
            juce::Path dashed;
            const float dashes[] { 7.0f, 6.0f };
            juce::PathStrokeType (1.4f).createDashedStroke (dashed, outline, dashes, 2);
            g.setColour (over || hasKeyboardFocus (false) ? brass : line);
            g.fillPath (dashed);

            auto c = r.getCentre();
            Icons::draw (g, Icons::plus(), juce::Rectangle<float> (34, 34).withCentre (c.translated (0, -22)), over ? brass : ink2, 2.0f);
            g.setColour (ink);
            g.setFont (Theme::sans (18.0f, Theme::Weight::semibold));
            g.drawText ("Make your own deck", r.withHeight (26).withCentre (c.translated (0, 16)), juce::Justification::centred, false);
            g.setColour (ink2);
            g.setFont (Theme::sans (14.0f));
            g.drawText ("Pick the chords from a song you're learning", r.withHeight (20).withCentre (c.translated (0, 40)),
                        juce::Justification::centred, false);
        }
    };
}

//==============================================================================
struct DeckBrowser::Content : public juce::Component
{
    explicit Content (DeckBrowser& o) : owner (o) {}

    void rebuild()
    {
        tiles.clear();
        auto addTile = [this] (const Deck& d)
        {
            auto* t = new DeckTile (owner.ctx, d);
            t->onClick = [this, d] { if (owner.onOpen) owner.onOpen (d); };
            t->onEdit = [this, d] { if (owner.onEdit) owner.onEdit (&d); };
            tiles.add (t);
            addAndMakeVisible (t);
        };

        for (auto& d : ChordLibrary::get().builtInDecks()) addTile (d);
        builtInCount = tiles.size();
        for (auto& d : owner.ctx.progress.customDecks()) addTile (d);

        auto* nt = new NewDeckTile();
        nt->onClick = [this] { if (owner.onEdit) owner.onEdit (nullptr); };
        tiles.add (nt);
        addAndMakeVisible (nt);
        layout (getWidth());
    }

    int layout (int width)
    {
        const int margin = Theme::s6, gap = 20;
        const int cols = juce::jmax (2, (width - margin * 2 + gap) / (300 + gap));
        const int tileW = (width - margin * 2 - gap * (cols - 1)) / cols;
        const int tileH = juce::jmax (300, (int) (tileW * 0.84f));

        int y = 120;
        auto place = [&] (int from, int to)
        {
            for (int i = from; i < to; ++i)
            {
                const int col = (i - from) % cols, row = (i - from) / cols;
                tiles[i]->setBounds (margin + col * (tileW + gap), y + row * (tileH + gap), tileW, tileH);
            }
            const int rows = (to - from + cols - 1) / cols;
            y += rows * (tileH + gap);
        };

        place (0, builtInCount);
        yourDecksY = y + 14;
        y += 62;
        place (builtInCount, tiles.size());
        return y + margin;
    }

    void paint (juce::Graphics& g) override
    {
        auto top = juce::Rectangle<int> (Theme::s6, 34, getWidth() - Theme::s6 * 2, 80);
        drawHeading (g, top, "Learn chords", "Pick a deck. Flip the cards, quiz yourself, then prove it on your guitar.");

        g.setColour (Theme::Colours::ink);
        g.setFont (Theme::sans (20.0f, Theme::Weight::semibold));
        g.drawText ("Your decks", Theme::s6, yourDecksY, 400, 30, juce::Justification::centredLeft);
    }

    DeckBrowser& owner;
    juce::OwnedArray<juce::Button> tiles;
    int builtInCount = 0, yourDecksY = 0;
};

DeckBrowser::DeckBrowser (AppContext& c) : ctx (c), content (std::make_unique<Content> (*this))
{
    viewport.setViewedComponent (content.get(), false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    addAndMakeVisible (viewport);
    content->rebuild();
}

DeckBrowser::~DeckBrowser() = default;

void DeckBrowser::refresh()
{
    content->rebuild();
    resized();
}

void DeckBrowser::resized()
{
    viewport.setBounds (getLocalBounds());
    const int w = getWidth() - viewport.getScrollBarThickness();
    content->setSize (w, content->layout (w));
}

//==============================================================================
DeckEditor::DeckEditor (AppContext& c, const Deck* existing) : ctx (c), isNew (existing == nullptr)
{
    if (existing != nullptr) deck = *existing;
    else
    {
        deck.id = "custom-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64());
        deck.builtIn = false;
    }

    nameEditor.setText (deck.title, false);
    nameEditor.setTextToShowWhenEmpty ("Deck name, e.g. \"Wonderwall\"", Theme::Colours::ink3);
    nameEditor.setFont (Theme::sans (18.0f, Theme::Weight::medium));
    nameEditor.setIndents (12, 10);
    nameEditor.onTextChange = [this] { updateSaveState(); };
    addAndMakeVisible (nameEditor);

    searchEditor.setTextToShowWhenEmpty ("Filter chords", Theme::Colours::ink3);
    searchEditor.setFont (Theme::sans (15.0f));
    searchEditor.setIndents (12, 8);
    searchEditor.onTextChange = [this] { layoutChips(); };
    addAndMakeVisible (searchEditor);

    for (auto& chord : ChordLibrary::get().all())
    {
        auto* b = chips.add (new juce::TextButton (chord.symbol));
        variant (*b, "chip");
        b->setClickingTogglesState (true);
        b->setToggleState (deck.chordIds.contains (chord.id), juce::dontSendNotification);
        b->setTooltip (chord.name);
        b->getProperties().set ("chordId", chord.id);
        b->onClick = [this] { updateSaveState(); };
        chipHolder.addAndMakeVisible (b);
    }
    chipView.setViewedComponent (&chipHolder, false);
    chipView.setScrollBarsShown (true, false);
    addAndMakeVisible (chipView);

    variant (saveButton, "primary");
    variant (cancelButton, "ghost");
    variant (deleteButton, "danger");
    saveButton.onClick = [this] { save(); };
    cancelButton.onClick = [this] { if (onClose) onClose ({}); };
    deleteButton.onClick = [this]
    {
        ctx.progress.deleteCustomDeck (deck.id);
        if (onClose) onClose ({});
    };
    addAndMakeVisible (saveButton);
    addAndMakeVisible (cancelButton);
    addChildComponent (deleteButton);
    deleteButton.setVisible (! isNew);
    setWantsKeyboardFocus (true);
    updateSaveState();
}

void DeckEditor::updateSaveState()
{
    int n = 0;
    for (auto* b : chips) if (b->getToggleState()) ++n;
    saveButton.setEnabled (nameEditor.getText().trim().isNotEmpty() && n >= 2);
    saveButton.setButtonText (n >= 2 ? "Save deck (" + juce::String (n) + ")" : "Pick at least 2 chords");
    repaint();
}

void DeckEditor::save()
{
    deck.title = nameEditor.getText().trim();
    deck.chordIds.clear();
    for (auto* b : chips)
        if (b->getToggleState()) deck.chordIds.add (b->getProperties()["chordId"].toString());
    deck.description = "Your own selection of " + juce::String (deck.chordIds.size()) + " chords.";
    ctx.progress.saveCustomDeck (deck);
    if (onClose) onClose (deck.id);
}

void DeckEditor::layoutChips()
{
    const auto filter = searchEditor.getText().trim();
    const int w = chipView.getWidth() - 12;
    int x = 0, y = 0;
    juce::String lastCategory;
    const auto& all = ChordLibrary::get().all();
    chipHolder.rows.clear();

    for (int i = 0; i < chips.size(); ++i)
    {
        const auto& chord = all[(size_t) i];
        const bool show = filter.isEmpty() || chord.symbol.containsIgnoreCase (filter) || chord.name.containsIgnoreCase (filter);
        chips[i]->setVisible (show);
        if (! show) continue;

        if (chord.category != lastCategory)
        {
            if (lastCategory.isNotEmpty()) y += 44;
            x = 0;
            chipHolder.rows.push_back ({ chord.category, y });
            y += 26;
            lastCategory = chord.category;
        }
        const int cw = 76;
        if (x + cw > w) { x = 0; y += 42; }
        chips[i]->setBounds (x, y, cw - 8, 34);
        x += cw;
    }
    chipHolder.setSize (w, y + 44);
    chipHolder.repaint();
}

void DeckEditor::paint (juce::Graphics& g)
{
    using namespace Theme::Colours;
    g.fillAll (juce::Colours::black.withAlpha (0.55f));
    Theme::dropShadow (g, panel.toFloat(), 18.0f, 0.8f);
    g.setColour (Theme::Colours::panel);
    g.fillRoundedRectangle (panel.toFloat(), 18.0f);

    auto r = panel.reduced (28, 24);
    g.setColour (ink);
    g.setFont (Theme::sans (24.0f, Theme::Weight::bold));
    g.drawText (isNew ? "New deck" : "Edit deck", r.removeFromTop (34), juce::Justification::centredLeft);
}

void DeckEditor::ChipHolder::paint (juce::Graphics& g)
{
    g.setColour (Theme::Colours::ink2);
    g.setFont (Theme::sans (14.0f, Theme::Weight::semibold));
    for (auto& [name, y] : rows)
        g.drawText (name, 0, y, 300, 22, juce::Justification::centredLeft);
}

void DeckEditor::resized()
{
    panel = getLocalBounds().withSizeKeepingCentre (juce::jmin (720, getWidth() - 80), juce::jmin (640, getHeight() - 80));
    auto r = panel.reduced (28, 24);
    r.removeFromTop (46);
    nameEditor.setBounds (r.removeFromTop (46));
    r.removeFromTop (14);
    searchEditor.setBounds (r.removeFromTop (38).withWidth (260));
    r.removeFromTop (12);

    auto buttons = r.removeFromBottom (44);
    saveButton.setBounds (buttons.removeFromRight (220));
    buttons.removeFromRight (10);
    cancelButton.setBounds (buttons.removeFromRight (110));
    deleteButton.setBounds (buttons.removeFromLeft (140));
    r.removeFromBottom (14);

    // Leave room above each category row for its caption (painted by this component)
    chipView.setBounds (r);
    layoutChips();
}

void DeckEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! panel.contains (e.getPosition()) && onClose) onClose ({});
}

bool DeckEditor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) { if (onClose) onClose ({}); return true; }
    return false;
}

} // namespace gg
