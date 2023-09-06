//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.


#pragma once

#include <JuceHeader.h>
#include "SurveyComponent.h"

//==============================================================================
/*
*/
class BasicSurveyComponent : public SurveyComponent
{
public:
    BasicSurveyComponent() {
        addAndMakeVisible(okButton);
        okButton.setButtonText("OK");
        okButton.addListener(&buttonListener);
        okButton.setColour(ToggleButton::tickDisabledColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
        
        ageLabel.setText("Age", dontSendNotification);
        addAndMakeVisible(ageLabel);

        addAndMakeVisible(ageBox);

        normalHearingLabel.setText("Do you have normal hearing?", dontSendNotification);
        addAndMakeVisible(normalHearingLabel);

        normalHearingYesButton.setButtonText("Yes");
        normalHearingYesButton.setRadioGroupId(1002);
        addAndMakeVisible(normalHearingYesButton);

        normalHearingNoButton.setButtonText("No");
        normalHearingNoButton.setRadioGroupId(1002);
        addAndMakeVisible(normalHearingNoButton);

        normalHearingUnknownButton.setButtonText("Don't know");
        normalHearingUnknownButton.setRadioGroupId(1002);
        addAndMakeVisible(normalHearingUnknownButton);

        experienceLabel.setText("How many years of professional audio experience do you have?\n"
                                "(e.g. sound mixing/production, music performance,\n"
                                "ear training, audio r&d, etc.)", dontSendNotification);
        addAndMakeVisible(experienceLabel);
        addAndMakeVisible(yearsOfExperienceBox);
        
        numberOfListeningTestsLabel.setText("How many formal listening tests have you participated in?", dontSendNotification);
        addAndMakeVisible(numberOfListeningTestsLabel);
        addAndMakeVisible(numberOfListeningTestsBox);
        
        recentTestLabel.setText("Have you participated in a formal listening test in the past year?", dontSendNotification);
        addAndMakeVisible(recentTestLabel);
        
        recentTestYesButton.setButtonText("Yes");
        recentTestYesButton.setRadioGroupId(1003);
        addAndMakeVisible(recentTestYesButton);

        recentTestNoButton.setButtonText("No");
        recentTestNoButton.setRadioGroupId(1003);
        addAndMakeVisible(recentTestNoButton);

        setSize(800, 400);

    }

    ~BasicSurveyComponent() override {}

    void resized() override {
        const int spacing = 30;
        const int groupSpacing = 8;

        int x = spacing;
        int y = spacing;
        
        setupLabel(ageLabel, x, y);
        ageBox.setBounds(ageLabel.getRight() + groupSpacing, y, 32, ageLabel.getHeight());
        
        y = ageLabel.getBottom() + spacing;
        
        setupLabel(normalHearingLabel, x, y);
        normalHearingYesButton.setBounds(normalHearingLabel.getRight() + groupSpacing, y, 64, normalHearingLabel.getHeight());
        normalHearingNoButton.setBounds(normalHearingYesButton.getRight(), y, 64, normalHearingLabel.getHeight());
        normalHearingUnknownButton.setBounds(normalHearingNoButton.getRight(), y, 64, normalHearingLabel.getHeight());
        
        y = normalHearingLabel.getBottom() + spacing;

        setupLabel(experienceLabel, x, y);
        yearsOfExperienceBox.setBounds(experienceLabel.getRight()+groupSpacing, y, 32, experienceLabel.getHeight()/3);
        
        y = experienceLabel.getBottom() + spacing;
        
        setupLabel(numberOfListeningTestsLabel, x, y);
        numberOfListeningTestsBox.setBounds(numberOfListeningTestsLabel.getRight() + groupSpacing, y, 32, numberOfListeningTestsLabel.getHeight());
        
        y = numberOfListeningTestsLabel.getBottom() + spacing;
        
        setupLabel(recentTestLabel, x, y);
        recentTestYesButton.setBounds(recentTestLabel.getRight() + groupSpacing, y, 64, recentTestLabel.getHeight());
        recentTestNoButton.setBounds(recentTestYesButton.getRight() + groupSpacing, y, 64, recentTestLabel.getHeight());

        y = recentTestLabel.getBottom() + spacing*2;

        okButton.setSize(BUTTON_W * 2, BUTTON_H);
        okButton.setCentrePosition(getWidth() / 2, y);
       
    }
    
    XmlElement getSurveyResultsXml() override {
        XmlElement surveyResultsXml("surveyResults");
        surveyResultsXml.setAttribute("surveyCompleted", surveyCompleted ? "yes" : "no");

        if (surveyCompleted) {
            surveyResultsXml.setAttribute("age", ageBox.getText().getIntValue());
            surveyResultsXml.setAttribute("normalHearing", normalHearingYesButton.getToggleState() ? "yes" : (normalHearingNoButton.getToggleState() ? "no" : "unknown"));
            surveyResultsXml.setAttribute("yearsOfExperience", yearsOfExperienceBox.getText().getIntValue());
            surveyResultsXml.setAttribute("numberOfPreviousListeningTests", numberOfListeningTestsBox.getText().getIntValue());
            surveyResultsXml.setAttribute("testTakenInLastYear", recentTestYesButton.getToggleState() ? "yes" : "no");
        }

        return surveyResultsXml;
    }
    
    void clearSurveyResponses() override {
        ageBox.clear();
        normalHearingYesButton.setToggleState(false, dontSendNotification);
        normalHearingNoButton.setToggleState(false, dontSendNotification);
        normalHearingUnknownButton.setToggleState(false, dontSendNotification);
        yearsOfExperienceBox.clear();
        numberOfListeningTestsBox.clear();
        recentTestYesButton.setToggleState(false, dontSendNotification);
        recentTestNoButton.setToggleState(false, dontSendNotification);
    }

private:
    Label ageLabel;
    TextEditor ageBox;
    
    Label normalHearingLabel;
    ToggleButton normalHearingYesButton;
    ToggleButton normalHearingNoButton;
    ToggleButton normalHearingUnknownButton;

    Label experienceLabel;
    TextEditor yearsOfExperienceBox;
    
    Label numberOfListeningTestsLabel;
    TextEditor numberOfListeningTestsBox;
    
    Label recentTestLabel;
    ToggleButton recentTestYesButton;
    ToggleButton recentTestNoButton;

    int verifyData() override {
        int returnVal = 0;
        
        String numberString = ageBox.getText();
        if (numberString.isEmpty() || !numberString.containsOnly("0123456789")) {
            AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Invalid age",
                                             "Your age must be a natural number.");
            returnVal++;
        }
        
        if(!normalHearingNoButton.getToggleState() &&
           !normalHearingYesButton.getToggleState() &&
           !normalHearingUnknownButton.getToggleState()) {
            AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Normal Hearing?",
                                             "Please indicate if you have normal hearing yes/no/unknown.");
            returnVal++;
        }

        numberString = yearsOfExperienceBox.getText();
        if (numberString.isEmpty() || !numberString.containsOnly("0123456789")) {
            AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Invalid years of experience",
                                             "Your years of experience must be a natural number.");
            returnVal++;
        }
        
        numberString = numberOfListeningTestsBox.getText();
        if (numberString.isEmpty() || !numberString.containsOnly("0123456789")) {
            AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Invalid number of listening tests",
                                             "Your number of listening tests must be a natural number.");
            returnVal++;
        }

        if (!recentTestYesButton.getToggleState() && !recentTestNoButton.getToggleState()) {
            AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Recent formal listening test?",
                                             "Please indicate if you participated in a recent formal listening test yes/no.");
            returnVal++;
        }

        surveyCompleted = returnVal == 0;
        return returnVal;
        
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicSurveyComponent)
};
