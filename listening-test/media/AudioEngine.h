// Copyright(C) 2026 Netflix, Inc.
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

struct PlaybackSnapshot {
    int sample;
    double sampleRate;
    bool running;
    bool paused;
    double seconds() const { return sampleRate > 0.0 ? sample / sampleRate : 0.0; }
};

// Owns decoded media and preallocated rendering buffers. Control setters may
// run concurrently with render(). prepare() requires rendering to be excluded
// (device startup or detached); replaceMedia() requires a detached callback.
// No device, GUI, timer, or video ownership.
class AudioEngine {
public:
    void prepare(double sampleRate, int maximumBlockSize);
    void deviceStopped() { playerRunning = false; }
    int getSampleRate() const { return (int) currentSampleRate.load(); }
    void replaceMedia(juce::OwnedArray<juce::AudioBuffer<float>>&, int samples, int channels);
    void render(float* const* outputs, int channels, int samples);
    PlaybackSnapshot playbackPosition() const {
        return { currentSample.load(), currentSampleRate.load(), playerRunning.load(), playerPaused.load() };
    }
    void switchStimulus(int newStimulusNumber) { nextStimulus = newStimulusNumber; }

    int getCurrentStimulus() const { return currentStimulus.load(); }

    int getNextStimulus() const { return nextStimulus.load(); }

    int getChannelCount() const { return channelCount.load(); }

    double getCurrentPosition() const {
        const int total = totalSamples.load();
        return total > 0 ? (double) currentSample.load() / total : 0.0;
    }

    double getCurrentTime() const {
        const double rate = currentSampleRate.load();
        return rate > 0.0 ? (double) currentSample.load() / rate : 0.0;
    }

    bool isRunning() const { return playerRunning.load(); }

    bool isPaused() const { return playerPaused.load(); }

    int getCurrentSample() const { return currentSample.load(); }

    void setPlayLoop(bool shouldPlayLoop) { playInLoop = shouldPlayLoop; }

    void setSample(int newSample) { currentSample = newSample; }

    void setTotalSamples(int newTotalSamples) { totalSamples = newTotalSamples; }

    void setFragmentStartSample(int newStartSample) { startSample = newStartSample; }

    void setFragmentEndSample(int newEndSample) { endSample = newEndSample; }

    bool setPosition(float fraction) {
        const int total = totalSamples.load();
        if (total <= 0 || !std::isfinite(fraction) || fraction < 0.0f || fraction >= 1.0f)
            return false;
        setSample((int) std::floor(fraction * total));
        return true;
    }

    void setFragmentStartPosition(float newPositionPercent) {
        setFragmentStartSample((int) std::floor(newPositionPercent * totalSamples.load()));
    }

    void setFragmentEndPosition(float newPositionPercent) {
        setFragmentEndSample((int) std::floor(newPositionPercent * totalSamples.load()));
    }

    void pause() { playerPaused = true; }

    void resume() { playerPaused = false; }

private:
    // Some BS.1116 protocols require switching without crossfades.
    static constexpr bool crossfadeEnabled = true;
    void setChannelCount(int chanCount);
    void renderNextBlock(int numChannels, int numSamples);
    void copyStimulusInto(juce::AudioBuffer<float>& destination, int stimulusIndex,
                          int numChannels, int position, int samples, int loopStart, int loopEnd, bool loop);
    void allocateMixBuffers();
    std::atomic<int> currentSample{0}, totalSamples{1}, startSample{0}, endSample{0};
    std::atomic<bool> playInLoop{true};
    std::atomic<int> currentStimulus{-1}, nextStimulus{-1};
    std::atomic<bool> playerRunning{false}, playerPaused{true};
    std::atomic<int> channelCount{0};
    std::atomic<double> currentSampleRate{0.0};
    int currentBlockSize = 0;
    juce::OwnedArray<juce::AudioBuffer<float>> audioStimData;
    juce::AudioBuffer<float> mixBuffer, fadeBuffer;
};
