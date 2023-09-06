//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#ifndef BASE_TEST_COMPONENT_H
#define BASE_TEST_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "TestLauncher.h"
#include "guisettings.h"
#include "CommentBox.h"

class BaseTestComponent : public Component,
                          public Slider::Listener,
                          public Button::Listener,
                          public ChangeBroadcaster {
public:
    BaseTestComponent(AudioPlayer &aP, TestLauncher &tL);

    ~BaseTestComponent();

    bool getStartPlayRequest() { return startPlayback; }

    bool getToggleRequest() { return togglePlayback; }

    bool getResetRequest() { return resetPlayback; }

    bool getTestFinished() { return testFinished; }

    int getCurrentButton() { return currentButton; }

    void clearStartPlayRequest() { startPlayback = false; }

    void clearToggleRequest() { togglePlayback = false; }

    void clearResetRequest() { resetPlayback = false; }

    void paint(Graphics &g);

    void sliderValueChanged(Slider *s);

    bool keyPressed(const KeyPress &k);

    void buttonClicked(Button *b);

    String getTitleText();

    virtual void createButtons() = 0;

    virtual void resized() = 0;

private:
    virtual void clearResponses() = 0;

    virtual bool saveResponses() = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseTestComponent);

protected:
    void selectStimulus(int i);

    void handleChangeTrialRequest(int newTrialIndex);

    int stimCount;
    int currentButton;

    AudioPlayer &audioPlayer;
    TestLauncher &testLauncher;

    /* Upper-right-hand header (summary test info) */
    Label headerLabel;
    Path headerLine;

    TextButton nextButton;
    TextButton referenceButton;
    OwnedArray <TextButton> stimuliButtons;
    OwnedArray <Slider> ratingSliders;
    OwnedArray <Path> ratingsDividers;
    OwnedArray <CommentBox> commentBoxes;

    bool startPlayback;
    bool togglePlayback;
    bool resetPlayback;
    bool testFinished;

    const String stimButtonShortcuts[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="};

};

#endif /* BASE_TEST_COMPONENT_H */
