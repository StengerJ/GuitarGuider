#pragma once

#include "AppContext.h"
#include "core/QuizEngine.h"
#include "core/ChordRecognizer.h"
#include "ui/Widgets.h"
#include "ui/ChordDiagram.h"

namespace gg
{

/** Paints the printed chord-chart card surface used across the Learn section. */
void drawPaperCard (juce::Graphics&, juce::Rectangle<float> r, float radius = Theme::radiusLg, bool shadow = true, bool ruled = false);

/** Plays a synthesized strum of the chord through the audio engine. */
void hearChord (AppContext&, const juce::String& chordId);

//==============================================================================
class DeckBrowser : public juce::Component
{
public:
    explicit DeckBrowser (AppContext&);
    ~DeckBrowser() override;
    void refresh();
    void resized() override;

    std::function<void (const Deck&)> onOpen;
    std::function<void (const Deck*)> onEdit;   // nullptr = new deck

private:
    struct Content;
    AppContext& ctx;
    juce::Viewport viewport;
    std::unique_ptr<Content> content;
};

//==============================================================================
class DeckEditor : public juce::Component
{
public:
    DeckEditor (AppContext&, const Deck* existing);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    std::function<void (const juce::String& savedDeckId)> onClose;  // empty id = cancelled/deleted

private:
    AppContext& ctx;
    Deck deck;
    bool isNew;
    juce::TextEditor nameEditor, searchEditor;
    juce::OwnedArray<juce::TextButton> chips;
    struct ChipHolder : public juce::Component
    {
        std::vector<std::pair<juce::String, int>> rows;   // category caption + y
        void paint (juce::Graphics&) override;
    };
    juce::Viewport chipView;
    ChipHolder chipHolder;
    juce::TextButton saveButton { "Save deck" }, cancelButton { "Cancel" }, deleteButton { "Delete deck" };
    juce::Rectangle<int> panel;

    void layoutChips();
    void updateSaveState();
    void save();
};

//==============================================================================
class FlashcardsView : public juce::Component, private juce::Timer
{
public:
    FlashcardsView (AppContext&, juce::StringArray chordIds);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void flip();
    void showBackForScreenshot() { showingBack = true; flipProgress = 1.0f; repaint(); }
    void finishForScreenshot();

private:
    AppContext& ctx;
    juce::StringArray deck;
    FlashcardSession session;
    bool shuffle = true, startWithDiagram = false;
    bool showingBack = false;
    float flipProgress = 1.0f;     // 0..1 animation (0.5 = edge-on)
    bool flipTarget = false;
    float slide = 0.0f;            // card exit animation, -1..1
    juce::Rectangle<float> cardArea;

    IconButton learningButton { "Still learning", Icons::close(), "Still learning" };
    IconButton knowButton { "Know it", Icons::check(), "Know it" };
    IconButton backButton { "Previous card", Icons::arrowLeft() };
    IconButton hearButton { "Hear it", Icons::speaker(), "Hear it" };
    IconButton shuffleButton { "Shuffle", Icons::shuffle() };
    juce::TextButton frontChip { "Name first" };
    juce::TextButton restartButton { "Restart" }, reviewButton, learnModeButton { "Quiz me in Learn mode" };

    void restart (juce::StringArray ids);
    void mark (bool known);
    void updateButtons();
    void timerCallback() override;
    void drawFace (juce::Graphics&, juce::Rectangle<float> r, bool back, const Chord&);

public:
    std::function<void()> onSwitchToLearn;
};

//==============================================================================
class LearnModeView : public juce::Component, private juce::Timer
{
public:
    LearnModeView (AppContext&, juce::StringArray chordIds);
    ~LearnModeView() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    void answerForScreenshot (bool correct);

private:
    struct Option;
    AppContext& ctx;
    juce::StringArray deck;
    LearnSession session;
    juce::OwnedArray<Option> options;
    int chosen = -1;
    bool answered = false, wasCorrect = false;
    juce::TextButton continueButton { "Continue" }, againButton { "Study again" };
    IconButton hearButton { "Hear it", Icons::speaker(), "Hear it" };
    juce::Rectangle<float> promptArea;

    void buildOptions();
    void choose (int index);
    void advance();
    void timerCallback() override;
};

//==============================================================================
class MatchView : public juce::Component, private juce::Timer
{
public:
    MatchView (AppContext&, juce::StringArray chordIds, juce::String deckId);
    ~MatchView() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    void startGame();
    void finishForScreenshot();

private:
    struct Tile;
    AppContext& ctx;
    juce::StringArray deck;
    juce::String deckId;
    MatchGame game;
    juce::OwnedArray<Tile> tiles;
    int selected = -1;
    enum class State { intro, playing, done } state = State::intro;
    double startTime = 0.0, finalTime = 0.0, penalty = 0.0;
    bool newRecord = false;
    juce::TextButton startButton { "Start game" };

    void tileClicked (int index);
    void timerCallback() override;
};

//==============================================================================
class PlayAlongView : public juce::Component, private juce::Timer
{
public:
    PlayAlongView (AppContext&, juce::StringArray chordIds);
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    AppContext& ctx;
    juce::StringArray deck;
    juce::Random rng;
    juce::String target;
    std::unique_ptr<ChordRecognizer> recognizer;
    std::vector<float> buffer;
    std::array<float, 12> shownChroma {};
    int heardFrames = 0;
    double successAt = 0.0;
    int streak = 0, completed = 0;
    float targetScore = 0.0f;
    juce::String heardChord;

    IconButton skipButton { "Next chord", Icons::arrowRight(), "Skip" };
    IconButton hearButton { "Hear it", Icons::speaker(), "Hear it" };
    juce::TextButton settingsButton { "Set up guitar input" }, demoButton { "Use demo guitar" };

    void nextChord();
    void timerCallback() override;
};

} // namespace gg
