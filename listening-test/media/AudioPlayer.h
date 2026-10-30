// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#pragma once

#include "AudioEngine.h"
#include "AudioDeviceAdapter.h"
#include "VideoPlaybackCoordinator.h"

struct LoadedTrialMedia;

// Application facade. Construct/destroy and device/video operations on the
// message thread. Members are ordered so video and device callbacks are stopped
// before the engine is destroyed. Transport setters delegate to engine atomics.
class AudioPlayer {
public:
    AudioPlayer(File audioSettingsFile, bool initialiseDevice = true);
    void start();
    void stop();
    int getSampleRate() const { return device.sampleRate(); }
    int getOutputChannelCount() const { return device.outputChannels().countNumberOfSetBits(); }
    void switchStimulus(int value) { engine.switchStimulus(value); }
    double getCurrentPosition() const { return engine.getCurrentPosition(); }
    double getCurrentTime() const { return engine.getCurrentTime(); }
    bool isRunning() const { return engine.isRunning(); }
    bool isPaused() const { return engine.isPaused(); }
    void setPlayLoop(bool value) { engine.setPlayLoop(value); }
    bool setPosition(float value);
    void setFragmentStartPosition(float value) { engine.setFragmentStartPosition(value); }
    void setFragmentEndPosition(float value) { engine.setFragmentEndPosition(value); }
    void pause() { engine.pause(); }
    void resume() { engine.resume(); }
    // Callback must be detached with stop() before replacing media.
    void replaceTrialMedia(LoadedTrialMedia&);
    VideoComponent* getVideoComponent() { return video.getComponent(); }
    std::unique_ptr<XmlElement> createStateXml() { return device.manager().createStateXml(); }
    AudioDeviceManager& getAudioDeviceManager() { return device.manager(); }
    // Supports device-free application fixtures; prepare only while detached.
    AudioEngine& getEngine() { return engine; }
private:
    AudioEngine engine;
    AudioDeviceAdapter device;
    VideoPlaybackCoordinator video;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer)
};
