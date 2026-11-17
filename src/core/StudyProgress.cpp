#include "StudyProgress.h"

namespace gg
{

StudyProgress::StudyProgress (juce::File storageFile) : file (std::move (storageFile))
{
    if (file != juce::File())
        load();
}

juce::File StudyProgress::defaultStorageFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("GuitarGuider").getChildFile ("progress.json");
}

const ChordStats& StudyProgress::stats (const juce::String& chordId) const
{
    static const ChordStats empty;
    auto it = chordStats.find (chordId);
    return it != chordStats.end() ? it->second : empty;
}

void StudyProgress::recordAnswer (const juce::String& chordId, bool wasCorrect)
{
    auto& s = chordStats[chordId];
    if (wasCorrect) { ++s.correct; ++s.streak; }
    else            { ++s.wrong; s.streak = 0; }
    s.lastSeenMs = juce::Time::currentTimeMillis();
    changed();
}

float StudyProgress::deckMastery (const Deck& deck) const
{
    if (deck.chordIds.isEmpty()) return 0.0f;
    float total = 0.0f;
    for (auto& id : deck.chordIds)
        total += (float) stats (id).level() / 3.0f;
    return total / (float) deck.chordIds.size();
}

int StudyProgress::countAtLevel (const Deck& deck, int level) const
{
    int n = 0;
    for (auto& id : deck.chordIds)
        if (stats (id).level() == level) ++n;
    return n;
}

void StudyProgress::saveCustomDeck (const Deck& deck)
{
    for (auto& d : userDecks)
        if (d.id == deck.id) { d = deck; d.builtIn = false; changed(); return; }

    userDecks.push_back (deck);
    userDecks.back().builtIn = false;
    changed();
}

void StudyProgress::deleteCustomDeck (const juce::String& deckId)
{
    userDecks.erase (std::remove_if (userDecks.begin(), userDecks.end(),
                                     [&] (const Deck& d) { return d.id == deckId; }),
                     userDecks.end());
    changed();
}

double StudyProgress::bestMatchTime (const juce::String& deckId) const
{
    auto it = matchTimes.find (deckId);
    return it != matchTimes.end() ? it->second : 0.0;
}

bool StudyProgress::submitMatchTime (const juce::String& deckId, double seconds)
{
    auto best = bestMatchTime (deckId);
    if (best <= 0.0 || seconds < best)
    {
        matchTimes[deckId] = seconds;
        changed();
        return true;
    }
    return false;
}

void StudyProgress::resetAll()
{
    chordStats.clear();
    matchTimes.clear();
    changed();
}

void StudyProgress::changed()
{
    if (file != juce::File())
        save();
    if (onChange)
        onChange();
}

juce::var StudyProgress::toVar() const
{
    auto* root = new juce::DynamicObject();

    auto* statsObj = new juce::DynamicObject();
    for (auto& [id, s] : chordStats)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("correct", s.correct);
        o->setProperty ("wrong", s.wrong);
        o->setProperty ("streak", s.streak);
        o->setProperty ("lastSeen", s.lastSeenMs);
        statsObj->setProperty (id, juce::var (o));
    }
    root->setProperty ("chords", juce::var (statsObj));

    juce::Array<juce::var> decksArr;
    for (auto& d : userDecks)
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", d.id);
        o->setProperty ("title", d.title);
        o->setProperty ("description", d.description);
        juce::Array<juce::var> ids;
        for (auto& id : d.chordIds) ids.add (id);
        o->setProperty ("chords", ids);
        decksArr.add (juce::var (o));
    }
    root->setProperty ("decks", decksArr);

    auto* times = new juce::DynamicObject();
    for (auto& [id, t] : matchTimes)
        times->setProperty (id, t);
    root->setProperty ("matchTimes", juce::var (times));

    root->setProperty ("version", 1);
    return juce::var (root);
}

void StudyProgress::fromVar (const juce::var& v)
{
    chordStats.clear();
    userDecks.clear();
    matchTimes.clear();

    if (auto* chordsObj = v["chords"].getDynamicObject())
    {
        for (auto& p : chordsObj->getProperties())
        {
            ChordStats s;
            s.correct = (int) p.value["correct"];
            s.wrong = (int) p.value["wrong"];
            s.streak = (int) p.value["streak"];
            s.lastSeenMs = (juce::int64) p.value["lastSeen"];
            chordStats[p.name.toString()] = s;
        }
    }

    if (auto* arr = v["decks"].getArray())
    {
        for (auto& dv : *arr)
        {
            Deck d;
            d.id = dv["id"].toString();
            d.title = dv["title"].toString();
            d.description = dv["description"].toString();
            d.builtIn = false;
            if (auto* ids = dv["chords"].getArray())
                for (auto& id : *ids)
                    if (ChordLibrary::get().find (id.toString()) != nullptr)
                        d.chordIds.add (id.toString());
            if (d.id.isNotEmpty())
                userDecks.push_back (d);
        }
    }

    if (auto* times = v["matchTimes"].getDynamicObject())
        for (auto& p : times->getProperties())
            matchTimes[p.name.toString()] = (double) p.value;
}

bool StudyProgress::save() const
{
    if (file == juce::File()) return false;
    file.getParentDirectory().createDirectory();
    return file.replaceWithText (juce::JSON::toString (toVar()));
}

bool StudyProgress::load()
{
    if (! file.existsAsFile()) return false;
    auto parsed = juce::JSON::parse (file.loadFileAsString());
    if (! parsed.isObject()) return false;
    fromVar (parsed);
    return true;
}

} // namespace gg
