#include "ChordLibrary.h"

namespace gg
{

juce::String pitchClassName (int pc)
{
    static const char* names[] { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B" };
    return names[((pc % 12) + 12) % 12];
}

std::array<int, 6> Chord::parseFrets (const juce::String& s)
{
    std::array<int, 6> out {};
    out.fill (-1);

    if (s.containsChar (','))
    {
        auto tokens = juce::StringArray::fromTokens (s, ",", {});
        for (int i = 0; i < 6 && i < tokens.size(); ++i)
        {
            auto t = tokens[i].trim();
            out[(size_t) i] = (t == "x" || t == "X") ? -1 : t.getIntValue();
        }
        return out;
    }

    for (int i = 0; i < 6 && i < s.length(); ++i)
    {
        auto c = s[i];
        out[(size_t) i] = (c == 'x' || c == 'X') ? -1 : (int) (c - '0');
    }
    return out;
}

int Chord::baseFret() const
{
    int minF = 99, maxF = 0;
    for (auto f : frets)
        if (f > 0) { minF = juce::jmin (minF, f); maxF = juce::jmax (maxF, f); }

    if (maxF <= 4) return 1;
    return minF;
}

int Chord::soundingStrings() const
{
    int n = 0;
    for (auto f : frets) if (f >= 0) ++n;
    return n;
}

std::vector<int> Chord::midiNotes() const
{
    std::vector<int> notes;
    for (size_t i = 0; i < 6; ++i)
        if (frets[i] >= 0)
            notes.push_back (standardTuning[i] + frets[i]);
    return notes;
}

int Chord::pitchClassMask() const
{
    int mask = 0;
    for (auto n : midiNotes())
        mask |= 1 << (n % 12);
    return mask;
}

int Chord::rootPitchClass() const
{
    if (symbol.isEmpty()) return 0;
    static const int letterPc[] { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
    auto letter = juce::CharacterFunctions::toUpperCase (symbol[0]);
    if (letter < 'A' || letter > 'G') return 0;
    int pc = letterPc[letter - 'A'];
    if (symbol.length() > 1)
    {
        if (symbol[1] == '#') ++pc;
        else if (symbol[1] == 'b') --pc;
    }
    return (pc + 12) % 12;
}

std::vector<Chord::Barre> Chord::barres() const
{
    std::vector<Barre> result;
    for (int finger = 1; finger <= 4; ++finger)
    {
        int fret = -1, from = -1, to = -1, count = 0;
        for (int s = 0; s < 6; ++s)
        {
            if (fingers[(size_t) s] == finger && frets[(size_t) s] > 0)
            {
                if (fret < 0) fret = frets[(size_t) s];
                if (frets[(size_t) s] != fret) continue;
                if (from < 0) from = s;
                to = s;
                ++count;
            }
        }
        if (count >= 2)
            result.push_back ({ fret, from, to });
    }
    return result;
}

//==============================================================================
namespace
{
    Chord make (const char* id, const char* symbol, const char* name, const char* category,
                const char* frets, const char* fingers)
    {
        Chord c;
        c.id = id;
        c.symbol = symbol;
        c.name = name;
        c.category = category;
        c.frets = Chord::parseFrets (frets);
        auto f = Chord::parseFrets (fingers);
        for (size_t i = 0; i < 6; ++i)
            c.fingers[i] = juce::jmax (0, f[i]);
        return c;
    }
}

ChordLibrary::ChordLibrary()
{
    chords = {
        // Open major / minor
        make ("C",  "C",  "C major", "Open", "x32010", "x32010"),
        make ("A",  "A",  "A major", "Open", "x02220", "x01230"),
        make ("G",  "G",  "G major", "Open", "320003", "210003"),
        make ("E",  "E",  "E major", "Open", "022100", "023100"),
        make ("D",  "D",  "D major", "Open", "xx0232", "xx0132"),
        make ("Am", "Am", "A minor", "Open", "x02210", "x02310"),
        make ("Em", "Em", "E minor", "Open", "022000", "023000"),
        make ("Dm", "Dm", "D minor", "Open", "xx0231", "xx0231"),

        // Dominant & minor sevenths
        make ("C7",  "C7",  "C dominant 7th", "Seventh", "x32310", "x32410"),
        make ("A7",  "A7",  "A dominant 7th", "Seventh", "x02020", "x02030"),
        make ("D7",  "D7",  "D dominant 7th", "Seventh", "xx0212", "xx0213"),
        make ("E7",  "E7",  "E dominant 7th", "Seventh", "020100", "020100"),
        make ("G7",  "G7",  "G dominant 7th", "Seventh", "320001", "320001"),
        make ("B7",  "B7",  "B dominant 7th", "Seventh", "x21202", "x21304"),
        make ("Am7", "Am7", "A minor 7th",    "Seventh", "x02010", "x02010"),
        make ("Em7", "Em7", "E minor 7th",    "Seventh", "020000", "020000"),
        make ("Dm7", "Dm7", "D minor 7th",    "Seventh", "xx0211", "xx0211"),

        // Major sevenths
        make ("Cmaj7", "Cmaj7", "C major 7th", "Major 7th", "x32000", "x32000"),
        make ("Fmaj7", "Fmaj7", "F major 7th", "Major 7th", "xx3210", "xx3210"),
        make ("Amaj7", "Amaj7", "A major 7th", "Major 7th", "x02120", "x02130"),
        make ("Dmaj7", "Dmaj7", "D major 7th", "Major 7th", "xx0222", "xx0111"),
        make ("Gmaj7", "Gmaj7", "G major 7th", "Major 7th", "320002", "320001"),
        make ("Emaj7", "Emaj7", "E major 7th", "Major 7th", "021100", "031200"),

        // Suspended, added & colour chords
        make ("Asus2", "Asus2", "A suspended 2nd", "Sus & add", "x02200", "x01200"),
        make ("Asus4", "Asus4", "A suspended 4th", "Sus & add", "x02230", "x01230"),
        make ("Dsus2", "Dsus2", "D suspended 2nd", "Sus & add", "xx0230", "xx0130"),
        make ("Dsus4", "Dsus4", "D suspended 4th", "Sus & add", "xx0233", "xx0134"),
        make ("Esus4", "Esus4", "E suspended 4th", "Sus & add", "022200", "023400"),
        make ("Cadd9", "Cadd9", "C add 9",         "Sus & add", "x32033", "x21034"),
        make ("Bdim",  "Bdim",  "B diminished",    "Sus & add", "x2343x", "x1243x"),
        make ("Caug",  "Caug",  "C augmented",     "Sus & add", "x32110", "x43110"),

        // Barre chords
        make ("F",   "F",   "F major",  "Barre", "133211",    "134211"),
        make ("Fm",  "Fm",  "F minor",  "Barre", "133111",    "134111"),
        make ("Bb",  "Bb",  "B-flat major", "Barre", "x13331", "x12341"),
        make ("B",   "B",   "B major",  "Barre", "x24442",    "x12341"),
        make ("Bm",  "Bm",  "B minor",  "Barre", "x24432",    "x13421"),
        make ("F#m", "F#m", "F-sharp minor", "Barre", "244222", "134111"),
        make ("Cm",  "Cm",  "C minor",  "Barre", "x35543",    "x13421"),
        make ("Gm",  "Gm",  "G minor",  "Barre", "355333",    "134111"),
        make ("C#m", "C#m", "C-sharp minor", "Barre", "x46654", "x13421"),

        // Power chords
        make ("E5", "E5", "E power chord", "Power", "022xxx", "023xxx"),
        make ("A5", "A5", "A power chord", "Power", "x022xx", "x023xx"),
        make ("F5", "F5", "F power chord", "Power", "133xxx", "134xxx"),
        make ("G5", "G5", "G power chord", "Power", "355xxx", "134xxx"),
        make ("B5", "B5", "B power chord", "Power", "x244xx", "x134xx"),
        make ("C5", "C5", "C power chord", "Power", "x355xx", "x134xx"),
        make ("D5", "D5", "D power chord", "Power", "x577xx", "x134xx"),
    };

    auto idsInCategory = [this] (const juce::String& cat)
    {
        juce::StringArray ids;
        for (auto& c : chords) if (c.category == cat) ids.add (c.id);
        return ids;
    };

    juce::StringArray everything;
    for (auto& c : chords) everything.add (c.id);

    decks = {
        { "first-chords", "Your first chords",
          "The eight open shapes nearly every song starts with.",
          idsInCategory ("Open"), true },
        { "sevenths", "Seventh chords",
          "Bluesy dominant 7ths and mellow minor 7ths.",
          idsInCategory ("Seventh"), true },
        { "major-sevenths", "Major 7th chords",
          "Dreamy, jazzy voicings in open position.",
          idsInCategory ("Major 7th"), true },
        { "sus-add", "Sus, add & colour",
          "Suspended, added-tone, diminished and augmented sounds.",
          idsInCategory ("Sus & add"), true },
        { "barre", "Barre chords",
          "E- and A-shape barres that unlock every key.",
          idsInCategory ("Barre"), true },
        { "power", "Power chords",
          "Two- and three-note shapes for rock and metal.",
          idsInCategory ("Power"), true },
        { "everything", "Every chord",
          "The whole library, shuffled together.",
          everything, true },
    };
}

const ChordLibrary& ChordLibrary::get()
{
    static ChordLibrary instance;
    return instance;
}

const Chord* ChordLibrary::find (const juce::String& id) const
{
    for (auto& c : chords)
        if (c.id == id)
            return &c;
    return nullptr;
}

} // namespace gg
