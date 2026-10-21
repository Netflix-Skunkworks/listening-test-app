//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "AudioPlayer.h"
#include "TestLauncher.h"

const int doCrossFade = true;  // Typically it sounds better with a crossfade.  Some BS.1116 tests require it to be turned off!

/* How far the video may drift from the audio before it is re-seeked.  Held in
   samples, but converted using the device's own rate. */
const int VIDEO_SYNC_TOLERANCE_SAMPLES = 2000;

/* How often the message thread checks the video against the audio position. */
const int VIDEO_SYNC_INTERVAL_MS = 20;

/* Headroom over the device's reported block size.  The block size is allowed to
   vary between callbacks, and a block larger than these buffers is rendered as
   silence rather than risking an out-of-range write. */
const int MINIMUM_BLOCK_ALLOCATION = 1024;

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
        currentSampleRate(0.0),
        currentBlockSize(0),
        videoComponent(false) {
    resetCurrentDevice(audioSettingsFile);
}

//==============================================================================
AudioPlayer::~AudioPlayer() {
    /* Both callbacks have to be stopped here rather than left to the base
       classes: this object's members are destroyed before ~Timer runs, and the
       device manager still holds this as a callback until it is removed. */
    stopTimer();
    audioDeviceManager.removeAudioCallback(this);
}

void AudioPlayer::resetCurrentDevice(File audioSettingsFile) {
    std::unique_ptr <XmlElement> deviceState(parseXML(audioSettingsFile));
    audioDeviceManager.initialise(MAXNUMBEROFDEVICECHANNELS, MAXNUMBEROFDEVICECHANNELS, deviceState.get(), false);
    audioDeviceManager.addAudioCallback(this);
}

int AudioPlayer::getSampleRate() {
    /* Prefer the rate cached when the device started: this is called from the
       GUI timer, and querying the device manager takes a lock. */
    const double cachedRate = currentSampleRate.load();
    if (cachedRate > 0.0) {
        return (int) cachedRate;
    }

    AudioDeviceManager::AudioDeviceSetup ads;
    audioDeviceManager.getAudioDeviceSetup(ads);
    return (int) ads.sampleRate;
}


