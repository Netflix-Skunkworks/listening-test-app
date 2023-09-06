//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#include "ABTestComponent.h"
#include "CommentBox.h"


ABTestComponent::ABTestComponent(AudioPlayer &aP, TestLauncher &tL) : BaseTestComponent(aP, tL) {
    addAndMakeVisible(&nextButton);
    nextButton.setButtonText("Next >");
    nextButton.addListener(this);
    nextButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    initLabel(headerLabel, String("Use the slider\nto choose your\npreferred stimulus."), Justification::topLeft);
    initLabel(buttonLabel, String("Use the buttons\nto select which\nstimulus to play."), Justification::topLeft);
    initLabel(yourSelectionLabel, String("Your selection:"), Justification::topLeft);
    initLabel(preferenceLabel, "", Justification::centred);
    initLabel(sliderLeftLabel, String("A"), Justification::centredRight);
    initLabel(sliderRightLabel, String("B"), Justification::centredLeft);
    initLabel(commentsLabel, String("Notes\n(optional)"), Justification::topLeft);

    stimCount = 2;

}

void ABTestComponent::resized() {
    nextButton.setBounds(getWidth() - BUTTON_W, getHeight() - BUTTON_H, BUTTON_W, BUTTON_H);
    if (testLauncher.getCurrentTrialIndex() == testLauncher.getTrialsCount() - 1) {
        /* update button text if the last test */
        nextButton.setButtonText("Finish Test");
    } else {
        nextButton.setButtonText("Next >");
    }
    
    buttonLabel.setFont(Font(14, Font::bold));
    setupLabel(buttonLabel, 0, 0);
    
    commentsLabel.setFont(Font(14, Font::bold));
    setupLabel(commentsLabel, 0, buttonLabel.getBottom() + Y_SPACING);
    
    headerLabel.setFont(Font(14, Font::bold));
    setupLabel(headerLabel, 0, commentsLabel.getBottom() + Y_SPACING);
    
    yourSelectionLabel.setFont(Font(14, Font::bold));
    setupLabel(yourSelectionLabel, 0, (headerLabel.getBottom() + getHeight())/2);
    
    int labelMargin = max(max(buttonLabel.getRight(), headerLabel.getRight()), yourSelectionLabel.getRight()) + X_SPACING;

    // Buttons centered, if possible, otherwise spaced from label margin to the right edge
    int width = min(160, (getWidth() - labelMargin) * 2/5);
    stimuliButtons[0]->setBounds(max(getWidth()/2 - width * 5/4, labelMargin),
                                 0,
                                 width,
                                 buttonLabel.getHeight());
    stimuliButtons[1]->setBounds(stimuliButtons[0]->getRight() + width/2,
                                 stimuliButtons[0]->getY(),
                                 stimuliButtons[0]->getWidth(),
                                 stimuliButtons[0]->getHeight());

    // Draw comment boxes
    commentBoxes[0]->setBounds(stimuliButtons[0]->getX(),
                               commentsLabel.getY(),
                               stimuliButtons[0]->getWidth(),
                               commentsLabel.getHeight());
    commentBoxes[1]->setBounds(stimuliButtons[1]->getX(),
                               commentsLabel.getY(),
                               stimuliButtons[1]->getWidth(),
                               commentsLabel.getHeight());

    // Preference slider spanning to center of each button
    preferenceSlider.setBounds(stimuliButtons[0]->getX()+width/2,
                               headerLabel.getY(),
                               width*3/2,
                               BUTTON_H);

    sliderLeftLabel.setFont(Font(12, Font::bold));
    sliderRightLabel.setFont(Font(12, Font::bold));

    // labels on either end of the slider
    width = 16;
    sliderLeftLabel.setBounds(preferenceSlider.getX() - width,
                              preferenceSlider.getY(),
                              width,
                              preferenceSlider.getHeight());

    sliderRightLabel.setBounds(preferenceSlider.getRight(),
                               preferenceSlider.getY(),
                               width,
                               preferenceSlider.getHeight());
    
    // giant label indicating subject's current choice
    preferenceLabel.setFont(Font(240, Font::bold));
    preferenceLabel.setBounds(preferenceSlider.getX(),
                              preferenceSlider.getBottom() + Y_SPACING,
                              preferenceSlider.getWidth(),
                              getHeight()-preferenceSlider.getBottom()-Y_SPACING);
    
}

void ABTestComponent::buttonClicked(Button *b) {
    for (int i = 0; i < stimuliButtons.size(); i++) {
        if (b == stimuliButtons[i]) {
            if (currentButton == i + 1) {
                togglePlayback = true;
                sendChangeMessage();
            } else {
                startPlayback = true;
                sendChangeMessage();
                currentButton = i + 1;
                selectStimulus(i);
            }
        }
    }
    BaseTestComponent::buttonClicked(b);
}

