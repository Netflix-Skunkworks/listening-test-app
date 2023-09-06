//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#include "BS1116Component.h"
#include "CommentBox.h"

//==============================================================================
BS1116Component::BS1116Component(AudioPlayer &aP, TestLauncher &tL) : BaseTestComponent(aP, tL) {
    addAndMakeVisible(&nextButton);
    nextButton.setButtonText("Next >");
    nextButton.addListener(this);
    nextButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&headerLabel);
    headerLabel.setText("Impairment", dontSendNotification);
    headerLabel.setJustificationType(Justification::topLeft);
    headerLabel.setEditable(false, false, false);
    headerLabel.setColour(TextEditor::textColourId, LINE_COLOUR);

    /* create labels & lines */
    const StringArray labelsText = {"Imperceptible", "Perceptible, but not annoying", "Slightly Annoying", "Annoying",
                                    "Very Annoying"};
    for (int i = 0; i < labelsText.size(); i++) {
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

    stimCount = 2;
}

void BS1116Component::resized() {
    nextButton.setBounds(getWidth() - BUTTON_W, getHeight() - BUTTON_H, BUTTON_W, BUTTON_H);
    if (testLauncher.getCurrentTrialIndex() == testLauncher.getTrialsCount() - 1) {
        /* update button text if the last test */
        nextButton.setButtonText("Finish Test");
    } else {
        nextButton.setButtonText("Next >");
    }

    int margin = nextButton.getX() - BUTTON_H;

    /* Redraw reference button, stimulus buttons & sliders */
    referenceButton.setBounds(0, 0, margin, BUTTON_H);

    int buttonWidth = (int) ceil(
            (float) referenceButton.getWidth() / ((stimuliButtons.size() - 1) / REL_BUTTON_SPACING + 1));
    int spacing = (int) (buttonWidth / REL_BUTTON_SPACING);
    assert(stimuliButtons.size() == ratingSliders.size());
    for (int i = 0; i < stimuliButtons.size(); i++) {
        stimuliButtons[i]->setBounds(spacing * i,
                                     referenceButton.getBottom() + 20,
                                     buttonWidth,
                                     BUTTON_H);
        
        ratingSliders[i]->setBounds(stimuliButtons[i]->getX(),
                                    stimuliButtons[i]->getBottom() + 20,
                                    stimuliButtons[i]->getWidth(),
                                    nextButton.getY() - (stimuliButtons[i]->getBottom() + 20));
        ratingSliders[i]->setRange(1.0, 5.0, 0.1);

        commentBoxes[i]->setBounds(ratingSliders[i]->getX(),
                                   nextButton.getY()+8,
                                   ratingSliders[i]->getWidth(),
                                   nextButton.getHeight()-8);
    }

    /* Draw ratings lines across sliders, and ratings labels */
    for (int i = 0; i < ratingsDividers.size(); i++) {
        int lineY = ratingSliders[0]->getY() + ratingSliders[0]->getPositionOfValue(5.0 - i);
        ratingsDividers[i]->clear();
        ratingsDividers[i]->startNewSubPath(ratingSliders[0]->getX(), lineY);
        ratingsDividers[i]->lineTo(margin, lineY);
        ratingsDividers[i]->closeSubPath();
        ratingLabels[i]->setJustificationType(Justification::centredLeft);
        ratingLabels[i]->setBounds(margin,
                                   lineY - BUTTON_H / 2,
                                   getWidth() - margin,
                                   BUTTON_H);
    }

    headerLabel.setFont(Font(18, Font::bold));
    headerLabel.setBounds(margin, stimuliButtons[0]->getY(), getWidth() - margin, stimuliButtons[0]->getHeight());
    headerLabel.setJustificationType(Justification::centredLeft);

    BorderSize<int> headerLabelBorder = headerLabel.getBorderSize();
    headerLine.clear();
    headerLine.startNewSubPath(headerLabelBorder.getLeft(), headerLabelBorder.getBottom());
    headerLine.lineTo(headerLabelBorder.getRight(), headerLabelBorder.getBottom());
}

void BS1116Component::buttonClicked(Button *b) {
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


bool BS1116Component::saveResponses() {
    int count5 = 0;
    Trial *pTrial = testLauncher.getCurrentTrial();
    assert(pTrial->stimuliPlays.size() == ratingSliders.size());
    assert(pTrial->stimuliPlays.size() == pTrial->responses.size());
    for (int i = 0; i < pTrial->stimuliPlays.size(); i++) {
        pTrial->stimuliPlays.set(i, testLauncher.getPlayCount(i));
        pTrial->responses.set(i, ratingSliders[i]->getValue());
        if (ratingSliders[i]->getValue() == 5.0)
            count5++;
    }

    for (int i = 0; i < pTrial->comments.size(); i++) {
        if (!commentBoxes[i]->isEmpty()) {
            pTrial->comments.set(i, commentBoxes[i]->getText());
        }
    }

    if (count5 != 1) {
        AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Trial Incomplete",
                                         "Cannot proceed.  One stimulus must be rated 5.0 and one must be rated otherwise");
    }
    return (count5 == 1);
}

void BS1116Component::clearResponses() {
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

bool BS1116Component::readResponses() {
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

void BS1116Component::createButtons() {
    /* create buttons and sliders */
    stimuliButtons.clear();
    ratingSliders.clear();
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
}
