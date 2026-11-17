#pragma once

#include "ChordLibrary.h"
#include <map>

namespace gg
{

/** Per-chord learning stats, persisted to JSON. */
struct ChordStats
{
    int correct = 0;
    int wrong = 0;
    int streak = 0;        // consecutive correct answers
    juce::int64 lastSeenMs = 0;

    /** 0 = new, 1 = learning, 2 = familiar, 3 = mastered */
    int level() const noexcept
    {
        if (correct + wrong == 0) return 0;
        if (streak >= 4) return 3;
        if (streak >= 2) return 2;
        return 1;
    }
};

class StudyProgress
{
public:
    explicit StudyProgress (juce::File storageFile = {});

    static juce::File defaultStorageFile();

    const ChordStats& stats (const juce::String& chordId) const;
    void recordAnswer (const juce::String& chordId, bool wasCorrect);

    /** 0..1 fraction of the deck's chords at level >= 2 (weighted). */
    float deckMastery (const Deck& deck) const;
    int countAtLevel (const Deck& deck, int level) const;

    // Custom decks --------------------------------------------------------
    const std::vector<Deck>& customDecks() const noexcept { return userDecks; }
    void saveCustomDeck (const Deck& deck);   // insert or replace by id
    void deleteCustomDeck (const juce::String& deckId);

    // Match best times ----------------------------------------------------
    double bestMatchTime (const juce::String& deckId) const;  // seconds, 0 = none
    bool submitMatchTime (const juce::String& deckId, double seconds); // true if new record

    void resetAll();

    bool save() const;
    bool load();

    juce::var toVar() const;
    void fromVar (const juce::var& v);

    std::function<void()> onChange;

private:
    juce::File file;
    std::map<juce::String, ChordStats> chordStats;
    std::vector<Deck> userDecks;
    std::map<juce::String, double> matchTimes;

    void changed();
};

} // namespace gg
