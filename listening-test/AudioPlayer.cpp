//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "AudioPlayer.h"
#include "TestLauncher.h"

const int doCrossFade = true;  // Typically it sounds better with a crossfade.  Some BS.1116 tests require it to be turned off!

//==============================================================================
AudioPlayer::AudioPlayer(File audioSettingsFile) :
        currentSample(0),
        totalSamples(1), // don't use zero to avoid divide-by-zero problems in playback slider when no test is loaded
        startSample(0),
        endSample(0),
        playInLoop(true),
        currentStimulus(-1),
        nextStimulus(-1),
        playerRunning(false),
        playerPaused(false),
        channelCount(0),
        videoComponent(false),
        videoFile(new File(String())) {
    resetCurrentDevice(audioSettingsFile);
}

//==============================================================================
AudioPlayer::~AudioPlayer() {
}

void AudioPlayer::resetCurrentDevice(File audioSettingsFile) {
    std::unique_ptr <XmlElement> deviceState(parseXML(audioSettingsFile));
    audioDeviceManager.initialise(MAXNUMBEROFDEVICECHANNELS, MAXNUMBEROFDEVICECHANNELS, deviceState.get(), false);
    audioDeviceManager.addAudioCallback(this);
}

int AudioPlayer::getSampleRate() {
    AudioDeviceManager::AudioDeviceSetup ads;
    audioDeviceManager.getAudioDeviceSetup(ads);
    return (ads.sampleRate);
}


// ============================================================================================
bool AudioPlayer::setPosition(float newPositionPercent) {
    unsigned int newSamp(static_cast<unsigned int> (floor(newPositionPercent * totalSamples)));
    if (newSamp < totalSamples) {
        setSample(newSamp);
        if (videoComponent.isVideoOpen()) {
            videoComponent.setPlayPosition(newPositionPercent * videoComponent.getVideoDuration());
        }

        return true;
    } else {
        return false;
    }
}

//==============================================================================
void AudioPlayer::audioDeviceAboutToStart(AudioIODevice *device) {
    AudioDeviceManager::AudioDeviceSetup ads;
    audioDeviceManager.getAudioDeviceSetup(ads);

    currentStimulus = -1;
    nextStimulus = -1;

    playerRunning = true;
    playerPaused = true;

    if (videoFile->exists() && (videoComponent.getCurrentVideoFile() != *videoFile)) {
        videoComponent.load(*videoFile);
    }
}


//==============================================================================
void AudioPlayer::audioDeviceStopped() {
    playerRunning = false;
}

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))

void AudioPlayer::start() {
    if (!isRunning()) {
        audioDeviceManager.addAudioCallback(this);
    }
}

void AudioPlayer::stop() {
    if (isRunning()) {
        audioDeviceManager.removeAudioCallback(this);
    }

    if (videoComponent.isVideoOpen() && videoComponent.isPlaying())
        videoComponent.stop();
}

void AudioPlayer::addAudioFromFile(AudioFormatReader *wavReader, int chanCount, int numSamps) {
    jassert(channelCount == chanCount);
    AudioBuffer<float> *audioBuffer = new AudioBuffer<float>(chanCount, numSamps);
    wavReader->read(audioBuffer, 0, numSamps, 0, false, false);
    audioStimData.add(audioBuffer);
}

void AudioPlayer::releaseAllAudioData() {
    audioStimData.clear();
}

//==============================================================================
void AudioPlayer::audioDeviceIOCallbackWithContext(const float *const * /*inputChannelData*/,
                                                   int /*numInputChannels*/,
                                                   float *const *outputChannelData,
                                                   int totalNumOutputChannels,
                                                   int numOutSamples,
                                                   const AudioIODeviceCallbackContext &context) {
    ignoreUnused(context);

    AudioBuffer<float> outputBuffer;
    outputBuffer.setDataToReferTo(outputChannelData, channelCount, numOutSamples);
    outputBuffer.clear();

    if (!playerPaused) {
        if (videoComponent.isVideoOpen()) {
            int vidPosInSamples = videoComponent.getPlayPosition() * getSampleRate();

            if (abs(vidPosInSamples - currentSample) > 2000) {
                videoComponent.setPlayPosition((double) currentSample / getSampleRate());
            }

            if (!videoComponent.isPlaying()) {
                videoComponent.play();
            }
        }

        /* Copy as much as possible from input file */
        int samplesToCopy = max(0, min(numOutSamples, endSample - currentSample));
        int leftoverSamples = numOutSamples - samplesToCopy;

        assert(leftoverSamples <= (endSample - startSample));
        if (currentStimulus != -1) {
            /* continue playing the stimulus */
            for (int ch = 0; ch < channelCount; ch++) {
                outputBuffer.copyFrom(ch, 0, *audioStimData[currentStimulus], ch, currentSample, samplesToCopy);
            }

            /* if looping, read the rest of samples from the beginning of the loop */
            if (leftoverSamples > 0 && playInLoop) {
                for (int ch = 0; ch < channelCount; ch++) {
                    /* Read the rest from the start of the file */
                    outputBuffer.copyFrom(ch, samplesToCopy, *audioStimData[currentStimulus], ch, startSample,
                                          leftoverSamples);
                }
            }

            /* cross-fade if stimuli were switched */
            if (currentStimulus != nextStimulus) {
                if (doCrossFade) {
                    AudioBuffer<float> fadeInBuffer(channelCount, numOutSamples);
                    fadeInBuffer.clear();
                    for (int ch = 0; ch < channelCount; ch++) {
                        fadeInBuffer.copyFrom(ch, 0, *audioStimData[nextStimulus], ch, currentSample, samplesToCopy);
                        if (playInLoop) {
                            fadeInBuffer.copyFrom(ch, samplesToCopy, *audioStimData[nextStimulus], ch, startSample,
                                                  leftoverSamples);
                        }

                        /* do cross-fade */
                        outputBuffer.applyGainRamp(ch, 0, numOutSamples, 1.0f, 0.0f); // fade-out
                        outputBuffer.addFromWithRamp(ch, 0, fadeInBuffer.getReadPointer(ch, 0), numOutSamples, 0.0f,
                                                     1.0f); // add with fade-in
                    }
                }
                currentStimulus = nextStimulus;
            }
        } else if (nextStimulus != -1) {
            /* start playing the first selected stimulus */
            for (int ch = 0; ch < channelCount; ch++) {
                outputBuffer.copyFrom(ch, 0, *audioStimData[nextStimulus], ch, currentSample, samplesToCopy);
                if (leftoverSamples > 0 && playInLoop) {
                    outputBuffer.copyFrom(ch, samplesToCopy, *audioStimData[nextStimulus], ch, startSample,
                                          leftoverSamples);
                }
            }
            currentStimulus = nextStimulus;
        }

        /* advance audio buffer */
        currentSample += numOutSamples;
        if (currentSample >= endSample) {
            if (playInLoop) {
                currentSample = startSample + currentSample - endSample;
            } else {
                pause();
                videoComponent.stop();
                currentSample = startSample;
            }
        }
    } else if (videoComponent.isPlaying()) {
        videoComponent.stop();
    }
}
