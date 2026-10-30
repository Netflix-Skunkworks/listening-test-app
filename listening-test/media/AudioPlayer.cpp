// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#include "AudioPlayer.h"
#include "TrialMediaLoader.h"

AudioPlayer::AudioPlayer(File settings, bool initialiseDevice)
    : device(engine), video(engine) {
    if (initialiseDevice) device.reset(settings);
}

void AudioPlayer::start() {
    device.attach();
    video.start();
}

void AudioPlayer::stop() {
    device.detach();
    video.stop();
}

bool AudioPlayer::setPosition(float fraction) {
    if (!engine.setPosition(fraction)) return false;
    video.seek(fraction);
    return true;
}

void AudioPlayer::replaceTrialMedia(LoadedTrialMedia& media) {
    engine.replaceMedia(media.audio, media.samples, media.channels);
    video.setVideoFile(media.video);
}
