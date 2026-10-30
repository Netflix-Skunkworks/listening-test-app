// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov; Copyright(C) 2017-2026 Netflix, Inc.
#pragma once
#include <JuceHeader.h>
class AudioEngine;

// Construct, destroy, seek, and control on the message thread. The source must
// outlive this coordinator. Only setVideoFile may be called from other threads;
// loading and every VideoComponent operation run on the message-thread timer.
class VideoPlaybackCoordinator : private Timer {
public:
    explicit VideoPlaybackCoordinator(const AudioEngine&);
    ~VideoPlaybackCoordinator() override;
    void start();
    void stop();
    void seek(double fraction);
    void setVideoFile(const File&);
    VideoComponent* getComponent() { return &videoComponent; }
private:
    void timerCallback() override;
    void updateVideoFile();
    void syncVideoToAudio();
    const AudioEngine& position;
    VideoComponent videoComponent;
    File requestedVideoFile;
    CriticalSection videoFileLock;
};
