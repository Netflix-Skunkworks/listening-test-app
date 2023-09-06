//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "DemoComponent.h"

//==============================================================================
DemoComponent::DemoComponent(AudioPlayer &aP, TestLauncher &tL) : BaseTestComponent(aP, tL),
                                                                  tooltipWindow() {
    addAndMakeVisible(&prevButton);
    prevButton.setButtonText("< Prev");
    prevButton.addListener(this);
    prevButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&nextButton);
    nextButton.setButtonText("Next >");
    nextButton.addListener(this);
    nextButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&headerLabel);
    headerLabel.setText("Quality of each\nstimulus compared\nto the Reference", dontSendNotification);
    headerLabel.setJustificationType(Justification::centredLeft);
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

void DemoComponent::resized() {
    float wscale = getWidth() / (float) DEFAULT_WIDTH;
    float hscale = getHeight() / (float) DEFAULT_HEIGHT;

    nextButton.setBounds(getWidth() - BUTTON_W * wscale, getHeight() - BUTTON_H, BUTTON_W * wscale, BUTTON_H);
    prevButton.setBounds(nextButton.getX() - BUTTON_W * wscale - 8, getHeight() - BUTTON_H, BUTTON_W * wscale, BUTTON_H);

    headerLabel.setFont(Font(18 * wscale, Font::plain));
    headerLabel.setBounds(prevButton.getX(), BUTTON_Y * hscale, nextButton.getWidth() * 2 + 8,
                          nextButton.getY() - BUTTON_Y * hscale);
    headerLabel.setJustificationType(Justification::topLeft);

    BorderSize<int> headerLabelBorder = headerLabel.getBorderSize();
    headerLine.clear();
    headerLine.startNewSubPath(headerLabelBorder.getLeft(), headerLabelBorder.getBottom());
    headerLine.lineTo(headerLabelBorder.getRight(), headerLabelBorder.getBottom());

    int margin = prevButton.getX() - prevButton.getWidth() / 2;


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
        ratingSliders[i]->setBounds(spacing * i,
                                    stimuliButtons[i]->getBottom() + 20,
                                    stimuliButtons[i]->getWidth(),
                                    nextButton.getY() - (stimuliButtons[i]->getBottom() + 20));
    }

    /* Draw ratings lines across sliders */
    for (int i = 0; i < ratingsDividers.size(); i++) {
        int lineY = ratingSliders[0]->getY() + ratingSliders[0]->getPositionOfValue(20 * (i + 1));
        ratingsDividers[i]->clear();
        ratingsDividers[i]->startNewSubPath(ratingSliders[0]->getX(), lineY);
        ratingsDividers[i]->lineTo(stimuliButtons[stimuliButtons.size() - 1]->getRight(), lineY);
        ratingsDividers[i]->closeSubPath();
    }

    /* Draw ratings labels */
    int labelHeight = abs(ratingSliders[0]->getPositionOfValue(100) - ratingSliders[0]->getPositionOfValue(80));
    for (int i = 0; i < ratingLabels.size(); i++) {
        int labelY = ratingSliders[0]->getY() + ratingSliders[0]->getPositionOfValue(100 - 20 * i);
        ratingLabels[i]->setBounds(prevButton.getX(),
                                   labelY,
                                   getWidth() - prevButton.getY(),
                                   labelHeight);
    }

}

void DemoComponent::buttonClicked(Button *b) {
    if (b == &nextButton) {
        audioPlayer.pause();
        if (testLauncher.getCurrentTrialIndex() != testLauncher.getTrialsCount() - 1) {
            saveResponses();
            handleChangeTrialRequest(testLauncher.getCurrentTrialIndex() + 1);
        }

        prevButton.setAlpha(1.0);
        if (testLauncher.getCurrentTrialIndex() == testLauncher.getTrialsCount() - 1) {
            nextButton.setAlpha(0.5);
        } else {
            nextButton.setAlpha(1.0);
        }
    } else if (b == &prevButton) {
        if (testLauncher.getCurrentTrialIndex() != 0) {
            saveResponses();
            handleChangeTrialRequest(testLauncher.getCurrentTrialIndex() - 1);
        }
        nextButton.setAlpha(1.0);
        if (testLauncher.getCurrentTrialIndex() == 0) {
            prevButton.setAlpha(0.5);
        } else {
            prevButton.setAlpha(1.0);
        }
    } else {
        bool stimClick = false;
        for (int i = 0; i < stimuliButtons.size(); i++) {
            ratingSliders[i]->setAlpha(SLIDER_DISABLED_ALPHA);
            ratingSliders[i]->setTextBoxIsEditable(false);

            if (b == stimuliButtons[i]) {
                stimClick = true;
                ratingSliders[i]->setAlpha(1.0);
                ratingSliders[i]->setTextBoxIsEditable(true);
                if (currentButton == i + 1) {
                    togglePlayback = true;
                    sendChangeMessage();
                } else {
                    startPlayback = true;
                    sendChangeMessage();
                    currentButton = i + 1;
                    int stim = testLauncher.getCurrentTrial()->refIndex > i ? i : i + 1;
                    selectStimulus(i);

                    // Demo component needs to override the selection because it does not contain a hidden reference.
                    audioPlayer.switchStimulus(testLauncher.getCurrentTrial()->filesOrder[stim]);
                }
                ratingSliders[i]->setAlpha(1.0);
                ratingSliders[i]->setTextBoxIsEditable(true);
            }
        }

        if (!stimClick) {
            BaseTestComponent::buttonClicked(b);
        }
    }
}


