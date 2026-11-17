#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>

namespace gg
{

/** A single guitar chord voicing.
    Strings are ordered low E (index 0) to high E (index 5).
    fret == -1 means the string is muted, 0 means open.
    finger == 0 means no finger (open or muted), 1-4 index..pinky, 5 = thumb. */
struct Chord
{
    juce::String id;          // unique, e.g. "G", "Am7", "F-barre"
    juce::String name;        // display name, e.g. "G major"
    juce::String symbol;      // short symbol, e.g. "G", "Am7"
    juce::String category;    // e.g. "Open", "Seventh"
    std::array<int, 6> frets {};
    std::array<int, 6> fingers {};

    /** Lowest fretted (non-open) fret, or 1 when the chord fits in the first 4 frets. */
    int baseFret() const;

    /** Number of strings that sound. */
    int soundingStrings() const;

    /** MIDI note numbers of the sounding strings (standard tuning). */
    std::vector<int> midiNotes() const;

    /** Bitmask of the 12 pitch classes in the chord (bit 0 = C). */
    int pitchClassMask() const;

    /** Root pitch class (0 = C) derived from the symbol. */
    int rootPitchClass() const;

    struct Barre { int fret = 0; int fromString = 0; int toString = 0; };
    /** Detects a barre: the same finger holding the same fret on 2+ strings. */
    std::vector<Barre> barres() const;

    /** Parse helpers. Frets string like "x32010"; for frets >= 10 use comma form "x,10,12,12,12,10". */
    static std::array<int, 6> parseFrets (const juce::String& s);
};

/** Standard tuning MIDI notes, low to high. */
inline constexpr std::array<int, 6> standardTuning { 40, 45, 50, 55, 59, 64 };

juce::String pitchClassName (int pc);

/** A named collection of chord ids. */
struct Deck
{
    juce::String id;
    juce::String title;
    juce::String description;
    juce::StringArray chordIds;
    bool builtIn = true;
};

class ChordLibrary
{
public:
    static const ChordLibrary& get();

    const std::vector<Chord>& all() const noexcept { return chords; }
    const Chord* find (const juce::String& id) const;
    const std::vector<Deck>& builtInDecks() const noexcept { return decks; }

private:
    ChordLibrary();
    std::vector<Chord> chords;
    std::vector<Deck> decks;
};

} // namespace gg
