// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once
#include <JuceHeader.h>
#include "../session/TestTypes.h"
#include "../configuration/TestSpecificationRepository.h"
#include <functional>

class NewTestSelectComponent : public Component {
public:
    explicit NewTestSelectComponent(const File& directory);
    ~NewTestSelectComponent() override;
    std::function<void()> onReady;
    bool populateComboBoxWithTestNames();
    String getSubjectID() const { return subjectID; }
    String getTestID() const { return testID; }
    File getSelectedTestFile();
    const StringArray& discoveryErrors() const { return errors; }
    bool getSkipSurvey() const { return skipDemographicSurveyToggle.getToggleState(); }
    void resized() override;
private:
    void ready();
    TestSpecificationRepository repository;
    String subjectID, testID;
    std::vector<StoredTestSpecification> specifications;
    StringArray errors;
    ComboBox testSelectComboBox;
    TextEditor nameTextBox;
    TextButton readyTextButton;
    Label selectYourTestLabel, enterYourNameLabel;
    ToggleButton skipDemographicSurveyToggle;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewTestSelectComponent)
};
