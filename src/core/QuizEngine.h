#pragma once

#include "ChordLibrary.h"

namespace gg
{

/** Quizlet-style flashcard pass through a deck with "still learning" / "know it" sorting. */
class FlashcardSession
{
public:
    FlashcardSession() = default;
    FlashcardSession (juce::StringArray chordIds, bool shuffle, juce::int64 seed = juce::Time::currentTimeMillis());

    int size() const noexcept { return order.size(); }
    int position() const noexcept { return index; }
    bool finished() const noexcept { return index >= order.size(); }
    juce::String current() const { return finished() ? juce::String() : order[index]; }

    /** Marks the current card and advances. */
    void mark (bool known);
    /** Steps back one card, undoing its mark. */
    bool back();

    const juce::StringArray& knownCards() const noexcept { return known; }
    const juce::StringArray& learningCards() const noexcept { return learning; }

private:
    juce::StringArray order, known, learning;
    int index = 0;
};

/** Multiple choice "Learn" mode: every chord must be answered correctly twice in a row. */
class LearnSession
{
public:
    enum class QuestionType { DiagramToName, NameToDiagram };

    struct Question
    {
        QuestionType type = QuestionType::DiagramToName;
        juce::String target;
        juce::StringArray options;   // 4 chord ids (fewer if the library is tiny)
        int correctIndex = 0;
    };

    static constexpr int requiredStreak = 2;

    LearnSession() = default;
    LearnSession (juce::StringArray chordIds, juce::int64 seed = juce::Time::currentTimeMillis());

    bool finished() const;
    const Question& question() const noexcept { return current; }

    /** Returns true if correct. Call next() to move on. */
    bool answer (int optionIndex);
    void next();

    int masteredCount() const;
    int total() const noexcept { return ids.size(); }
    int streakFor (const juce::String& id) const;
    int questionsAsked() const noexcept { return asked; }
    int correctAnswers() const noexcept { return correctCount; }

private:
    juce::StringArray ids;
    std::vector<int> streaks;
    juce::Random rng;
    Question current;
    juce::String lastTarget;
    int asked = 0, correctCount = 0;

    void buildQuestion();
};

/** Match game: pairs of chord names and diagrams, shuffled into a grid. */
class MatchGame
{
public:
    struct Tile { juce::String chordId; bool isDiagram = false; bool matched = false; };

    MatchGame() = default;
    MatchGame (juce::StringArray chordIds, int pairs, juce::int64 seed = juce::Time::currentTimeMillis());

    const std::vector<Tile>& tiles() const noexcept { return tileList; }

    /** Tries to pair two tiles. Returns true on a match (tiles become matched). */
    bool tryMatch (int a, int b);
    bool complete() const;
    int mistakes() const noexcept { return misses; }

private:
    std::vector<Tile> tileList;
    int misses = 0;
};

} // namespace gg
