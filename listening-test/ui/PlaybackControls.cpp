// Listening Test Application
// Copyright(C) 2026 Netflix, Inc.
#include "PlaybackControls.h"
#include "../media/AudioPlayer.h"
#include "AppTheme.h"

PlaybackControls::PlaybackControls(AudioPlayer& audio) : player(audio) {
    for (auto* child : std::initializer_list<Component*> {
             &border, &position, &restart, &previous, &play, &next, &loop, &lock, &time })
        addAndMakeVisible(child);
    border.setText("Playback Controls");
    border.setTextLabelPosition(Justification::centred);
    position.setRange(0, AppTheme::positionMaximum, 1);
    position.setSliderStyle(Slider::ThreeValueHorizontal);
    position.setTextBoxStyle(Slider::NoTextBox, false, 80, 20);
    position.setMinAndMaxValues(0, AppTheme::positionMaximum, dontSendNotification);
    position.setChangeNotificationOnlyOnRelease(false);
    position.onValueChange = [this] { sliderChanged(); };
    position.onDragStart = [this] { if (active) { dragging = true; stopTimer(); } };
    position.onDragEnd = [this] {
        dragging = false;
        if (active) startTimer(AppTheme::transportRefreshMs);
    };
    restart.onClick = [this] { perform(PlaybackAction::restart); };
    previous.onClick = [this] { perform(PlaybackAction::previousLoop); };
    play.onClick = [this] { perform(PlaybackAction::toggle); };
    next.onClick = [this] { perform(PlaybackAction::nextLoop); };
    restart.setConnectedEdges(Button::ConnectedOnRight);
    previous.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
    play.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
    next.setConnectedEdges(Button::ConnectedOnLeft);
    for (auto* button : { &restart, &previous, &play, &next })
        button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);
    loop.setToggleState(true, dontSendNotification);
    loop.onClick = [this] { if (active) player.setPlayLoop(loop.getToggleState()); };
    lock.onClick = [this] { if (active) lockLoop = lock.getToggleState(); };
    time.setText("00:00.0", dontSendNotification);
    time.setJustificationType(Justification::centredLeft);
    setEnabled(false);
}

PlaybackControls::~PlaybackControls() { stopTimer(); }

void PlaybackControls::setActive(bool value) {
    active = value;
    setEnabled(value);
    player.pause();
    if (active) {
        if (onStartDevice) onStartDevice();
        startTimer(AppTheme::transportRefreshMs);
    } else {
        dragging = false;
        player.stop();
        stopTimer();
    }
    refresh();
}

void PlaybackControls::configure(juce::int64 length) {
    samples = length;
    reset();
}

void PlaybackControls::reset() {
    player.pause();
    loopStart = 0;
    loopEnd = AppTheme::positionMaximum;
    lockLoop = false;
    loop.setToggleState(true, dontSendNotification);
    lock.setToggleState(false, dontSendNotification);
    position.setMinAndMaxValues(loopStart, loopEnd, dontSendNotification);
    position.setValue(0, dontSendNotification);
    player.setPlayLoop(true);
    player.setFragmentStartPosition(0);
    player.setFragmentEndPosition(1);
    player.setPosition(0);
    refresh();
}

void PlaybackControls::perform(PlaybackAction action) {
    // Queued clicks and late view callbacks cannot reactivate a hidden/finished test.
    if (!active) return;
    switch (action) {
        case PlaybackAction::play:
            if (!player.isRunning() && onStartDevice) onStartDevice();
            player.resume();
            break;
        case PlaybackAction::pause: player.pause(); break;
        case PlaybackAction::toggle:
            perform(player.isRunning() && !player.isPaused() ? PlaybackAction::pause : PlaybackAction::play);
            return;
        case PlaybackAction::restart:
            position.setValue(loopStart, dontSendNotification);
            player.setPosition(float(loopStart / AppTheme::positionMaximum));
            break;
        case PlaybackAction::previousLoop: shiftLoop(-1); break;
        case PlaybackAction::nextLoop: shiftLoop(1); break;
        case PlaybackAction::setLoopStart:
            position.setMinValue(position.getValue(), dontSendNotification); sliderChanged(); break;
        case PlaybackAction::setLoopEnd:
            position.setMaxValue(position.getValue(), dontSendNotification); sliderChanged(); break;
        case PlaybackAction::clearLoopStart:
            position.setMinValue(0, dontSendNotification); sliderChanged(); break;
        case PlaybackAction::clearLoopEnd:
            position.setMaxValue(AppTheme::positionMaximum, dontSendNotification); sliderChanged(); break;
    }
    refresh();
}

