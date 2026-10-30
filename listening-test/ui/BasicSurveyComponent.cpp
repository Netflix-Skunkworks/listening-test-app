// Listening Test Application — Copyright(C) 2017-2026 Netflix, Inc.
#include "BasicSurveyComponent.h"
#include "AppTheme.h"

StringArray BasicSurveyComponent::validateResponses() const {
    StringArray errors;
    validateNaturalNumber(ageBox.getText(), "Age", errors);
    validateNaturalNumber(yearsOfExperienceBox.getText(), "Years of experience", errors);
    validateNaturalNumber(numberOfListeningTestsBox.getText(), "Number of listening tests", errors);
    if (!normalHearingYesButton.getToggleState() && !normalHearingNoButton.getToggleState()
        && !normalHearingUnknownButton.getToggleState())
        errors.add("Please answer: Do you have normal hearing?");
    if (!recentTestYesButton.getToggleState() && !recentTestNoButton.getToggleState())
        errors.add("Please answer: Have you participated in a listening test in the past year?");
    return errors;
}

std::unique_ptr<XmlElement> BasicSurveyComponent::getSurveyResults() const {
    auto xml = std::make_unique<XmlElement>("surveyResults");
    xml->setAttribute("surveyCompleted", surveyCompleted ? "yes" : "no");
    if (!surveyCompleted) return xml;
    xml->setAttribute("age", ageBox.getText().getIntValue());
    xml->setAttribute("normalHearing", normalHearingYesButton.getToggleState() ? "yes"
                      : normalHearingNoButton.getToggleState() ? "no" : "unknown");
    xml->setAttribute("yearsOfExperience", yearsOfExperienceBox.getText().getIntValue());
    xml->setAttribute("numberOfPreviousListeningTests", numberOfListeningTestsBox.getText().getIntValue());
    xml->setAttribute("testTakenInLastYear", recentTestYesButton.getToggleState() ? "yes" : "no");
    return xml;
}

BasicSurveyComponent::BasicSurveyComponent() {
    addAndMakeVisible(okButton);
    okButton.setButtonText("OK");
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

void BasicSurveyComponent::resized() {
    const int spacing = 30;
    const int groupSpacing = 8;

    int x = spacing;
    int y = spacing;

    AppTheme::setupLabel(ageLabel, x, y);
    ageBox.setBounds(ageLabel.getRight() + groupSpacing, y, 32, ageLabel.getHeight());

    y = ageLabel.getBottom() + spacing;

    AppTheme::setupLabel(normalHearingLabel, x, y);
    normalHearingYesButton.setBounds(normalHearingLabel.getRight() + groupSpacing, y, 64, normalHearingLabel.getHeight());
    normalHearingNoButton.setBounds(normalHearingYesButton.getRight(), y, 64, normalHearingLabel.getHeight());
    normalHearingUnknownButton.setBounds(normalHearingNoButton.getRight(), y, 64, normalHearingLabel.getHeight());

    y = normalHearingLabel.getBottom() + spacing;

    AppTheme::setupLabel(experienceLabel, x, y);
    yearsOfExperienceBox.setBounds(experienceLabel.getRight()+groupSpacing, y, 32, experienceLabel.getHeight()/3);

    y = experienceLabel.getBottom() + spacing;

    AppTheme::setupLabel(numberOfListeningTestsLabel, x, y);
    numberOfListeningTestsBox.setBounds(numberOfListeningTestsLabel.getRight() + groupSpacing, y, 32, numberOfListeningTestsLabel.getHeight());

    y = numberOfListeningTestsLabel.getBottom() + spacing;

    AppTheme::setupLabel(recentTestLabel, x, y);
    recentTestYesButton.setBounds(recentTestLabel.getRight() + groupSpacing, y, 64, recentTestLabel.getHeight());
    recentTestNoButton.setBounds(recentTestYesButton.getRight() + groupSpacing, y, 64, recentTestLabel.getHeight());

    y = recentTestLabel.getBottom() + spacing*2;

    okButton.setSize(AppTheme::buttonWidth * 2, AppTheme::buttonHeight);
    okButton.setCentrePosition(getWidth() / 2, y);

}

void BasicSurveyComponent::clearSurveyResponses() {
    surveyCompleted = false;
    ageBox.clear();
    normalHearingYesButton.setToggleState(false, dontSendNotification);
    normalHearingNoButton.setToggleState(false, dontSendNotification);
    normalHearingUnknownButton.setToggleState(false, dontSendNotification);
    yearsOfExperienceBox.clear();
    numberOfListeningTestsBox.clear();
    recentTestYesButton.setToggleState(false, dontSendNotification);
    recentTestNoButton.setToggleState(false, dontSendNotification);
}
