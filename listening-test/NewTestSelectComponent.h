//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef NEW_TEST_SELECT_COMPONENT_H
#define NEW_TEST_SELECT_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "guisettings.h"

class NewTestSelectComponent : public Component,
                               public Button::Listener,
                               public ChangeBroadcaster {
public:
    NewTestSelectComponent();

    ~NewTestSelectComponent();

    bool populateComboBoxWithTestNames();

    String getSubjectID() { return subjectID; };

    String getTestID() { return testID; };

    int getTestIdx() { return testIdx; };

    bool getSkipSurvey() { return skipDemographicSurvey; };

    String getStimDirectory();

    void initiateLoading();

    void resized();

    void comboBoxChanged(ComboBox *c);

    void buttonClicked(Button *b);


private:
    String fileName;
    String subjectID;
    String testID = String();
    int testIdx = -1;
    bool skipDemographicSurvey = false;

    ComboBox testSelectComboBox;
    TextEditor nameTextBox;
    TextButton readyTextButton;
    Label selectYourTestLabel;
    Label enterYourNameLabel;
    ToggleButton skipDemographicSurveyToggle;

    class MyButtonListener : public Button::Listener {
    public:
        MyButtonListener(NewTestSelectComponent &c) : owner(c) {};

        ~MyButtonListener() {};

        void buttonClicked(Button *b) {
            if (b == &owner.skipDemographicSurveyToggle) {
                owner.skipDemographicSurvey = owner.skipDemographicSurveyToggle.getToggleState();
            }
        }

    private:
        NewTestSelectComponent &owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyButtonListener);
    } buttonListener;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewTestSelectComponent);
};

#endif   // NEW_TEST_SELECT_COMPONENT_H