bool PlaybackControls::handleKey(const KeyPress& key) {
    if (!active) return false;
    if (key.isKeyCode(KeyPress::spaceKey)) perform(PlaybackAction::toggle);
    else if (key.isKeyCode(KeyPress::rightKey)) perform(PlaybackAction::nextLoop);
    else if (key.isKeyCode(KeyPress::leftKey)) perform(PlaybackAction::previousLoop);
    else if (key.isKeyCode(KeyPress::homeKey)) perform(PlaybackAction::restart);
    else if (key.isKeyCode('X')) perform(key.getModifiers().isShiftDown() ? PlaybackAction::clearLoopStart : PlaybackAction::setLoopStart);
    else if (key.isKeyCode('C')) perform(key.getModifiers().isShiftDown() ? PlaybackAction::clearLoopEnd : PlaybackAction::setLoopEnd);
    else return false;
    return true;
}

void PlaybackControls::shiftLoop(int direction) {
    const auto width = loopEnd - loopStart;
    const auto start = jlimit(0.0, double(AppTheme::positionMaximum) - width, loopStart + direction * width);
    position.setMinAndMaxValues(start, start + width, dontSendNotification);
    sliderChanged();
}

void PlaybackControls::sliderChanged() {
    if (!active) return;
    double minimum = samples > 0 ? jmin(double(AppTheme::positionMaximum),
        AppTheme::positionMaximum * AppTheme::minimumLoopSeconds * player.getSampleRate() / samples) : 0;
    if (lockLoop) minimum = loopEnd - loopStart;
    auto start = position.getMinValue(), end = position.getMaxValue();
    const auto cursor = position.getValue();
    const bool startChanged = start != loopStart, endChanged = end != loopEnd;
    if (startChanged || endChanged) {
        if (lockLoop) {
            start = startChanged ? jlimit(0.0, double(AppTheme::positionMaximum) - minimum, start)
                                 : jlimit(0.0, double(AppTheme::positionMaximum) - minimum, end - minimum);
            end = start + minimum;
        } else if (startChanged && !endChanged) {
            start = jlimit(0.0, jmax(0.0, end - minimum), start);
        } else {
            start = jlimit(0.0, double(AppTheme::positionMaximum) - minimum, start);
            end = jlimit(jmin(double(AppTheme::positionMaximum), start + minimum), double(AppTheme::positionMaximum), end);
        }
        loopStart = start;
        loopEnd = end;
        position.setMinAndMaxValues(start, end, dontSendNotification);
        player.setFragmentStartPosition(float(start / AppTheme::positionMaximum));
        player.setFragmentEndPosition(float(end / AppTheme::positionMaximum));
    } else {
        player.setPosition(float(cursor / AppTheme::positionMaximum));
    }
    const auto playback = player.getCurrentPosition() * AppTheme::positionMaximum;
    if (playback < loopStart || playback > loopEnd) {
        position.setValue(loopStart, dontSendNotification);
        player.setPosition(float(loopStart / AppTheme::positionMaximum));
    }
}

void PlaybackControls::refresh() {
    play.setIcon(player.isRunning() && !player.isPaused() ? TransportButton::Icon::pause : TransportButton::Icon::play);
    time.setText(String::formatted("%3.1f", player.getCurrentTime()), dontSendNotification);
}

void PlaybackControls::timerCallback() {
    if (!active || dragging) return;
    position.setValue(std::floor(AppTheme::positionMaximum * player.getCurrentPosition()), dontSendNotification);
    refresh();
}

void PlaybackControls::resized() {
    border.setBounds(getLocalBounds());
    position.setBounds(92, 20, jmax(0, getWidth() - 200), 25);
    loop.setBounds(16, 20, 64, 25);
    time.setBounds(position.getRight() + 20, 20, 80, 25);
    lock.setBounds(16, getHeight() - 32, 150, 25);
    const int x = jmax(position.getX() + position.getWidth() / 2 - 200, lock.getRight());
    const int width = jmax(0, jmin(100, (position.getRight() - x) / 4));
    int left = x;
    for (auto* button : { &restart, &previous, &play, &next }) {
        button->setBounds(left, getHeight() - 32, width, 25);
        left += width;
    }
}
