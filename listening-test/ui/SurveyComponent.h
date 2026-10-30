// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once
#include <JuceHeader.h>
#include <functional>

class SurveyComponent : public juce::Component {
public:
    SurveyComponent();
    ~SurveyComponent() override = 0;
    std::function<void()> onContinue;
    void paint(juce::Graphics&) override;
    void resized() override;
    // Each survey owns its fields, validation, and XML schema. Return a
    // surveyResults element; session storage preserves its entire subtree.
    virtual juce::StringArray validateResponses() const = 0;
    virtual std::unique_ptr<juce::XmlElement> getSurveyResults() const = 0;
    virtual void clearSurveyResponses() = 0;
protected:
    static void validateNaturalNumber(const juce::String& text, const juce::String& label,
                                      juce::StringArray& errors, bool required = true);
    bool surveyCompleted = false;
    TextButton okButton;
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SurveyComponent)
};
