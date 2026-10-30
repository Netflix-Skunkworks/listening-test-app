// Listening Test Application — audio rendering extracted from AudioPlayer.
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#include "AudioEngine.h"
using namespace juce;

void AudioEngine::prepare(double rate, int blockSize) {
    currentStimulus = -1;
    nextStimulus = -1;
    currentSampleRate = rate;
    currentBlockSize = blockSize;
    allocateMixBuffers();
    playerPaused = true;
    playerRunning = true;
}

void AudioEngine::setChannelCount(int channels) {
    channelCount = jmax(0, channels);
    allocateMixBuffers();
}

void AudioEngine::allocateMixBuffers() {
    const int channels = jmax(1, channelCount.load());
    const int samples = jmax(1024, currentBlockSize);
    mixBuffer.setSize(channels, samples, false, true, false);
    fadeBuffer.setSize(channels, samples, false, true, false);
}

void AudioEngine::replaceMedia(OwnedArray<AudioBuffer<float>>& media, int samples, int channels) {
    jassert(!isRunning());
    audioStimData.swapWith(media);
    totalSamples = jmax(0, samples);
    setChannelCount(channels);
    currentSample = 0;
    startSample = 0;
    endSample = totalSamples.load();
    currentStimulus = -1;
    nextStimulus = -1;
}

void AudioEngine::copyStimulusInto(AudioBuffer<float>& destination, int index, int channels,
                                  int position, int samples, int loopStart, int loopEnd, bool loop) {
    if (!isPositiveAndBelow(index, audioStimData.size())) return;
    const auto& source = *audioStimData[index];
    int offset = 0;
    while (offset < samples) {
        if (position >= loopEnd) {
            if (!loop || loopStart >= loopEnd) break;
            position = loopStart;
        }
        const int count = jmin(samples - offset, loopEnd - position);
        if (count <= 0) break;
        const int readable = jlimit(0, count, source.getNumSamples() - position);
        for (int ch = 0; ch < jmin(channels, source.getNumChannels()); ++ch)
            if (readable > 0) destination.copyFrom(ch, offset, source, ch, position, readable);
        offset += count;
        position += count;
    }
}

// Oversized callbacks are processed in bounded chunks, without reallocating.
void AudioEngine::render(float* const* outputs, int channels, int samples) {
    int offset = 0;
    while (offset < samples) {
        const int count = jmin(samples - offset, mixBuffer.getNumSamples());
        const int active = jmin(channelCount.load(), channels, mixBuffer.getNumChannels());
        const bool rendered = playerRunning.load() && !playerPaused.load() && active > 0 && count > 0;
        if (rendered) renderNextBlock(active, count);
        for (int ch = 0; ch < channels; ++ch) {
            if (outputs[ch] == nullptr) continue;
            if (rendered && ch < active)
                FloatVectorOperations::copy(outputs[ch] + offset, mixBuffer.getReadPointer(ch), count);
            else
                FloatVectorOperations::clear(outputs[ch] + offset, count > 0 ? count : samples - offset);
        }
        if (count <= 0) break;
        offset += count;
    }
}

void AudioEngine::renderNextBlock(int numChannels, int numSamples) {
    // Read each independent atomic control once per block. This is not a
    // transaction across controls; loop bounds are clamped together below.
    const int bufferLength = jmax(0, totalSamples.load());
    const bool loop = playInLoop.load();
    const int wanted = nextStimulus.load();
    int playing = currentStimulus.load();
    int position = jlimit(0, bufferLength, currentSample.load());

    const int loopStart = jlimit(0, bufferLength, startSample.load());
    const int loopEnd = jlimit(loopStart, bufferLength, endSample.load());
    if (loop && position >= loopEnd) position = loopStart;

    mixBuffer.clear(0, numSamples);

    if (playing >= 0) {
        copyStimulusInto(mixBuffer, playing, numChannels, position, numSamples, loopStart, loopEnd, loop);

        if (playing != wanted && wanted >= 0) {
            const bool canCrossFade = crossfadeEnabled
                                      && fadeBuffer.getNumChannels() >= numChannels
                                      && fadeBuffer.getNumSamples() >= numSamples;

            if (canCrossFade) {
                fadeBuffer.clear(0, numSamples);
                copyStimulusInto(fadeBuffer, wanted, numChannels, position, numSamples, loopStart, loopEnd, loop);

                for (int ch = 0; ch < numChannels; ch++) {
                    mixBuffer.applyGainRamp(ch, 0, numSamples, 1.0f, 0.0f); // fade-out
                    mixBuffer.addFromWithRamp(ch, 0, fadeBuffer.getReadPointer(ch), numSamples, 0.0f,
                                              1.0f); // add with fade-in
                }
            }

            /* With the cross-fade turned off the switch takes effect from the
               next block and this one keeps the outgoing stimulus, exactly as
               before.  Some BS.1116 tests depend on that. */
            playing = wanted;
        }
    } else if (wanted >= 0) {
        /* start playing the first selected stimulus */
        copyStimulusInto(mixBuffer, wanted, numChannels, position, numSamples, loopStart, loopEnd, loop);
        playing = wanted;
    }

    /* advance audio buffer */
    position += numSamples;
    if (position >= loopEnd) {
        if (loop) {
            const int loopLength = jmax(1, loopEnd - loopStart);
            position = loopStart + ((position - loopEnd) % loopLength);
        } else {
            pause();
            position = loopStart;
        }
    }

    currentSample = position;
    currentStimulus = playing;
}

