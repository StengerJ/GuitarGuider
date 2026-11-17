#include "Session.h"

namespace gg
{

namespace
{
    const juce::uint32 trackColours[] { 0xffd9a441, 0xff52b0c4, 0xffd06c95, 0xff3c9a64,
                                        0xff7a5fb5, 0xffe06a2c, 0xff98a1a8, 0xffb23a48 };

    void panGains (float pan, float& l, float& r) noexcept
    {
        const float a = (juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        l = std::cos (a) * juce::MathConstants<float>::sqrt2;
        r = std::sin (a) * juce::MathConstants<float>::sqrt2;
    }
}

Session::Session() = default;

int Session::addTrack (const juce::String& name)
{
    auto t = std::make_unique<Track>();
    ++nameCounter;
    t->name = name.isNotEmpty() ? name : "Track " + juce::String (nameCounter);

    const juce::SpinLock::ScopedLockType sl (lock);
    if ((int) tracks.size() >= maxTracks) return -1;
    t->colour = trackColours[tracks.size() % 8];
    tracks.push_back (std::move (t));
    ++version;
    return (int) tracks.size() - 1;
}

void Session::removeTrack (int index)
{
    std::unique_ptr<Track> doomed;
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        if (index < 0 || index >= (int) tracks.size()) return;
        doomed = std::move (tracks[(size_t) index]);
        tracks.erase (tracks.begin() + index);
    }
    ++version;
}

int Session::numTracks() const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    return (int) tracks.size();
}

Track* Session::track (int index) const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    return juce::isPositiveAndBelow (index, (int) tracks.size()) ? tracks[(size_t) index].get() : nullptr;
}

void Session::writeAudio (int trackIndex, juce::int64 startSample, const float* data, int numSamples)
{
    if (numSamples <= 0 || startSample < 0) return;

    const juce::SpinLock::ScopedLockType sl (lock);
    if (! juce::isPositiveAndBelow (trackIndex, (int) tracks.size())) return;
    auto& a = tracks[(size_t) trackIndex]->audio;
    const auto end = (size_t) (startSample + numSamples);
    if (a.size() < end) a.resize (end, 0.0f);
    std::copy (data, data + numSamples, a.begin() + (std::ptrdiff_t) startSample);
    ++version;
}

void Session::clearTrack (int trackIndex)
{
    std::vector<float> doomed;
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        if (! juce::isPositiveAndBelow (trackIndex, (int) tracks.size())) return;
        doomed.swap (tracks[(size_t) trackIndex]->audio);
    }
    ++version;
}

juce::int64 Session::lengthSamples() const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    size_t len = 0;
    for (auto& t : tracks) len = juce::jmax (len, t->audio.size());
    return (juce::int64) len;
}

void Session::resample (std::vector<float>& audio, double ratio)
{
    if (audio.empty() || std::abs (ratio - 1.0) < 1.0e-9) return;
    const auto outLen = (size_t) std::ceil ((double) audio.size() / ratio);
    std::vector<float> out (outLen + 8, 0.0f);
    std::vector<float> padded (audio);
    padded.resize (audio.size() + 16, 0.0f);
    juce::LagrangeInterpolator interp;
    interp.process (ratio, padded.data(), out.data(), (int) outLen);
    out.resize (outLen);
    audio.swap (out);
}

void Session::setSampleRate (double newRate)
{
    if (newRate <= 0 || std::abs (newRate - rate) < 0.5) return;
    const juce::SpinLock::ScopedLockType sl (lock);
    const double ratio = rate / newRate;   // input samples per output sample
    for (auto& t : tracks)
        resample (t->audio, ratio);
    rate = newRate;
    ++version;
}

void Session::mixInto (float* left, float* right, int n, juce::int64 position) const noexcept
{
    const juce::SpinLock::ScopedTryLockType sl (lock);
    if (! sl.isLocked()) return;

    bool anySolo = false;
    for (auto& t : tracks) anySolo = anySolo || t->solo.load (std::memory_order_relaxed);

    for (auto& t : tracks)
    {
        if (t->mute.load (std::memory_order_relaxed)) continue;
        if (anySolo && ! t->solo.load (std::memory_order_relaxed)) continue;

        const auto size = (juce::int64) t->audio.size();
        if (position >= size) continue;
        const int count = (int) juce::jmin ((juce::int64) n, size - position);

        float gl, gr;
        panGains (t->pan.load (std::memory_order_relaxed), gl, gr);
        const float vol = t->volume.load (std::memory_order_relaxed);
        const float* src = t->audio.data() + position;

        juce::FloatVectorOperations::addWithMultiply (left, src, gl * vol, count);
        juce::FloatVectorOperations::addWithMultiply (right, src, gr * vol, count);
    }
}

juce::AudioBuffer<float> Session::renderMix() const
{
    const auto len = (int) lengthSamples();
    juce::AudioBuffer<float> out (2, juce::jmax (1, len));
    out.clear();
    if (len == 0) return out;

    const juce::SpinLock::ScopedLockType sl (lock);
    bool anySolo = false;
    for (auto& t : tracks) anySolo = anySolo || t->solo.load();

    for (auto& t : tracks)
    {
        if (t->mute.load() || (anySolo && ! t->solo.load()) || t->audio.empty()) continue;
        float gl, gr;
        panGains (t->pan.load(), gl, gr);
        const float vol = t->volume.load();
        out.addFrom (0, 0, t->audio.data(), (int) t->audio.size(), gl * vol);
        out.addFrom (1, 0, t->audio.data(), (int) t->audio.size(), gr * vol);
    }

    // Soft safety limiter so exports never clip hard
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = out.getWritePointer (ch);
        for (int i = 0; i < len; ++i)
            if (std::abs (d[i]) > 0.9f)
                d[i] = (d[i] > 0 ? 1.0f : -1.0f) * (0.9f + 0.1f * std::tanh ((std::abs (d[i]) - 0.9f) * 10.0f));
    }
    return out;
}

bool Session::exportWav (const juce::File& file) const
{
    auto mix = renderMix();
    file.deleteFile();
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
    if (stream == nullptr) return false;

    juce::WavAudioFormat wav;
    auto writer = wav.createWriterFor (stream, juce::AudioFormatWriterOptions{}
                                                   .withSampleRate (rate)
                                                   .withNumChannels (2)
                                                   .withBitsPerSample (24));
    if (writer == nullptr) return false;

    return writer->writeFromAudioSampleBuffer (mix, 0, mix.getNumSamples());
}

bool Session::importAudioFile (int trackIndex, const juce::File& file, juce::AudioFormatManager& formats)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr) return false;

    const auto maxSeconds = 15 * 60;
    const auto len = (int) juce::jmin (reader->lengthInSamples, (juce::int64) (reader->sampleRate * maxSeconds));
    juce::AudioBuffer<float> buf ((int) reader->numChannels, len);
    reader->read (&buf, 0, len, 0, true, true);

    std::vector<float> mono ((size_t) len, 0.0f);
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        juce::FloatVectorOperations::addWithMultiply (mono.data(), buf.getReadPointer (ch),
                                                      1.0f / (float) buf.getNumChannels(), len);

    resample (mono, reader->sampleRate / rate);

    {
        const juce::SpinLock::ScopedLockType sl (lock);
        if (! juce::isPositiveAndBelow (trackIndex, (int) tracks.size())) return false;
        tracks[(size_t) trackIndex]->audio.swap (mono);
        tracks[(size_t) trackIndex]->name = file.getFileNameWithoutExtension();
    }
    ++version;
    return true;
}

} // namespace gg
