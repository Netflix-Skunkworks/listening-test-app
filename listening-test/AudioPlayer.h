//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "../JuceLibraryCode/JuceHeader.h"
#include <atomic>
#include <cmath>

#define    MAXNUMBEROFDEVICECHANNELS    64


/*  Threading
    ---------
    Playback state is written by the message thread (and by the stimulus loader
    thread between trials) and read by the audio thread, so everything shared
    is atomic and the audio callback takes a single snapshot of it per block.

    The video component is driven entirely from the message thread, by this
    class's own timer.  JUCE's VideoComponent wraps platform playback objects
    that must not be touched from the audio thread, and loading a file is not
    safe from the loader thread either, so setVideoFile() only records what is
    wanted and the timer performs the load.
*/
class AudioPlayer : public AudioIODeviceCallback,
                    private Timer {
public:

    AudioPlayer(File audioSettingsFile);

    ~AudioPlayer() override;

    void resetCurrentDevice(File audioSettingsFile);

    void start();

    void stop();

    int getSampleRate();

    BigInteger getOutputChannels() {
        AudioDeviceManager::AudioDeviceSetup deviceSettings;
        audioDeviceManager.getAudioDeviceSetup(deviceSettings);
        return deviceSettings.outputChannels;
    }

    int getOutputChannelCount() { return getOutputChannels().countNumberOfSetBits(); }

    void audioDeviceIOCallback(const float **inputChannelData,
                               int numInputChannels,
                               float **outputChannelData,
                               int totalNumOutputChannels,
                               int numOutSamples) override;


    void audioDeviceAboutToStart(AudioIODevice *device) override;

    void audioDeviceStopped() override;

    void switchStimulus(int newStimulusNumber) { nextStimulus = newStimulusNumber; }

    int getCurrentStimulus() { return currentStimulus.load(); }

    int getNextStimulus() { return nextStimulus.load(); }

    int getChannelCount() { return channelCount.load(); }

    double getCurrentPosition() {
        const int total = totalSamples.load();
        return total > 0 ? (double) currentSample.load() / total : 0.0;
    }

    double getCurrentTime() {
        const double rate = getSampleRate();
        return rate > 0.0 ? (double) currentSample.load() / rate : 0.0;
    }

    bool isRunning() { return playerRunning.load(); }

    bool isPaused() { return playerPaused.load(); }

    int getCurrentSample() { return currentSample.load(); }

    void setPlayLoop(bool shouldPlayLoop) { playInLoop = shouldPlayLoop; }

    void setSample(int newSample) { currentSample = newSample; }

    void setTotalSamples(int newTotalSamples) { totalSamples = newTotalSamples; }

    void setFragmentStartSample(int newStartSample) { startSample = newStartSample; }

    void setFragmentEndSample(int newEndSample) { endSample = newEndSample; }

    bool setPosition(float newPositionPercent);

    void setFragmentStartPosition(float newPositionPercent) {
        setFragmentStartSample((int) std::floor(newPositionPercent * totalSamples.load()));
    }

    void setFragmentEndPosition(float newPositionPercent) {
        setFragmentEndSample((int) std::floor(newPositionPercent * totalSamples.load()));
    }

    void setChannelCount(int chanCount);

    void pause() { playerPaused = true; }

    void resume() { playerPaused = false; }

    void releaseAllAudioData();

    void addAudioFromFile(AudioFormatReader *wavReader, int chanCount, int numSamps);

    /* Safe to call from any thread; the file is opened by the message thread. */
    void setVideoFile(const File &file);

    void clearVideoFile() { setVideoFile(File()); }

    VideoComponent *getVideoComponent() { return &videoComponent; }

    std::unique_ptr <XmlElement> createStateXml() { return audioDeviceManager.createStateXml(); }

    AudioDeviceManager &getAudioDeviceManager() {
        return audioDeviceManager;
    }


private:
    /* Message thread: keeps the video component in step with the audio. */
    void timerCallback() override;

    void updateVideoFile();

    void syncVideoToAudio();

    /* Audio thread: renders one block of the selected stimulus into mixBuffer. */
    void renderNextBlock(int numChannels, int numSamples);

    void copyStimulusInto(AudioBuffer<float> &destination, int stimulusIndex, int numChannels,
                          int position, int samplesToCopy, int loopStart, int leftoverSamples);

    /* Message or loader thread: never called from the audio callback. */
    void allocateMixBuffers();

    std::atomic<int> currentSample;
    std::atomic<int> totalSamples;
    std::atomic<int> startSample;
    std::atomic<int> endSample;
    std::atomic<bool> playInLoop;
    std::atomic<int> currentStimulus;
    std::atomic<int> nextStimulus;
    std::atomic<bool> playerRunning;
    std::atomic<bool> playerPaused;
    std::atomic<int> channelCount;
    std::atomic<double> currentSampleRate;
    std::atomic<int> currentBlockSize;

    AudioDeviceManager audioDeviceManager;
    OwnedArray <AudioBuffer<float>> audioStimData;

    /* Pre-allocated so that the audio callback never allocates. */
    AudioBuffer<float> mixBuffer;
    AudioBuffer<float> fadeBuffer;

    VideoComponent videoComponent;
    File requestedVideoFile;
    CriticalSection videoFileLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayer);
};


#endif
