#include "LearnPage.h"
#include "core/StrumSynth.h"

namespace gg
{

void drawPaperCard (juce::Graphics& g, juce::Rectangle<float> r, float radius, bool shadow, bool ruled)
{
    if (shadow) Theme::dropShadow (g, r, radius, 0.55f);
    g.setColour (Theme::Colours::card);
    g.fillRoundedRectangle (r, radius);
    if (! ruled) return;
    // faint printed rule under the title, like an index card
    g.setColour (juce::Colour (0xffd9c9b0).withAlpha (0.55f));
    g.fillRect (r.reduced (radius, 0).withY (r.getY() + r.getHeight() * 0.2f).withHeight (1.0f));
}

void hearChord (AppContext& ctx, const juce::String& chordId)
{
    if (auto* c = ChordLibrary::get().find (chordId))
        ctx.audio.playPreview (StrumSynth::renderChord (*c, ctx.audio.sampleRate()));
}

//==============================================================================
LearnPage::LearnPage (AppContext& c) : ctx (c), browser (c)
{
    addAndMakeVisible (browser);
    browser.onOpen = [this] (const Deck& d) { openDeck (d); };
    browser.onEdit = [this] (const Deck* d) { showEditor (d); };

    backButton.onClick = [this] { closeDeck(); };
    addChildComponent (backButton);

    for (auto* name : { "Flashcards", "Learn", "Match", "Play along" })
    {
        auto* t = modeTabs.add (new juce::TextButton (name));
        variant (*t, "tab");
        t->setClickingTogglesState (false);
        t->setRadioGroupId (2001);
        const int idx = modeTabs.size() - 1;
        t->onClick = [this, idx] { setMode (idx); };
        addChildComponent (t);
    }
    modeTabs[3]->setTooltip ("Strum the chord on your guitar and GuitarGuider checks it");
    setWantsKeyboardFocus (false);
}

LearnPage::~LearnPage() = default;

void LearnPage::openDeck (const Deck& d, int m)
{
    openDeckData = d;
    studying = true;
    browser.setVisible (false);
    backButton.setVisible (true);
    for (auto* t : modeTabs) t->setVisible (true);
    mode = -1;
    setMode (m);
    repaint();
}

void LearnPage::closeDeck()
{
    studying = false;
    modeView = nullptr;
    backButton.setVisible (false);
    for (auto* t : modeTabs) t->setVisible (false);
    browser.refresh();
    browser.setVisible (true);
    repaint();
}

void LearnPage::setMode (int m)
{
    if (m == mode && modeView != nullptr) return;
    mode = m;
    modeTabs[m]->setToggleState (true, juce::dontSendNotification);

    const auto ids = openDeckData.chordIds;
    switch (m)
    {
        case 0:
        {
            auto v = std::make_unique<FlashcardsView> (ctx, ids);
            v->onSwitchToLearn = [this] { setMode (1); };
            modeView = std::move (v);
            break;
        }
        case 1:  modeView = std::make_unique<LearnModeView> (ctx, ids); break;
        case 2:  modeView = std::make_unique<MatchView> (ctx, ids, openDeckData.id); break;
        default: modeView = std::make_unique<PlayAlongView> (ctx, ids); break;
    }
    addAndMakeVisible (modeView.get());
    resized();
    modeView->grabKeyboardFocus();
}

void LearnPage::showEditor (const Deck* existing)
{
    editor = std::make_unique<DeckEditor> (ctx, existing);
    editor->onClose = [this] (const juce::String&)
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<LearnPage> (this)]
        {
            if (safe != nullptr) { safe->editor = nullptr; safe->browser.refresh(); }
        });
    };
    addAndMakeVisible (editor.get());
    editor->setBounds (getLocalBounds());
    editor->grabKeyboardFocus();
}

void LearnPage::visibilityChanged()
{
    if (isVisible() && ! studying) browser.refresh();
}

void LearnPage::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colours::bg);
    if (! studying) return;

    auto header = getLocalBounds().reduced (Theme::s6, 0).withHeight (84);
    header.removeFromLeft (110);
    g.setColour (Theme::Colours::ink);
    g.setFont (Theme::sans (24.0f, Theme::Weight::bold));
    g.drawText (openDeckData.title, header.removeFromTop (54).withTrimmedTop (18), juce::Justification::bottomLeft, true);
    g.setColour (Theme::Colours::ink2);
    g.setFont (Theme::sans (15.0f));
    g.drawText (juce::String (openDeckData.chordIds.size()) + " chords", header.removeFromTop (22),
                juce::Justification::topLeft, true);

    g.setColour (Theme::Colours::line.withAlpha (0.7f));
    g.fillRect (Theme::s6, 92, getWidth() - Theme::s6 * 2, 1);
}

void LearnPage::resized()
{
    auto r = getLocalBounds();
    browser.setBounds (r);
    if (editor != nullptr) editor->setBounds (r);

    auto header = r.removeFromTop (92).reduced (Theme::s6, 0);
    backButton.setBounds (header.removeFromLeft (96).withSizeKeepingCentre (96, 40).translated (0, 4));

    auto tabs = header.removeFromRight (440).withTrimmedTop (30).withTrimmedBottom (0);
    const int tw = tabs.getWidth() / juce::jmax (1, modeTabs.size());
    for (auto* t : modeTabs) t->setBounds (tabs.removeFromLeft (tw));

    if (modeView != nullptr) modeView->setBounds (r);
}

void LearnPage::prepareShot (int index)
{
    editor = nullptr;
    const auto& decks = ChordLibrary::get().builtInDecks();
    switch (index)
    {
        case 0: closeDeck(); break;
        case 1: openDeck (decks[0], 0); break;
        case 2: openDeck (decks[0], 0); if (auto* f = dynamic_cast<FlashcardsView*> (modeView.get())) f->showBackForScreenshot(); break;
        case 3: openDeck (decks[0], 0); if (auto* f = dynamic_cast<FlashcardsView*> (modeView.get())) f->finishForScreenshot(); break;
        case 4: openDeck (decks[1], 1); break;
        case 5: openDeck (decks[1], 1); if (auto* l = dynamic_cast<LearnModeView*> (modeView.get())) l->answerForScreenshot (false); break;
        case 6: openDeck (decks[0], 2); if (auto* m = dynamic_cast<MatchView*> (modeView.get())) m->startGame(); break;
        default: openDeck (decks[0], 3); break;
    }
}

} // namespace gg
