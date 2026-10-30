//
//    Listening Test Application
//    Copyright(C) 2017-2026  Netflix, Inc.

#pragma once

#include "SurveyComponent.h"

class BsuSurveyComponent : public SurveyComponent {
public:

    BsuSurveyComponent();

    ~BsuSurveyComponent() override = default;

    void paint(Graphics &g) override;

    void resized() override;

    StringArray validateResponses() const override;
    std::unique_ptr<XmlElement> getSurveyResults() const override;

    void clearSurveyResponses() override;

private:

    const String administratorString = "TO BE COMPLETED BY TEST ADMINISTRATOR";
    Label administratorLabel;
    const String subjectString = "TO BE COMPLETED BY TEST SUBJECT";
    Label subjectLabel;

    TextEditor ageBox;
    Label ageLabel;

    const String screeningLabelString = "Hearing Screening showed all thresholds 15 dB HL or better?";
    Label screeningLabel;
    ToggleButton screeningYes;
    ToggleButton screeningNo;

    const String listenerGroup = "Listener Group:";
    Label listenerGroupLabel;
    ToggleButton listenerGroupNovel;
    ToggleButton listenerGroupTrained;

    const String department = "University Department:";
    Label deptLabel;
    ToggleButton deptTCOM;
    ToggleButton deptMMP;
    ToggleButton deptOther;
    TextEditor novelListenerDept;

    const String workTeam = "Work Team?";
    Label workTeamLabel;
    ToggleButton workTeamYes;
    ToggleButton workTeamNo;

    Path administratorSubjectDivider;

    const String musicianString = "If you play a musical instrument, how many years have you played?  (0 if you don't play an instrument.)";
    Label musicianLabel;
    TextEditor yearsMusicianBox;

    const String earTrainingString = "How many years of musical ear training do you have?";
    Label earTrainingLabel;
    TextEditor yearsEarTrainingBox;

    const String nativeEnglishString = "Is American English your native language?  If not, please enter your native language in the box below.";
    Label nativeEnglishLabel;
    ToggleButton nativeEnglishYes;
    ToggleButton nativeEnglishNo;
    TextEditor nativeLanguageBox;

    const String secondLanguageString = "If you speak a second language other than American English, enter it here:";
    Label secondLanguageLabel;
    TextEditor secondLanguageBox;

    const String loudSoundQuestionString = "Has the subject been exposed to loud sounds (music, engines, gunfire) within the past 48 hours?";
    Label loudSoundLabel;
    ToggleButton loudSoundYes;
    ToggleButton loudSoundNo;

    const String hearingProtectionString = "Did they use hearing protection?  If yes, enter what kind or type in the box.\n(Examples: foam earplugs, rubber honeycomb-style plugs or ear muffs)";
    Label hearingProtectionLabel;
    ToggleButton hearingProtectionYes;
    ToggleButton hearingProtectionNo;
    TextEditor hearingProtectionBox;

    enum RadioButtonIds {
        ScreeningButtons = 1001,
        NativeEnglishButtons,
        LoudSoundButtons,
        HearingProtectionButtons,
        ListenerGroupButtons,
        DeptButtons,
        WorkGroupButtons
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BsuSurveyComponent);
};
