//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "MushraComponent.h"
#include "CommentBox.h"

//==============================================================================
MushraComponent::MushraComponent(AudioPlayer &aP, TestLauncher &tL) : BaseTestComponent(aP, tL) {
    addAndMakeVisible(&nextButton);
    nextButton.setButtonText("Next >");
    nextButton.addListener(this);
    nextButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&headerLabel);
    headerLabel.setText("Quality of each\nstimulus compared\nto the Reference", dontSendNotification);
    headerLabel.setJustificationType(Justification::topLeft);
    headerLabel.setEditable(false, false, false);
    headerLabel.setColour(TextEditor::textColourId, LINE_COLOUR);

    /* create labels & lines */
    const StringArray labelsText = {"Excellent", "Good", "Fair", "Poor", "Bad"};
    for (int i = 0; i < labelsText.size() - 1; i++) {
        ratingsDividers.add(new Path);
    }
    for (int i = 0; i < labelsText.size(); i++) {
        ratingLabels.add(new Label("label" + labelsText[i], labelsText[i]));
        ratingLabels[i]->setFont(Font(15, Font::plain));
        ratingLabels[i]->setJustificationType(Justification::centredLeft);
        ratingLabels[i]->setEditable(false, false, false);
        ratingLabels[i]->setColour(TextEditor::textColourId, Colours::black);
        ratingLabels[i]->setColour(TextEditor::backgroundColourId, Colours::black);
        ratingLabels[i]->setAlpha(1.0);
        addAndMakeVisible(ratingLabels[i]);
    }
}

void MushraComponent::resized() {
    float wscale = getWidth() / (float) DEFAULT_WIDTH;

    nextButton.setBounds(getWidth() - BUTTON_W * wscale, getHeight() - BUTTON_H, BUTTON_W * wscale, BUTTON_H);
    if (testLauncher.getCurrentTrialIndex() == testLauncher.getTrialsCount() - 1) {
        /* update button text if the last test */
        nextButton.setButtonText("Finish Test");
    } else {
        nextButton.setButtonText("Next >");
    }

    int margin = nextButton.getX() - nextButton.getWidth() / 2;

    /* Redraw reference button, stimulus buttons & sliders */
    referenceButton.setBounds(0, 0, margin - 8, BUTTON_H);

    int buttonWidth = (int) ceil(
            (float) referenceButton.getWidth() / ((stimuliButtons.size() - 1) / REL_BUTTON_SPACING + 1));
    int spacing = (int) (buttonWidth / REL_BUTTON_SPACING);
    assert(stimuliButtons.size() == ratingSliders.size());
    for (int i = 0; i < stimuliButtons.size(); i++) {
        stimuliButtons[i]->setBounds(spacing * i,
                                     referenceButton.getBottom() + 20,
                                     buttonWidth,
                                     BUTTON_H);
    }

    for (int i = 0; i < stimuliButtons.size(); i++) {
        ratingSliders[i]->setBounds(stimuliButtons[i]->getX(),
                                    stimuliButtons[i]->getBottom() + 20,
                                    stimuliButtons[i]->getWidth(),
                                    nextButton.getY() - (stimuliButtons[0]->getBottom() + 20));
    }

    // Draw comment boxes
    for (int i = 0; i < commentBoxes.size(); i++) {
        commentBoxes[i]->setBounds(ratingSliders[i]->getX(),
                                   nextButton.getY()+8,
                                   ratingSliders[i]->getWidth(),
                                   nextButton.getHeight()-8);
    }

    /* Draw ratings lines across sliders */
    for (int i = 0; i < ratingsDividers.size(); i++) {
        int lineY = ratingSliders[0]->getY() + ratingSliders[0]->getPositionOfValue(20 * (i + 1));
        ratingsDividers[i]->clear();
        ratingsDividers[i]->startNewSubPath(ratingSliders[0]->getX(), lineY);
        ratingsDividers[i]->lineTo(margin - 8, lineY);
        ratingsDividers[i]->closeSubPath();
    }

    /* Draw ratings labels */
    int labelHeight = abs(ratingSliders[0]->getPositionOfValue(ratingSliders[0]->getMaximum()) -
                          ratingSliders[0]->getPositionOfValue(80));
    for (int i = 0; i < ratingLabels.size(); i++) {
        int labelY = ratingSliders[0]->getY() + ratingSliders[0]->getPositionOfValue(100 - 20 * i);

        ratingLabels[i]->setJustificationType(Justification::centredLeft);
        ratingLabels[i]->setBounds(margin,
                                   labelY,
                                   getWidth() - margin,
                                   labelHeight);
    }

    float hscale = getHeight() / (float) DEFAULT_HEIGHT;
    headerLabel.setFont(Font(18 * min(wscale, hscale), Font::bold));
    headerLabel.setBounds(margin,
                          stimuliButtons[0]->getY(),
                          getWidth() - margin,
                          ratingLabels[0]->getY() - stimuliButtons[0]->getY());

    BorderSize<int> headerLabelBorder = headerLabel.getBorderSize();
    headerLine.clear();
    headerLine.startNewSubPath(headerLabelBorder.getLeft(), headerLabelBorder.getBottom());
    headerLine.lineTo(headerLabelBorder.getRight(), headerLabelBorder.getBottom());
}