void ABTestComponent::sliderValueChanged(Slider *s) {
    if (s == &preferenceSlider) {
        if (preferenceSlider.getValue() == 0) {
            preferenceLabel.setText("A", dontSendNotification);
            preferenceSlider.setRange(0, 1, 1);
        } else if (preferenceSlider.getValue() == 1) {
            preferenceLabel.setText("B", dontSendNotification);
            preferenceSlider.setRange(0, 1, 1);
        }

        Trial *pTrial = testLauncher.getCurrentTrial();
        if (pTrial != NULL) {
            pTrial->responsesMoved.set(0, true);
            pTrial->responsesMoved.set(1, true);
        }
    } else {
        return BaseTestComponent::sliderValueChanged(s);
    }
}

bool ABTestComponent::keyPressed(const KeyPress &key) {
    if (key == KeyPress::upKey ||
        key == KeyPress::downKey ||
        key == KeyPress::pageUpKey ||
        key == KeyPress::pageDownKey) {
        // do nothing.  these cases added to prevent base class from attempting to modify sliders that
        // have not been allocated for this subclass.
    } else {
        return BaseTestComponent::keyPressed(key);
    }

    return true;

}

bool ABTestComponent::saveResponses() {
    if ((preferenceSlider.getValue() != 0) && (preferenceSlider.getValue() != 1)) {

        AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Trial Incomplete",
                                         "Please choose your preference -- A or B.");
        return false;
    }

    Trial *pTrial = testLauncher.getCurrentTrial();
    pTrial->responses.set(0, 0);
    pTrial->responses.set(1, 0);
    pTrial->responses.set(preferenceSlider.getValue(), 1);
    pTrial->stimuliPlays.set(0, testLauncher.getPlayCount(0));
    pTrial->stimuliPlays.set(1, testLauncher.getPlayCount(1));

    for (int i = 0; i < pTrial->comments.size(); i++) {
        if (!commentBoxes[i]->isEmpty()) {
            pTrial->comments.set(i, commentBoxes[i]->getText());
        }
    }

    return true;
}

void ABTestComponent::clearResponses() {
    Trial *pTrial = testLauncher.getCurrentTrial();

    if (pTrial != NULL) {
        pTrial->responses.set(0, 0);
        pTrial->responses.set(1, 0);
        pTrial->responsesMoved.set(0, false);
        pTrial->responsesMoved.set(1, false);
    }

    pTrial->comments[0].clear();
    pTrial->comments[1].clear();
    commentBoxes[0]->clear();
    commentBoxes[1]->clear();

    preferenceSlider.setRange(0, 1, 0.5);
    preferenceSlider.setValue(0.5);
    preferenceLabel.setText("", dontSendNotification);
}

bool ABTestComponent::readResponses() {
    Trial *pTrial = testLauncher.getCurrentTrial();
    if (pTrial != NULL) {
        if (pTrial->responses[0] == 1) {
            preferenceSlider.setValue(0);
        } else if (pTrial->responses[1] == 1) {
            preferenceSlider.setValue(1);
        }
        
        if (pTrial->comments[0].isNotEmpty()) {
            commentBoxes[0]->setText(pTrial->comments[0]);
        }
        if (pTrial->comments[1].isNotEmpty()) {
            commentBoxes[1]->setText(pTrial->comments[1]);
        }

        return true;
    }

    return false;
}

void ABTestComponent::createButtons() {
    /* create buttons and sliders */
    stimuliButtons.clear();
    commentBoxes.clear();

    for (int i = 0; i < stimCount; i++) {
        String letterStr = String::charToString('A' + i);
        stimuliButtons.add(new TextButton("button" + letterStr));
        addAndMakeVisible(stimuliButtons[i]);
        stimuliButtons[i]->setButtonText(letterStr);
        stimuliButtons[i]->setColour(TextButton::buttonColourId, stimuliButtonColour);
        stimuliButtons[i]->setColour(TextButton::buttonOnColourId, activeButtonColour);
        stimuliButtons[i]->addListener(this);
        stimuliButtons[i]->setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
        if (i < 12) {
            stimuliButtons[i]->addShortcut(KeyPress::createFromDescription(stimButtonShortcuts[i]));
        }

        commentBoxes.add(new CommentBox("comment" + letterStr));
        addAndMakeVisible(commentBoxes[i]);

        // This component does not use typical sliders but we will add the objects to overcome assumptions in the base class
        ratingSliders.add(new Slider("slider" + letterStr));
    }

    preferenceSlider.setSliderStyle(Slider::LinearHorizontal);
    addAndMakeVisible(preferenceSlider);
    preferenceSlider.setRange(0, 1, 0.5);
    preferenceSlider.setValue(0.5);
    preferenceSlider.setTextBoxStyle(Slider::NoTextBox, false, 80, 20);
    preferenceSlider.setEnabled(true);
    preferenceSlider.setAlpha(1.0f);
    preferenceSlider.addListener(this);
}
