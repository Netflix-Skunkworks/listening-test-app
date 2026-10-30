// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#include "AudioDeviceAdapter.h"
using namespace juce;

AudioDeviceAdapter::~AudioDeviceAdapter() { detach(); }

void AudioDeviceAdapter::reset(const File& settings) {
    detach();
    auto state = parseXML(settings);
    deviceManager.initialise(MAXNUMBEROFDEVICECHANNELS, MAXNUMBEROFDEVICECHANNELS, state.get(), false);
    attach();
}

void AudioDeviceAdapter::attach() {
    if (attached) return;
    deviceManager.addAudioCallback(this);
    attached = true;
}

void AudioDeviceAdapter::detach() {
    if (attached) deviceManager.removeAudioCallback(this);
    attached = false;
    engine.deviceStopped();
}

int AudioDeviceAdapter::sampleRate() const {
    const int cached = engine.getSampleRate();
    if (cached > 0) return cached;
    AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    return (int) setup.sampleRate;
}

BigInteger AudioDeviceAdapter::outputChannels() const {
    AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup(setup);
    return setup.outputChannels;
}

void AudioDeviceAdapter::audioDeviceAboutToStart(AudioIODevice* device) {
    engine.prepare(device != nullptr ? device->getCurrentSampleRate() : 0.0,
                   device != nullptr ? device->getCurrentBufferSizeSamples() : 0);
}

void AudioDeviceAdapter::audioDeviceStopped() { engine.deviceStopped(); }

void AudioDeviceAdapter::audioDeviceIOCallback(const float**, int, float** outputs, int channels, int samples) {
    engine.render(outputs, channels, samples);
}
