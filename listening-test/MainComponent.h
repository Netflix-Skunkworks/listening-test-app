//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef MAIN_COMPONENT_H
#define MAIN_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "guisettings.h"

#include "TestManagerComponent.h"
#include "NewTestSelectComponent.h"
#include "SurveyComponent.h"

#include "DemoComponent.h"
#include "MushraComponent.h"
#include "BS1116Component.h"
#include "ABTestComponent.h"
#include "AVABTestComponent.h"

#include "BasicSurveyComponent.h"

#define CONCEAL_TRIAL_NAMES    // When defined, the test border will not indicate the name of each trial.


class MainComponent : public Component,
                      public ChangeListener,
                      public KeyListener,
                      public Timer {
public:
    MainComponent();

    ~MainComponent();

    void timerCallback();

    void changeListenerCallback(ChangeBroadcaster *o);

    void paint(Graphics &);

    void resized();

    bool keyPressed(const KeyPress &, Component *);

    bool keyPressed(const KeyPress &k) { return keyPressed(k, this); }


private:
    void saveAudioDeviceSettings();

    void setupChildTestComponent();
                          
    void changeVisibleComponent(Component*);

    void runContinueTestSelector();

    void handlePlaybackSliderChange();

    void loadNewTest(NewTestSelectComponent &);

    void resetPlayback();

    void startPlayback() {
        if (audioPlayer.isPaused() || !audioPlayer.isRunning())
            togglePlayback();
    }

    void togglePlayback() {
        if (!audioPlayer.isRunning()) {
            sliderListener.sliderValueChanged(&playbackSlider);
            audioPlayer.start();
            audioPlayer.resume();
            playButton.setButtonText("||");
            startTimer(TIMER_CALLBACK_INTERVAL_MS);
        } else {
            if (audioPlayer.isPaused()) {
                audioPlayer.resume();
                playButton.setButtonText("||");
            } else {
                audioPlayer.pause();
                playButton.setButtonText("|>");
            }
        }
    }

    class MyButtonListener : public Button::Listener {
    public:
        MyButtonListener(MainComponent &c) : owner(c) {};

        ~MyButtonListener() {};

        void buttonClicked(Button *b) {
            if (b == &owner.playButton) {
                owner.togglePlayback();
            } else if (b == &owner.audioSetupButton) {
                owner.audioSetupButton.setToggleState(!owner.audioSetupButton.getToggleState(), dontSendNotification);
                if (owner.audioSetupButton.getToggleState()) {
                    owner.changeVisibleComponent(&owner.deviceSelector);
                } else {
                    owner.saveAudioDeviceSettings();
                    owner.changeVisibleComponent(owner.testComponent);
                }
            } else if (b == &owner.manageTestsButton) {
                owner.manageTestsButton.setToggleState(!owner.manageTestsButton.getToggleState(), dontSendNotification);
                if (owner.manageTestsButton.getToggleState()) {
                    owner.testManagerComponent.refreshTestSpecs();
                    owner.changeVisibleComponent(&owner.testManagerComponent);
                } else {
                    owner.changeVisibleComponent(owner.testComponent);
                }
            } else if (b == &owner.newTestButton) {
                owner.newTestButton.setToggleState(!owner.newTestButton.getToggleState(), dontSendNotification);
                if (owner.newTestButton.getToggleState()) {
                    /* Prompt for new test if none exist */
                    if (!owner.newTestSelectComponent.populateComboBoxWithTestNames()) {
                        AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error occurred",
                                                    "Could not open test settings files, please re-run test setup manager", "OK",
                                                    &owner);
                    }
                    owner.changeVisibleComponent(&owner.newTestSelectComponent);
                } else {
                    owner.changeVisibleComponent(owner.testComponent);
                }
            } else if (b == &owner.loadTestButton) {
                owner.loadTestButton.setToggleState(!owner.loadTestButton.getToggleState(), dontSendNotification);
                owner.runContinueTestSelector();
            } else if (b == &owner.showHotkeysButton) {
                owner.showHotkeysButton.setToggleState(!owner.showHotkeysButton.getToggleState(), dontSendNotification);
                if (owner.showHotkeysButton.getToggleState())
                {
                    owner.changeVisibleComponent(&owner.hotkeyTextLabel);
                } else {
                    owner.changeVisibleComponent(owner.testComponent);
                }
            } else if (b == &owner.loopToggleButton) {
                owner.audioPlayer.setPlayLoop(owner.loopToggleButton.getToggleState());
            } else if (b == &owner.lockLoopToggleButton) {
                owner.lockLoop = !owner.lockLoop;
            } else if (b == &owner.rewindButton) {
                owner.playbackSlider.setValue(owner.playbackSliderMin, sendNotificationAsync);
                owner.audioPlayer.setPosition(owner.playbackSliderMin / SLIDER_MAXVALUE);
            } else if (b == &owner.backButton) {
                float interval = owner.playbackSliderMax - owner.playbackSliderMin;
                float newMin = max(0, owner.playbackSliderMin - interval);
                owner.playbackSlider.setMinAndMaxValues(newMin, newMin + interval);
            } else if (b == &owner.forwardButton) {
                float interval = owner.playbackSliderMax - owner.playbackSliderMin;
                float newMax = min(SLIDER_MAXVALUE, owner.playbackSliderMax + interval);
                owner.playbackSlider.setMinAndMaxValues(newMax - interval, newMax);
            }
        }

    private:
        MainComponent &owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyButtonListener);

    } buttonListener;

    class MySliderListener : public Slider::Listener {
    public:
        MySliderListener(MainComponent &c) : owner(c) {};

        ~MySliderListener() {};

        void sliderDragStarted(Slider *s) {
            if (s == &owner.playbackSlider) {
                owner.stopTimer();
            }
        }

        void sliderDragEnded(Slider *s) {
            if (s == &owner.playbackSlider) {
                owner.startTimer(TIMER_CALLBACK_INTERVAL_MS);
            }
        }

        void sliderValueChanged(Slider *s) {
            if (s == &owner.playbackSlider) {
                owner.handlePlaybackSliderChange();
            }
        }

    private:
        MainComponent &owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MySliderListener);

    } sliderListener;
    
    // Replace with your own survey if you wish
    BasicSurveyComponent bsc;
    SurveyComponent& surveyComponent = bsc;

    /* state */
    float playbackSliderMin;
    float playbackSliderMax;
    bool lockLoop;

    AudioPlayer audioPlayer;
    AudioDeviceSelectorComponent deviceSelector;
    TestLauncher testLauncher;
    TestManagerComponent testManagerComponent;
    NewTestSelectComponent newTestSelectComponent;
    DemoComponent demoComponent;
    MushraComponent mushraComponent;
    BS1116Component bs1116Component;
    ABTestComponent abComponent;
    AVABTestComponent avabComponent;
    BaseTestComponent *testComponent; /* This will hold the GUI for whichever test we run */
    GroupComponent testBorder;
    GroupComponent playbackBorder;
    Slider playbackSlider;

    TextButton playButton;
    TextButton rewindButton;
    TextButton backButton;
    TextButton forwardButton;

    ToggleButton loopToggleButton;
    ToggleButton lockLoopToggleButton;
    TextButton newTestButton;
    TextButton audioSetupButton;
    TextButton manageTestsButton;
    TextButton loadTestButton;
    TextButton showHotkeysButton;

    Label positionLabel;
    Label hotkeyTextLabel;
                                                    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent);
};


#endif   // MAIN_COMPONENT_H