// ============================================================================================
bool AudioPlayer::setPosition(float newPositionPercent) {
    const int total = totalSamples.load();
    const int newSamp = (int) std::floor(newPositionPercent * total);

    if (newSamp >= 0 && newSamp < total) {
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
    currentStimulus = -1;
    nextStimulus = -1;

    if (device != nullptr) {
        currentSampleRate = device->getCurrentSampleRate();
        currentBlockSize = device->getCurrentBufferSizeSamples();
    }

    /* This is the documented place to size working buffers.  It may be called
       from the device's own thread, but never concurrently with the callback. */
    allocateMixBuffers();

    playerRunning = true;
    playerPaused = true;
}


//==============================================================================
void AudioPlayer::audioDeviceStopped() {
    playerRunning = false;
}

void AudioPlayer::start() {
    if (!isRunning()) {
        audioDeviceManager.addAudioCallback(this);
    }

    /* Drives video playback; harmless when no video is loaded. */
    startTimer(VIDEO_SYNC_INTERVAL_MS);
}

void AudioPlayer::stop() {
    if (isRunning()) {
        audioDeviceManager.removeAudioCallback(this);
    }

    stopTimer();

    if (videoComponent.isVideoOpen() && videoComponent.isPlaying())
        videoComponent.stop();
}

void AudioPlayer::setChannelCount(int chanCount) {
    channelCount = chanCount;
    allocateMixBuffers();
}

void AudioPlayer::allocateMixBuffers() {
    const int channels = jmax(1, channelCount.load());
    const int samples = jmax(MINIMUM_BLOCK_ALLOCATION, currentBlockSize.load());

    mixBuffer.setSize(channels, samples, false, true, false);
    fadeBuffer.setSize(channels, samples, false, true, false);
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
void AudioPlayer::setVideoFile(const File &file) {
    const ScopedLock sl(videoFileLock);
    requestedVideoFile = file;
}

void AudioPlayer::timerCallback() {
    updateVideoFile();
    syncVideoToAudio();
}

void AudioPlayer::updateVideoFile() {
    File wanted;
    {
        const ScopedLock sl(videoFileLock);
        wanted = requestedVideoFile;
    }

    if (wanted == videoComponent.getCurrentVideoFile()) {
        return;
    }

    /* Stop a video from an earlier audio-video test continuing to play, and its
       own soundtrack with it, underneath a test that has no video of its own. */
    if (videoComponent.isVideoOpen()) {
        videoComponent.closeVideo();
    }

    if (wanted.existsAsFile()) {
        Result loadResult = videoComponent.load(wanted);
        if (loadResult.failed()) {
            Logger::writeToLog("Could not load video " + wanted.getFullPathName() + ": " +
                               loadResult.getErrorMessage());
        }
    }
}

void AudioPlayer::syncVideoToAudio() {
    if (!videoComponent.isVideoOpen()) {
        return;
    }

    if (playerRunning.load() && !playerPaused.load()) {
        const double sampleRate = currentSampleRate.load();
        if (sampleRate > 0.0) {
            const double audioSeconds = (double) currentSample.load() / sampleRate;
            const double tolerance = (double) VIDEO_SYNC_TOLERANCE_SAMPLES / sampleRate;

            if (std::abs(videoComponent.getPlayPosition() - audioSeconds) > tolerance) {
                videoComponent.setPlayPosition(audioSeconds);
            }
        }

        if (!videoComponent.isPlaying()) {
            videoComponent.play();
        }
    } else if (videoComponent.isPlaying()) {
        videoComponent.stop();
    }
}

//==============================================================================
void AudioPlayer::copyStimulusInto(AudioBuffer<float> &destination, int stimulusIndex, int numChannels,
                                   int position, int samplesToCopy, int loopStart, int leftoverSamples) {
    const AudioBuffer<float> *source = audioStimData[stimulusIndex];
    if (source == nullptr) {
        return;
    }

    const int channels = jmin(numChannels, source->getNumChannels());
    const int sourceLength = source->getNumSamples();

    /* Clamp both reads against what the source actually holds.  The loop points
       come from the playback slider and are not validated there. */
    const int head = jlimit(0, jmax(0, sourceLength - position), samplesToCopy);
    const int tail = jlimit(0, jmax(0, sourceLength - loopStart), leftoverSamples);

    for (int ch = 0; ch < channels; ch++) {
        if (head > 0) {
            destination.copyFrom(ch, 0, *source, ch, position, head);
        }
        if (tail > 0) {
            destination.copyFrom(ch, samplesToCopy, *source, ch, loopStart, tail);
        }
    }
}

void AudioPlayer::renderNextBlock(int numChannels, int numSamples) {
    /* One snapshot of the shared playback state per block.  These are written
       by the message thread while this runs, so reading them repeatedly could
       mix values from different moments. */
    const int bufferLength = jmax(0, totalSamples.load());
    const bool loop = playInLoop.load();
    const int wanted = nextStimulus.load();
    int playing = currentStimulus.load();
    int position = jlimit(0, bufferLength, currentSample.load());

    const int loopStart = jlimit(0, bufferLength, startSample.load());
    const int loopEnd = jlimit(loopStart, bufferLength, endSample.load());

    const int samplesToCopy = jlimit(0, numSamples, loopEnd - position);
    const int leftoverSamples = loop ? jmin(numSamples - samplesToCopy, loopEnd - loopStart) : 0;

    mixBuffer.clear(0, numSamples);

    if (playing >= 0) {
        copyStimulusInto(mixBuffer, playing, numChannels, position, samplesToCopy, loopStart, leftoverSamples);

        if (playing != wanted && wanted >= 0) {
            const bool canCrossFade = doCrossFade
                                      && fadeBuffer.getNumChannels() >= numChannels
                                      && fadeBuffer.getNumSamples() >= numSamples;

            if (canCrossFade) {
                fadeBuffer.clear(0, numSamples);
                copyStimulusInto(fadeBuffer, wanted, numChannels, position, samplesToCopy, loopStart,
                                 leftoverSamples);

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
        copyStimulusInto(mixBuffer, wanted, numChannels, position, samplesToCopy, loopStart, leftoverSamples);
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

//==============================================================================
void AudioPlayer::audioDeviceIOCallback(const float ** /*inputChannelData*/,
                                        int /*numInputChannels*/,
                                        float **outputChannelData,
                                        int totalNumOutputChannels,
                                        int numOutSamples) {
    const int channelsToPlay = jmin(channelCount.load(), totalNumOutputChannels, mixBuffer.getNumChannels());

    const bool rendered = !playerPaused.load()
                          && channelsToPlay > 0
                          && numOutSamples <= mixBuffer.getNumSamples();

    if (rendered) {
        renderNextBlock(channelsToPlay, numOutSamples);
    }

    for (int ch = 0; ch < totalNumOutputChannels; ch++) {
        float *out = outputChannelData[ch];
        if (out == nullptr) {
            continue;
        }

        if (rendered && ch < channelsToPlay) {
            FloatVectorOperations::copy(out, mixBuffer.getReadPointer(ch), numOutSamples);
        } else {
            FloatVectorOperations::clear(out, numOutSamples);
        }
    }
}
