//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#include "BaseTestComponent.h"

BaseTestComponent::BaseTestComponent(AudioPlayer &aP, TestLauncher &tL) :
        currentButton(-1),
        audioPlayer(aP),
        testLauncher(tL),
        startPlayback(false),
        togglePlayback(false),
        resetPlayback(false),
        testFinished(false) {
    addAndMakeVisible(&referenceButton);
    referenceButton.setButtonText("Reference");
    referenceButton.setColour(TextButton::buttonColourId, referenceButtonColour);
    referenceButton.setColour(TextButton::buttonOnColourId, activeButtonColour);
    referenceButton.setClickingTogglesState(true);
    referenceButton.addShortcut(KeyPress::createFromDescription("`"));
    referenceButton.addListener(this);
    referenceButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
}

BaseTestComponent::~BaseTestComponent() {
    removeAllChangeListeners();
}

//==============================================================================
void BaseTestComponent::paint(Graphics &g) {
    g.fillAll(Colours::darkgrey);

    Path *tmpPath;
    for (int i = 0; i < ratingsDividers.size(); i++) {
        tmpPath = ratingsDividers[i];
        g.setColour(LINE_BG_COLOUR);
        g.fillPath(*tmpPath);
        g.setColour(LINE_COLOUR);
        g.strokePath(*tmpPath, PathStrokeType(2));
    }

    // Draw header line
    g.setColour(Colour(0x0));
    g.fillPath(headerLine);
    g.setColour(Colours::black);
    g.strokePath(headerLine, PathStrokeType(1.5f));
}

void BaseTestComponent::sliderValueChanged(Slider *s) {
    for (int i = 0; i < ratingSliders.size(); i++) {
        if (s == ratingSliders[i]) {
            ratingSliders[i]->setAlpha(1.0);
            ratingSliders[i]->setColour(Slider::ColourIds::thumbColourId, Colours::grey);
            Trial *pTrial = testLauncher.getCurrentTrial();
            if (pTrial != NULL) {
                pTrial->responsesMoved.set(i, true);
            }
        }
    }
}

void BaseTestComponent::selectStimulus(int ind) {
    if (ind == -1) {
        int refIndex = testLauncher.getCurrentTrial()->refIndex;
        referenceButton.setToggleState(true, dontSendNotification);
        audioPlayer.switchStimulus(refIndex);
        testLauncher.dbgOut("playing reference\t" + testLauncher.getCurrentTrial()->soundFiles[refIndex]);

    } else {
        referenceButton.setToggleState(false, dontSendNotification);
    }


    for (int i = 0; i < stimuliButtons.size(); i++) {
        if (ind == i) {
            ratingSliders[ind]->setEnabled(true);
            stimuliButtons[ind]->setToggleState(true, dontSendNotification);
            audioPlayer.switchStimulus(testLauncher.getCurrentTrial()->filesOrder[i]);
            testLauncher.dbgOut("playing stimulus " + String(i) + " \t" +
                                testLauncher.getCurrentTrial()->soundFiles[testLauncher.getCurrentTrial()->filesOrder[i]]);

        } else {
            ratingSliders[i]->setEnabled(false);
            stimuliButtons[i]->setToggleState(false, dontSendNotification);
        }
    }
}

String BaseTestComponent::getTitleText() {
    return "Evaluating " + testLauncher.getCurrentTrial()->testName;
}

void BaseTestComponent::buttonClicked(Button *b) {
    b->toFront(true);
    if (b == &referenceButton) {
        if (currentButton == 0) {
            togglePlayback = true;
            sendChangeMessage();
        } else {
            currentButton = 0;
            selectStimulus(-1);
            testLauncher.incrementReferencePlayCount();

            startPlayback = true;
            sendChangeMessage();

            for (int i = 0; i < ratingSliders.size(); i++) {
                ratingSliders[i]->setAlpha(SLIDER_DISABLED_ALPHA);
                ratingSliders[i]->setTextBoxIsEditable(false);
            }
        }
    } else if (b == &nextButton) {
        audioPlayer.pause();
        if (saveResponses()) {
            handleChangeTrialRequest(testLauncher.getCurrentTrialIndex() + 1);
        }

        if (testLauncher.getCurrentTrialIndex() == testLauncher.getTrialsCount() - 1) {
            /* update button text if the last test */
            nextButton.setButtonText("Finish Test");
        } else {
            nextButton.setButtonText("Next >");
        }
    }
}


void BaseTestComponent::handleChangeTrialRequest(int newTrialIndex) {
    /* deselect all stimuli */
    selectStimulus(-1);

    /* stop current playback */
    audioPlayer.stop();
    testLauncher.getCurrentTrial()->setStopTime();

    /* Load audio data and go to the next test */
    bool shouldContinue = testLauncher.goToTrial(newTrialIndex);
    if (testLauncher.lastError.isNotEmpty()) {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error", testLauncher.lastError, "OK", this);
        return;
    }

    /* save results */
    testLauncher.saveResults();

    /* save results if the session is complete */
    if (!shouldContinue) {
        AlertWindow::showMessageBox(MessageBoxIconType::InfoIcon, "Your results have been recorded.",
                                    "Thanks for listening today.");
        testFinished = true;
        sendChangeMessage();
    }

    /* reset signal to parent */
    resetPlayback = true;
    currentButton = -1;
    audioPlayer.start();

    /* set the sliders to default */
    clearResponses();
    sendChangeMessage();
}

bool BaseTestComponent::keyPressed(const KeyPress &key) {
    if (currentButton > 0) {
        if (key == KeyPress::upKey) {
            ratingSliders[currentButton - 1]->setValue(ratingSliders[currentButton - 1]->getValue() +
                                                       ratingSliders[currentButton - 1]->getInterval());
        } else if (key == KeyPress::downKey) {
            ratingSliders[currentButton - 1]->setValue(
                    ratingSliders[currentButton - 1]->getValue() - ratingSliders[currentButton - 1]->getInterval());
        } else if (key == KeyPress::pageUpKey) {
            ratingSliders[currentButton - 1]->setValue(ratingSliders[currentButton - 1]->getValue() +
                                                       ratingSliders[currentButton - 1]->getMaximum() / 5.0f);
        } else if (key == KeyPress::pageDownKey) {
            ratingSliders[currentButton - 1]->setValue(ratingSliders[currentButton - 1]->getValue() -
                                                       ratingSliders[currentButton - 1]->getMaximum() / 5.0f);
        }
    }
    return true;
}
