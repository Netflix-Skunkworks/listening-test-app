//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "../JuceLibraryCode/JuceHeader.h"

#define    MAXNUMBEROFDEVICECHANNELS    64


class AudioPlayer : public AudioIODeviceCallback {
public:

    AudioPlayer(File audioSettingsFile);

    ~AudioPlayer();

    void resetCurrentDevice(File audioSettingsFile);

    void start();

    void stop();

    int getSampleRate();

    BigInteger getOutputChannels() {
        AudioDeviceManager::AudioDeviceSetup deviceSettings;
        audioDeviceManager.getAudioDeviceSetup(deviceSettings);
        return deviceSettings.outputChannels;
    }

    void audioDeviceIOCallbackWithContext(const float *const *inputChannelData,
                                          int umInputChannels,
                                          float *const *outputChannelData,
                                          int totalNumOutputChannels,
                                          int numOutSamples,
                                          const AudioIODeviceCallbackContext &context) override;


    void audioDeviceAboutToStart(AudioIODevice *device) override;

    void audioDeviceStopped() override;

    void switchStimulus(int newStimulusNumber) { nextStimulus = newStimulusNumber; }

    int getCurrentStimulus() { return currentStimulus; }

    int getNextStimulus() { return nextStimulus; }

    int getChannelCount() { return channelCount; }

    double getCurrentPosition() { return (double) getCurrentSample() / totalSamples; }

    double getCurrentTime() { return (double) getCurrentSample() / getSampleRate(); }

    bool isRunning() { return playerRunning; }

    bool isPaused() { return playerPaused; }

    int getCurrentSample() { return currentSample; }

    void setPlayLoop(bool shouldPlayLoop) { playInLoop = shouldPlayLoop; }

    void setSample(int newSample) { currentSample = newSample; }

    void setTotalSamples(int newTotalSamples) { totalSamples = newTotalSamples; }

    void setFragmentStartSample(int newStartSample) { startSample = newStartSample; }

    void setFragmentEndSample(int newEndSample) { endSample = newEndSample; }

    bool setPosition(float newPositionPercent);

    void setFragmentStartPosition(float newPositionPercent) {
        setFragmentStartSample(int(floor(newPositionPercent * totalSamples)));
    }

    void setFragmentEndPosition(float newPositionPercent) {
        setFragmentEndSample(int(floor(newPositionPercent * totalSamples)));
    }

    void setChannelCount(int chanCount) { channelCount = chanCount; }

    void pause() { playerPaused = true; }

    void resume() { playerPaused = false; }

    void releaseAllAudioData();

    void addAudioFromFile(AudioFormatReader *wavReader, int chanCount, int numSamps);

    void setVideoFile(File &file) { videoFile = &file; }

    VideoComponent *getVideoComponent() { return &videoComponent; }

    std::unique_ptr <XmlElement> createStateXml() { return audioDeviceManager.createStateXml(); }

    AudioDeviceManager &getAudioDeviceManager() {
        return audioDeviceManager;
    }


private:
    int currentSample;
    int totalSamples;
    int startSample;
    int endSample;
    bool playInLoop;
    int currentStimulus;
    int nextStimulus;
    volatile bool playerRunning;
    volatile bool playerPaused;
    int channelCount;

    AudioDeviceManager audioDeviceManager;
    OwnedArray <AudioBuffer<float>> audioStimData;

    VideoComponent videoComponent;
    File *videoFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer);
};


#endif
