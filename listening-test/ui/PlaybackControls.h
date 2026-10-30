// Listening Test Application
// Copyright(C) 2026 Netflix, Inc.
#pragma once

class AudioPlayer;
#include "PlaybackAction.h"
#include "TestViewControls.h"

// Message-thread transport UI. Owns its timer and loop edits, not the test session.
class PlaybackControls : public Component, private Timer {
public:
    explicit PlaybackControls(AudioPlayer&);
    ~PlaybackControls() override;
    std::function<void()> onStartDevice;
    void setActive(bool);
    bool isActive() const { return active; }
    void configure(juce::int64 lengthInSamples);
    void reset();
    void perform(PlaybackAction);
    bool handleKey(const KeyPress&);
    void resized() override;
private:
    void timerCallback() override;
    void sliderChanged();
    void refresh();
    void shiftLoop(int direction);
    AudioPlayer& player;
    bool active = false, lockLoop = false, dragging = false;
    double loopStart = 0, loopEnd = 100;
    juce::int64 samples = 0;
    GroupComponent border;
    Slider position;
    TransportButton restart { TransportButton::Icon::restart };
    TransportButton previous { TransportButton::Icon::rewind };
    TransportButton play { TransportButton::Icon::play };
    TransportButton next { TransportButton::Icon::fastForward };
    ToggleButton loop { "loop" }, lock { "lock loop interval" };
    Label time;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaybackControls)
};