bool DemoComponent::saveResponses() {
    Trial *pTrial = testLauncher.getCurrentTrial();
    assert(pTrial->stimuliPlays.size() == pTrial->responses.size());
    for (int i = 0; i < pTrial->stimuliPlays.size(); i++) {
        pTrial->stimuliPlays.set(i, testLauncher.getPlayCount(i));
        pTrial->responses.set(i, int(ratingSliders[i]->getValue()));
    }
    return true;
}

void DemoComponent::clearResponses() {
    Trial *pTrial = testLauncher.getCurrentTrial();

    if (pTrial != NULL) {
        for (int i = 0; i < ratingSliders.size(); i++) {
            ratingSliders[i]->setValue(pTrial->responses[i], dontSendNotification);
            ratingSliders[i]->setColour(Slider::ColourIds::thumbColourId, Colours::grey);
            ratingSliders[i]->setAlpha(SLIDER_DISABLED_ALPHA);

            pTrial->responses.set(i, 100);
            pTrial->responsesMoved.set(i, false);
        }
        for (int i = 0; i < stimuliButtons.size(); i++) {
            stimuliButtons[i]->setTooltip(testLauncher.getSoundFileNameOnly(i));
        }
    }
}

void DemoComponent::readResponses() {
    Trial *pTrial = testLauncher.getCurrentTrial();
    if (pTrial != NULL) {
        for (int i = 0; i < ratingSliders.size(); i++) {
            ratingSliders[i]->setValue(pTrial->responses[i], dontSendNotification);
            ratingSliders[i]->setAlpha(SLIDER_DISABLED_ALPHA);
        }

        for (int i = 0; i < stimuliButtons.size(); i++) {
            stimuliButtons[i]->setTooltip(testLauncher.getSoundFileNameOnly(i));
        }
    }
}


void DemoComponent::createButtons() {
    stimCount = testLauncher.getStimCount();

    /* change the button text if only one trial */
    if (testLauncher.getTrialsCount() == 1) {
        nextButton.setButtonText("done");
    }

    /* create buttons and sliders */
    stimuliButtons.clear();
    ratingSliders.clear();
    for (int i = 0, j = 0; i < stimCount; i++) {
        if (testLauncher.getCurrentTrial()->refIndex != i) {
            String letterStr = String(i + 1);
            stimuliButtons.add(new TextButton("button" + letterStr));
            addAndMakeVisible(stimuliButtons[j]);
            stimuliButtons[j]->setTooltip(testLauncher.getSoundFileNameOnly(i));
            stimuliButtons[j]->setButtonText(letterStr);
            stimuliButtons[j]->setColour(TextButton::buttonColourId, stimuliButtonColour);
            stimuliButtons[j]->setColour(TextButton::buttonOnColourId, activeButtonColour);
            stimuliButtons[j]->addListener(this);
            stimuliButtons[i]->setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
            if (i < 12) {
                stimuliButtons[i]->addShortcut(KeyPress::createFromDescription(stimButtonShortcuts[i]));
            }

            ratingSliders.add(new Slider("slider" + letterStr));
            addAndMakeVisible(ratingSliders[j]);
            ratingSliders[j]->setRange(0, 100, 1);
            ratingSliders[j]->setValue(100);
            ratingSliders[j]->setSliderStyle(Slider::LinearVertical);
            ratingSliders[j]->setTextBoxStyle(Slider::TextBoxBelow, false, 80, 20);
            ratingSliders[j]->setEnabled(false);
            ratingSliders[j]->setAlpha(SLIDER_DISABLED_ALPHA);
            ratingSliders[j]->setTextBoxIsEditable(false);
            ratingSliders[j]->addListener(this);
            j++;
        }
    }
}
