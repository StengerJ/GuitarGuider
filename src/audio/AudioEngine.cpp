#include "AudioEngine.h"
#include "core/StrumSynth.h"
#include "dsp/Presets.h"

namespace gg
{

AudioEngine::AudioEngine()
{
    ring.assign (1 << 15, 0.0f);
    recBuffer.assign ((size_t) recFifo.getTotalSize(), 0.0f);
    take.reserve (48000 * 60);
}

AudioEngine::~AudioEngine()
{
    stopTimer();
    deviceManager.removeAudioCallback (this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::initialise (juce::PropertiesFile& settings)
{
    demoInput = settings.getBoolValue ("demoInput", false);
    inputChannel = settings.getIntValue ("inputChannel", 0);
    monitorOn = settings.getBoolValue ("monitorOn", true);
    masterVolume = (float) settings.getDoubleValue ("masterVolume", 0.8);
    metronomeOn = settings.getBoolValue ("metronome", false);
    countIn = settings.getBoolValue ("countIn", true);
    session.bpm = settings.getDoubleValue ("bpm", 100.0);

    auto rig = juce::JSON::parse (settings.getValue ("rig"));
    if (rig.isObject())
        chain.fromVar (rig);
    else
    {
        // First launch: a bluesy rig with a couple of pedals so the board isn't empty
        const auto& first = PresetManager::factoryPresets()[1];
        chain.fromVar (first.state);
        settings.setValue ("lastPreset", first.name);
    }

    std::unique_ptr<juce::XmlElement> saved (settings.getXmlValue ("audioDeviceState"));
    auto err = deviceManager.initialise (2, 2, saved.get(), true);
    if (err.isNotEmpty())
        deviceManager.initialise (0, 2, nullptr, true);   // no input available: output only, demo input still works

    if (session.numTracks() == 0)
        session.addTrack ("Guitar 1");

    deviceManager.addAudioCallback (this);
    startTimerHz (30);
}

void AudioEngine::saveState (juce::PropertiesFile& settings)
{
    if (auto xml = deviceManager.createStateXml())
        settings.setValue ("audioDeviceState", xml.get());
    settings.setValue ("rig", juce::JSON::toString (chain.toVar(), true));
    settings.setValue ("demoInput", demoInput.load());
    settings.setValue ("inputChannel", inputChannel.load());
    settings.setValue ("monitorOn", monitorOn.load());
    settings.setValue ("masterVolume", masterVolume.load());
    settings.setValue ("metronome", metronomeOn.load());
    settings.setValue ("countIn", countIn.load());
    settings.setValue ("bpm", session.bpm.load());
    settings.saveIfNeeded();
}

bool AudioEngine::hasInputDevice() const
{
    auto* d = deviceManager.getCurrentAudioDevice();
    return d != nullptr && d->getActiveInputChannels().countNumberOfSetBits() > 0;
}

juce::String AudioEngine::deviceSummary() const
{
    auto* d = deviceManager.getCurrentAudioDevice();
    if (d == nullptr) return "No audio device";
    return d->getName();
}

double AudioEngine::latencyMs() const
{
    auto* d = deviceManager.getCurrentAudioDevice();
    if (d == nullptr || d->getCurrentSampleRate() <= 0) return 0.0;
    const int samples = d->getInputLatencyInSamples() + d->getOutputLatencyInSamples() + d->getCurrentBufferSizeSamples();
    return 1000.0 * samples / d->getCurrentSampleRate();
}

void AudioEngine::copyRecentInput (float* dest, int n) const
{
    const int size = (int) ring.size();
    n = juce::jmin (n, size);
    int start = ringWrite.load (std::memory_order_acquire) - n;
    if (start < 0) start += size;
    for (int i = 0; i < n; ++i)
        dest[i] = ring[(size_t) ((start + i) % size)];
}

//==============================================================================
void AudioEngine::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    const double sr = device->getCurrentSampleRate();
    const int block = juce::jmax (device->getCurrentBufferSizeSamples(), 64);
    currentRate = sr;

    chain.prepare (sr, juce::jmax (block, 512));
    mono.assign ((size_t) juce::jmax (block, 2048), 0.0f);
    left.assign (mono.size(), 0.0f);
    right.assign (mono.size(), 0.0f);
    demoRiff = StrumSynth::renderDemoRiff (sr);
    demoPos = 0;
    session.setSampleRate (sr);
}

void AudioEngine::audioDeviceStopped() {}

void AudioEngine::renderMetronome (float* l, float* r, int n, juce::int64 pos) noexcept
{
    const double sr = currentRate.load();
    const double beatLen = sr * 60.0 / juce::jmax (30.0, session.bpm.load());
    const int bpb = juce::jmax (1, session.beatsPerBar.load());
    const bool inCountIn = recording.load() && pos < recStart;
    const bool active = metronomeOn.load() || inCountIn;
    const float decay = std::exp (-1.0f / (float) (0.025 * sr));

    for (int i = 0; i < n; ++i)
    {
        const juce::int64 p = pos + i;
        const auto beat = (juce::int64) std::floor ((double) p / beatLen);
        if (beat != lastBeat)
        {
            lastBeat = beat;
            if (active || (recording.load() && p < recStart))
            {
                clickEnv = 1.0f;
                clickPhase = 0.0f;
                clickFreq = (((beat % bpb) + bpb) % bpb == 0) ? 1760.0f : 1175.0f;
            }
        }
        if (clickEnv > 0.0005f)
        {
            const float s = std::sin (clickPhase) * clickEnv * 0.28f;
            clickPhase += juce::MathConstants<float>::twoPi * clickFreq / (float) sr;
            clickEnv *= decay;
            l[i] += s; r[i] += s;
        }
    }
}

void AudioEngine::audioDeviceIOCallbackWithContext (const float* const* in, int numIn, float* const* out, int numOut,
                                                    int numSamples, const juce::AudioIODeviceCallbackContext&)
{
    const int maxChunk = (int) mono.size();
    int done = 0;

    while (done < numSamples)
    {
        const int n = juce::jmin (maxChunk, numSamples - done);
        float* m = mono.data();
        float* l = left.data();
        float* r = right.data();

        // 1. Input (interface or demo riff)
        const int ch = inputChannel.load();
        if (demoInput.load() && demoRiff.getNumSamples() > 0)
        {
            const float* src = demoRiff.getReadPointer (0);
            for (int i = 0; i < n; ++i)
            {
                m[i] = src[demoPos];
                if (++demoPos >= demoRiff.getNumSamples()) demoPos = 0;
            }
        }
        else if (numIn > 0)
        {
            const float* src = in[juce::jlimit (0, numIn - 1, ch)];
            if (src != nullptr) std::copy (src + done, src + done + n, m);
            else std::fill (m, m + n, 0.0f);
        }
        else
            std::fill (m, m + n, 0.0f);

        // 2. Metering + analysis tap (dry)
        float ip = 0.0f;
        const int size = (int) ring.size();
        int w = ringWrite.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
        {
            ip = juce::jmax (ip, std::abs (m[i]));
            ring[(size_t) w] = m[i];
            if (++w >= size) w = 0;
        }
        ringWrite.store (w, std::memory_order_release);
        if (ip > inputPeak.load (std::memory_order_relaxed)) inputPeak.store (ip, std::memory_order_relaxed);

        // 3. Amp chain
        chain.process (m, n);

        const float monitorGain = monitorOn.load() ? 1.0f : 0.0f;
        for (int i = 0; i < n; ++i) { l[i] = m[i] * monitorGain; r[i] = l[i]; }

        // 4. Transport: playback, recording, metronome
        if (playing.load())
        {
            juce::int64 pos = playhead.load();

            const auto sessionLen = session.lengthSamples();
            if (loopPlayback.load() && ! recording.load() && sessionLen > 0 && pos >= sessionLen)
                pos = 0;

            // Tracks only exist at positions >= 0 (negative = count-in)
            const int offset = (int) juce::jlimit ((juce::int64) 0, (juce::int64) n, -pos);
            if (offset < n)
                session.mixInto (l + offset, r + offset, n - offset, pos + offset);

            if (recording.load())
            {
                const int recOffset = (int) juce::jlimit ((juce::int64) 0, (juce::int64) n, recStart - pos);
                const int count = n - recOffset;
                if (count > 0)
                {
                    int s1, n1, s2, n2;
                    recFifo.prepareToWrite (count, s1, n1, s2, n2);
                    if (n1 + n2 < count) recOverflow = true;
                    if (n1 > 0) std::copy (m + recOffset, m + recOffset + n1, recBuffer.data() + s1);
                    if (n2 > 0) std::copy (m + recOffset + n1, m + recOffset + n1 + n2, recBuffer.data() + s2);
                    recFifo.finishedWrite (n1 + n2);
                }
            }

            renderMetronome (l, r, n, pos);
            playhead.store (pos + n);
        }

        // 5. Chord preview
        {
            const juce::SpinLock::ScopedTryLockType sl (previewLock);
            if (sl.isLocked() && previewPos < preview.getNumSamples())
            {
                const int count = juce::jmin (n, preview.getNumSamples() - previewPos);
                const float* src = preview.getReadPointer (0) + previewPos;
                for (int i = 0; i < count; ++i) { l[i] += src[i]; r[i] += src[i]; }
                previewPos += count;
            }
        }

        // 6. Output
        const float mv = masterVolume.load();
        float op = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            l[i] = juce::jlimit (-1.0f, 1.0f, l[i] * mv);
            r[i] = juce::jlimit (-1.0f, 1.0f, r[i] * mv);
            op = juce::jmax (op, std::abs (l[i]), std::abs (r[i]));
        }
        if (op > outputPeak.load (std::memory_order_relaxed)) outputPeak.store (op, std::memory_order_relaxed);

        for (int c = 0; c < numOut; ++c)
            if (out[c] != nullptr)
                std::copy ((c % 2 == 0 ? l : r), (c % 2 == 0 ? l : r) + n, out[c] + done);

        done += n;
    }
}

//==============================================================================
void AudioEngine::timerCallback()
{
    // Drain recorded audio into the live take (message thread)
    const int ready = recFifo.getNumReady();
    if (ready > 0)
    {
        int s1, n1, s2, n2;
        recFifo.prepareToRead (ready, s1, n1, s2, n2);
        take.insert (take.end(), recBuffer.begin() + s1, recBuffer.begin() + s1 + n1);
        take.insert (take.end(), recBuffer.begin() + s2, recBuffer.begin() + s2 + n2);
        recFifo.finishedRead (n1 + n2);
    }
}

void AudioEngine::play()
{
    if (playing) return;
    const double beatLen = currentRate.load() * 60.0 / juce::jmax (30.0, session.bpm.load());
    lastBeat = (juce::int64) std::floor ((double) (playhead.load() - 1) / beatLen);
    playing = true;
    if (onTransportChanged) onTransportChanged();
}

void AudioEngine::stop()
{
    const bool wasRec = recording.load();
    recording = false;
    playing = false;
    if (wasRec) finishRecording();
    if (onTransportChanged) onTransportChanged();
}

void AudioEngine::record (int trackIndex)
{
    if (recording || session.track (trackIndex) == nullptr) return;
    if (playing) stop();

    recTrack = trackIndex;
    recStart = juce::jmax ((juce::int64) 0, playhead.load());
    take.clear();
    recFifo.reset();
    recOverflow = false;

    juce::int64 startPos = recStart;
    if (countIn.load())
    {
        const double barLen = currentRate.load() * 60.0 / juce::jmax (30.0, session.bpm.load()) * session.beatsPerBar.load();
        startPos = recStart - (juce::int64) barLen;
    }
    playhead = startPos;
    const double beatLen = currentRate.load() * 60.0 / juce::jmax (30.0, session.bpm.load());
    lastBeat = (juce::int64) std::floor ((double) (startPos - 1) / beatLen);
    recording = true;
    playing = true;
    if (onTransportChanged) onTransportChanged();
}

void AudioEngine::finishRecording()
{
    timerCallback(); // drain whatever is left

    if (! take.empty() && recTrack >= 0)
    {
        // Compensate for the round-trip latency so the take lines up with what you heard
        int latency = 0;
        if (auto* d = deviceManager.getCurrentAudioDevice())
            latency = d->getInputLatencyInSamples() + d->getOutputLatencyInSamples();
        if (demoInput.load()) latency = 0;
        latency = juce::jmin (latency, (int) take.size());

        session.writeAudio (recTrack, recStart, take.data() + latency, (int) take.size() - latency);
    }
    take.clear();
}

void AudioEngine::setPosition (juce::int64 samples)
{
    if (recording) return;
    playhead = juce::jmax ((juce::int64) 0, samples);
    const double beatLen = currentRate.load() * 60.0 / juce::jmax (30.0, session.bpm.load());
    lastBeat = (juce::int64) std::floor ((double) (playhead.load() - 1) / beatLen);
    if (onTransportChanged) onTransportChanged();
}

void AudioEngine::playPreview (juce::AudioBuffer<float> buffer)
{
    juce::AudioBuffer<float> old;
    {
        const juce::SpinLock::ScopedLockType sl (previewLock);
        std::swap (old, preview);
        preview = std::move (buffer);
        previewPos = 0;
    }
}

} // namespace gg
