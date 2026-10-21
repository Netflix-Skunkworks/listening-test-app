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
        testFinished(false),
        resultSavePending(false),
        continueAfterSave(false) {
    addAndMakeVisible(&referenceButton);
    referenceButton.setButtonText("Reference");
    referenceButton.setColour(TextButton::buttonColourId, referenceButtonColour);
    referenceButton.setColour(TextButton::buttonOnColourId, activeButtonColour);
    referenceButton.setClickingTogglesState(true);
    referenceButton.addShortcut(KeyPress::createFromDescription("`"));
   #if JUCE_WINDOWS
    // Shortcuts are matched with GetAsyncKeyState, which takes a virtual-key
    // code. The character '`' is not that code; the key left of 1 is VK_OEM_3
    // (0xC0). JUCE translates '-' and '=', but not backtick.
    referenceButton.addShortcut(KeyPress(0xC0));
   #endif
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
            testLauncher.incrementPlayCount(i);
            testLauncher.dbgOut("playing stimulus " + String(i) + " \t" +
                                testLauncher.getCurrentTrial()->soundFiles[testLauncher.getCurrentTrial()->filesOrder[i]]);

        } else {
            ratingSliders[i]->setEnabled(false);
            stimuliButtons[i]->setToggleState(false, dontSendNotification);
        }
    }
}

void BaseTestComponent::selectDefaultStimulus() {
    const auto testType = testLauncher.getTestType();
    if (testType == TEST_TYPE_AB || testType == TEST_TYPE_AVAB)
        return;

    currentButton = 0;
    selectStimulus(-1);
    for (auto* slider : ratingSliders) {
        slider->setAlpha(SLIDER_DISABLED_ALPHA);
        slider->setTextBoxIsEditable(false);
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
        if (resultSavePending || saveResponses()) {
            handleChangeTrialRequest(testLauncher.getCurrentTrialIndex() + 1);
        }

        if (resultSavePending) {
            return;
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
    bool shouldContinue = continueAfterSave;
    if (!resultSavePending) {
        /* deselect all stimuli and stop current playback */
        selectStimulus(-1);
        audioPlayer.stop();
        testLauncher.getCurrentTrial()->setStopTime();

        /* Load audio data and go to the next test. A failed save must retry
           this same transition, rather than advancing another trial. */
        shouldContinue = testLauncher.goToTrial(newTrialIndex);
        if (testLauncher.lastError.isNotEmpty()) {
            AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error", testLauncher.lastError, "OK", this);
            return;
        }
    }

    /* save results */
    if (!testLauncher.saveResults()) {
        resultSavePending = true;
        continueAfterSave = shouldContinue;
        nextButton.setButtonText("Retry Save");
        AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Could not save results",
                                         testLauncher.lastError + "\n\nPress Retry Save to try again.");
        return;
    }
    resultSavePending = false;
    if (testLauncher.getTestType() == TEST_TYPE_MUSHRA_DEMO)
        nextButton.setButtonText("Next >");

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
    // Restarting the audio device clears its selection, so select after start().
    if (shouldContinue)
        selectDefaultStimulus();
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
