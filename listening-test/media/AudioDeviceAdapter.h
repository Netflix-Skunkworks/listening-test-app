// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include "AudioEngine.h"

constexpr int MAXNUMBEROFDEVICECHANNELS = 64;

// Owns the device and its callback registration, borrowing an engine that must
// outlive it. Device/control operations run on the message thread. detach()
// waits for the callback before media or working buffers may be replaced.
// JUCE calls prepare at device startup while rendering is excluded, and calls
// render on its audio thread. No file or video operations occur in callbacks.
class AudioDeviceAdapter : private juce::AudioIODeviceCallback {
public:
    explicit AudioDeviceAdapter(AudioEngine& source) : engine(source) {}
    ~AudioDeviceAdapter() override;
    void reset(const juce::File& settings);
    void attach();
    void detach();
    juce::AudioDeviceManager& manager() { return deviceManager; }
    int sampleRate() const;
    juce::BigInteger outputChannels() const;
private:
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallback(const float**, int, float**, int, int) override;
    AudioEngine& engine;
    juce::AudioDeviceManager deviceManager;
    bool attached = false;
};
