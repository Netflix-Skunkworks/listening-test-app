//
//    Listening Test Application
//    Copyright(C) 2017-2026  Netflix, Inc.

#pragma once

#include "SurveyComponent.h"

class BasicSurveyComponent : public SurveyComponent
{
public:
    BasicSurveyComponent();

    ~BasicSurveyComponent() override = default;

    void resized() override;

    StringArray validateResponses() const override;
    std::unique_ptr<XmlElement> getSurveyResults() const override;

    void clearSurveyResponses() override;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicSurveyComponent)
};
