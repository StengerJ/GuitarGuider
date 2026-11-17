#pragma once

#include "LearnViews.h"
#include "ui/Shootable.h"

namespace gg
{

class LearnPage : public juce::Component, public Shootable
{
public:
    explicit LearnPage (AppContext&);
    ~LearnPage() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    void openDeck (const Deck&, int mode = 0);
    void closeDeck();

    int numShots() const override { return 8; }
    void prepareShot (int index) override;

private:
    AppContext& ctx;
    DeckBrowser browser;
    Deck openDeckData;
    bool studying = false;
    int mode = 0;

    IconButton backButton { "Back to decks", Icons::arrowLeft(), "Decks" };
    juce::OwnedArray<juce::TextButton> modeTabs;
    std::unique_ptr<juce::Component> modeView;
    std::unique_ptr<DeckEditor> editor;

    void setMode (int m);
    void showEditor (const Deck* existing);
};

} // namespace gg