void MushraComponent::buttonClicked(Button *b) {
    for (int i = 0; i < stimuliButtons.size(); i++) {
        ratingSliders[i]->setAlpha(SLIDER_DISABLED_ALPHA);
        ratingSliders[i]->setTextBoxIsEditable(false);

        if (b == stimuliButtons[i]) {
            ratingSliders[i]->setAlpha(1.0);
            ratingSliders[i]->setTextBoxIsEditable(true);
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


bool MushraComponent::saveResponses() {
    int count100 = 0;
    Trial *pTrial = testLauncher.getCurrentTrial();
    assert(pTrial->stimuliPlays.size() == ratingSliders.size());
    assert(pTrial->stimuliPlays.size() == pTrial->responses.size());
    for (int i = 0; i < pTrial->stimuliPlays.size(); i++) {
        pTrial->stimuliPlays.set(i, testLauncher.getPlayCount(i));
        pTrial->responses.set(i, int(ratingSliders[i]->getValue()));
        if (ratingSliders[i]->getValue() == 100)
            count100++;
    }
    
    for (int i = 0; i < pTrial->comments.size(); i++) {
        if (!commentBoxes[i]->isEmpty()) {
            pTrial->comments.set(i, commentBoxes[i]->getText());
        }
    }
    
    if (count100 == 0) {
        AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Trial Incomplete",
                                         "None of the stimuli were rated 100.  There is a hidden reference; at least one stimulus must be scored 100.");
        return false;
    } else if (count100 > 1 && testLauncher.getTestType() == TEST_TYPE_MUSHRA_STRICT) {
        AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Trial Incomplete",
                                         "Please rate all stimuli before proceeding.  Only one stimulus can be scored 100.");
        return false;
    }
    
    bool allMatch = true;
    for (int i = 1; i < pTrial->responses.size(); i++) {
        if (pTrial->responses[i] != pTrial->responses[0]) {
            allMatch = false;
        }
    }
            
    if (allMatch) {
        return NativeMessageBox::showOkCancelBox(MessageBoxIconType::WarningIcon,
                                          "All responses match",
                                          "All of the responses are the same score!  Are you sure you want to proceed?");
            
    }
    
    return true;
}

void MushraComponent::clearResponses() {
    Trial *pTrial = testLauncher.getCurrentTrial();

    if (pTrial != NULL) {
        for (int i = 0; i < ratingSliders.size(); i++) {
            ratingSliders[i]->setValue(pTrial->responses[i], dontSendNotification);
            ratingSliders[i]->setColour(Slider::ColourIds::thumbColourId, Colours::grey);
            ratingSliders[i]->setAlpha(SLIDER_DISABLED_ALPHA);

            pTrial->responses.set(i, 100);
            pTrial->responsesMoved.set(i, false);
            
            pTrial->comments[i].clear();
            commentBoxes[i]->clear();
        }
    }
}

bool MushraComponent::readResponses() {
    Trial *pTrial = testLauncher.getCurrentTrial();
    if (pTrial != NULL) {
        for (int i = 0; i < ratingSliders.size(); i++) {
            ratingSliders[i]->setValue(pTrial->responses[i], dontSendNotification);
            ratingSliders[i]->setAlpha(SLIDER_DISABLED_ALPHA);
            
            if (pTrial->comments[i].isNotEmpty()) {
                commentBoxes[i]->setText(pTrial->comments[i]);
            }
        }

        return true;
    } else {
        return false;
    }
}

void MushraComponent::createButtons() {
    stimCount = testLauncher.getStimCount();

    /* change the button text if only one trial */
    if (testLauncher.getTrialsCount() == 1) {
        nextButton.setButtonText("done");
    }

    /* create buttons and sliders */
    stimuliButtons.clear();
    ratingSliders.clear();
    commentBoxes.clear();
    
    for (int i = 0; i < stimCount; i++) {
        String letterStr = String(i + 1);
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

        ratingSliders.add(new Slider("slider" + letterStr));
        addAndMakeVisible(ratingSliders[i]);
        ratingSliders[i]->setRange(0, 100, 1);
        ratingSliders[i]->setValue(100);
        ratingSliders[i]->setSliderStyle(Slider::LinearVertical);
        ratingSliders[i]->setTextBoxStyle(Slider::TextBoxBelow, false, 80, 20);
        ratingSliders[i]->setEnabled(false);
        ratingSliders[i]->setAlpha(SLIDER_DISABLED_ALPHA);
        ratingSliders[i]->setTextBoxIsEditable(false);
        ratingSliders[i]->addListener(this);
        
        commentBoxes.add(new CommentBox("comment" + letterStr));
        addAndMakeVisible(commentBoxes[i]);
    }

    resized();
}
