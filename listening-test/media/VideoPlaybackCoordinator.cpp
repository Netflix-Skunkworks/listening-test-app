// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#include "VideoPlaybackCoordinator.h"
#include "AudioEngine.h"

namespace {
constexpr int VIDEO_SYNC_TOLERANCE_SAMPLES = 2000;
}

VideoPlaybackCoordinator::VideoPlaybackCoordinator(const AudioEngine& source)
    : position(source), videoComponent(false) {}

VideoPlaybackCoordinator::~VideoPlaybackCoordinator() { stopTimer(); }

void VideoPlaybackCoordinator::start() { startTimer(20); }
void VideoPlaybackCoordinator::stop() {
    stopTimer();
    if (videoComponent.isVideoOpen() && videoComponent.isPlaying()) videoComponent.stop();
}

void VideoPlaybackCoordinator::seek(double fraction) {
    if (videoComponent.isVideoOpen())
        videoComponent.setPlayPosition(fraction * videoComponent.getVideoDuration());
}

void VideoPlaybackCoordinator::setVideoFile(const File &file) {
    const ScopedLock sl(videoFileLock);
    requestedVideoFile = file;
}

void VideoPlaybackCoordinator::timerCallback() {
    updateVideoFile();
    syncVideoToAudio();
}

void VideoPlaybackCoordinator::updateVideoFile() {
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

void VideoPlaybackCoordinator::syncVideoToAudio() {
    if (!videoComponent.isVideoOpen()) {
        return;
    }

    const auto state = position.playbackPosition();
    if (state.running && !state.paused) {
        const double sampleRate = state.sampleRate;
        if (sampleRate > 0.0) {
            const double audioSeconds = state.seconds();
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
