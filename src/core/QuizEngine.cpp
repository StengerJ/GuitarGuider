#include "QuizEngine.h"

namespace gg
{

namespace
{
    void shuffleArray (juce::StringArray& arr, juce::Random& rng)
    {
        for (int i = arr.size() - 1; i > 0; --i)
            arr.strings.swap (i, rng.nextInt (i + 1));
    }
}

//==============================================================================
FlashcardSession::FlashcardSession (juce::StringArray chordIds, bool shuffle, juce::int64 seed)
    : order (std::move (chordIds))
{
    if (shuffle)
    {
        juce::Random rng (seed);
        shuffleArray (order, rng);
    }
}

void FlashcardSession::mark (bool isKnown)
{
    if (finished()) return;
    auto id = order[index];
    known.removeString (id);
    learning.removeString (id);
    (isKnown ? known : learning).add (id);
    ++index;
}

bool FlashcardSession::back()
{
    if (index == 0) return false;
    --index;
    auto id = order[index];
    known.removeString (id);
    learning.removeString (id);
    return true;
}

//==============================================================================
LearnSession::LearnSession (juce::StringArray chordIds, juce::int64 seed)
    : ids (std::move (chordIds)), rng (seed)
{
    streaks.assign ((size_t) ids.size(), 0);
    if (! ids.isEmpty())
        buildQuestion();
}

bool LearnSession::finished() const
{
    return masteredCount() >= ids.size();
}

int LearnSession::masteredCount() const
{
    int n = 0;
    for (auto s : streaks) if (s >= requiredStreak) ++n;
    return n;
}

int LearnSession::streakFor (const juce::String& id) const
{
    auto i = ids.indexOf (id);
    return i >= 0 ? streaks[(size_t) i] : 0;
}

bool LearnSession::answer (int optionIndex)
{
    auto i = ids.indexOf (current.target);
    bool ok = optionIndex == current.correctIndex;
    ++asked;
    if (ok) ++correctCount;
    if (i >= 0)
        streaks[(size_t) i] = ok ? streaks[(size_t) i] + 1 : 0;
    return ok;
}

void LearnSession::next()
{
    if (! finished())
        buildQuestion();
}

void LearnSession::buildQuestion()
{
    // Pick a target among unmastered chords, preferring lower streaks and avoiding repeats.
    juce::StringArray candidates;
    int lowest = 1000;
    for (int i = 0; i < ids.size(); ++i)
        if (streaks[(size_t) i] < requiredStreak)
            lowest = juce::jmin (lowest, streaks[(size_t) i]);

    for (int i = 0; i < ids.size(); ++i)
        if (streaks[(size_t) i] < requiredStreak && (streaks[(size_t) i] == lowest || rng.nextFloat() < 0.35f))
            candidates.add (ids[i]);

    if (candidates.size() > 1)
        candidates.removeString (lastTarget);
    if (candidates.isEmpty())
        return;

    current = {};
    current.target = candidates[rng.nextInt (candidates.size())];
    current.type = rng.nextBool() ? QuestionType::DiagramToName : QuestionType::NameToDiagram;
    lastTarget = current.target;

    // Distractors: deck chords first, then the same category, then anything. Distinct symbols only.
    auto& lib = ChordLibrary::get();
    const auto* target = lib.find (current.target);
    juce::StringArray pool, deckPool, categoryPool;

    for (auto& id : ids)
        if (id != current.target) deckPool.add (id);
    for (auto& c : lib.all())
        if (target != nullptr && c.category == target->category && c.id != current.target && ! deckPool.contains (c.id))
            categoryPool.add (c.id);

    shuffleArray (deckPool, rng);
    shuffleArray (categoryPool, rng);
    pool.addArray (deckPool);
    pool.addArray (categoryPool);

    juce::StringArray rest;
    for (auto& c : lib.all())
        if (c.id != current.target && ! pool.contains (c.id)) rest.add (c.id);
    shuffleArray (rest, rng);
    pool.addArray (rest);

    juce::StringArray options { current.target };
    juce::StringArray usedSymbols { target != nullptr ? target->symbol : current.target };
    for (auto& id : pool)
    {
        if (options.size() >= 4) break;
        auto* c = lib.find (id);
        if (c == nullptr || usedSymbols.contains (c->symbol)) continue;
        options.add (id);
        usedSymbols.add (c->symbol);
    }

    shuffleArray (options, rng);
    current.options = options;
    current.correctIndex = options.indexOf (current.target);
}

//==============================================================================
MatchGame::MatchGame (juce::StringArray chordIds, int pairs, juce::int64 seed)
{
    juce::Random rng (seed);
    shuffleArray (chordIds, rng);
    pairs = juce::jmin (pairs, chordIds.size());

    for (int i = 0; i < pairs; ++i)
    {
        tileList.push_back ({ chordIds[i], false, false });
        tileList.push_back ({ chordIds[i], true, false });
    }

    for (int i = (int) tileList.size() - 1; i > 0; --i)
        std::swap (tileList[(size_t) i], tileList[(size_t) rng.nextInt (i + 1)]);
}

bool MatchGame::tryMatch (int a, int b)
{
    if (a == b || a < 0 || b < 0 || a >= (int) tileList.size() || b >= (int) tileList.size())
        return false;

    auto& ta = tileList[(size_t) a];
    auto& tb = tileList[(size_t) b];
    if (ta.matched || tb.matched) return false;

    if (ta.chordId == tb.chordId && ta.isDiagram != tb.isDiagram)
    {
        ta.matched = tb.matched = true;
        return true;
    }
    ++misses;
    return false;
}

bool MatchGame::complete() const
{
    for (auto& t : tileList)
        if (! t.matched) return false;
    return ! tileList.empty();
}

} // namespace gg
