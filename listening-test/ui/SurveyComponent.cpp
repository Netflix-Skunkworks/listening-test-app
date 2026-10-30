// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#include "SurveyComponent.h"
#include <limits>

SurveyComponent::SurveyComponent() {
    addAndMakeVisible(okButton);
    okButton.setButtonText("Continue");
    okButton.onClick = [this] {
        const auto errors = validateResponses();
        surveyCompleted = errors.isEmpty();
        if (!errors.isEmpty())
            AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Please complete the survey",
                errors.joinIntoString("\n"));
        else if (onContinue) onContinue();
    };
    setSize(800, 400);
}
SurveyComponent::~SurveyComponent() = default;
void SurveyComponent::validateNaturalNumber(const String& text, const String& label,
                                           StringArray& errors, bool required) {
    if (text.isEmpty() && !required) return;
    bool valid = text.isNotEmpty();
    int value = 0;
    for (auto character : text) {
        if (character < '0' || character > '9'
            || value > (std::numeric_limits<int>::max() - (character - '0')) / 10) {
            valid = false;
            break;
        }
        value = value * 10 + (character - '0');
    }
    if (!valid) errors.add(label + " must be a natural number.");
}
void SurveyComponent::paint(juce::Graphics& g) { g.fillAll(Colours::darkgrey); }
void SurveyComponent::resized() {
    okButton.setSize(getWidth() / 8, getHeight() / 8);
    okButton.setCentrePosition(getWidth() / 2, getHeight() / 2);
}
