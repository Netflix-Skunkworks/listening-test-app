// Listening Test Application — Copyright(C) 2017-2026 Netflix, Inc.
#include "BsuSurveyComponent.h"
#include "AppTheme.h"

StringArray BsuSurveyComponent::validateResponses() const {
    StringArray errors;
    validateNaturalNumber(ageBox.getText(), "Age", errors);
    validateNaturalNumber(yearsMusicianBox.getText(), "Years of musical training", errors, false);
    validateNaturalNumber(yearsEarTrainingBox.getText(), "Years of ear training", errors, false);
    auto choice = [&](const ToggleButton& yes, const ToggleButton& no, const String& label) {
        if (!yes.getToggleState() && !no.getToggleState()) errors.add("Please answer: " + label);
    };
    choice(screeningYes, screeningNo, "Hearing screening results");
    choice(listenerGroupTrained, listenerGroupNovel, "Listener training group");
    choice(workTeamYes, workTeamNo, "Work team membership");
    if (!deptTCOM.getToggleState() && !deptMMP.getToggleState() && !deptOther.getToggleState())
        errors.add("Please indicate the subject's department.");
    if (deptOther.getToggleState() && novelListenerDept.getText().isEmpty())
        errors.add("Please enter the other department.");
    choice(nativeEnglishYes, nativeEnglishNo, "Is English your native language?");
    if (nativeEnglishNo.getToggleState() && nativeLanguageBox.getText().isEmpty())
        errors.add("Please enter your native language.");
    choice(loudSoundYes, loudSoundNo, "Recent exposure to loud sounds");
    if (loudSoundYes.getToggleState()) {
        choice(hearingProtectionYes, hearingProtectionNo, "Use of hearing protection");
        if (hearingProtectionYes.getToggleState() && hearingProtectionBox.getText().isEmpty())
            errors.add("Please enter the type of hearing protection.");
    }
    return errors;
}

std::unique_ptr<XmlElement> BsuSurveyComponent::getSurveyResults() const {
    auto xml = std::make_unique<XmlElement>("surveyResults");
    xml->setAttribute("surveyCompleted", surveyCompleted ? "1" : "0");
    if (!surveyCompleted) return xml;
    auto boolean = [&](const char* name, const ToggleButton& yes) {
        xml->setAttribute(name, yes.getToggleState() ? "true" : "false");
    };
    xml->setAttribute("age", ageBox.getText().getIntValue());
    boolean("passed15dbTest", screeningYes);
    boolean("listenerIsTrained", listenerGroupTrained);
    boolean("isWorkTeam", workTeamYes);
    xml->setAttribute("studentDept", deptTCOM.getToggleState() ? String("TCOM")
                      : deptMMP.getToggleState() ? String("MMP") : novelListenerDept.getText());
    xml->setAttribute("isMusician", yearsMusicianBox.getText().getIntValue() > 0 ? "true" : "false");
    xml->setAttribute("yearsMusician", yearsMusicianBox.getText().getIntValue());
    xml->setAttribute("yearsOfHearingTraining", yearsEarTrainingBox.getText().getIntValue());
    boolean("nativeLanguageIsEnglish", nativeEnglishYes);
    xml->setAttribute("nativeLanguage", nativeLanguageBox.getText());
    xml->setAttribute("otherLanguages", secondLanguageBox.getText());
    boolean("heardLoudSoundsRecently", loudSoundYes);
    boolean("usedHearingProtection", hearingProtectionYes);
    xml->setAttribute("hearingProtectionType", hearingProtectionBox.getText());
    return xml;
}

BsuSurveyComponent::BsuSurveyComponent() {
    addAndMakeVisible(okButton);
    okButton.setButtonText("OK");
    okButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(administratorLabel);
    administratorLabel.setText(administratorString, dontSendNotification);
    administratorLabel.setColour(Label::textColourId, Colours::indianred);
    administratorLabel.setFont(Font(administratorLabel.getFont().getHeight() * 1.5, Font::bold));

    addAndMakeVisible(subjectLabel);
    subjectLabel.setText(subjectString, dontSendNotification);
    subjectLabel.setColour(Label::textColourId, Colours::indianred);
    subjectLabel.setFont(Font(subjectLabel.getFont().getHeight() * 1.5, Font::bold));

    addAndMakeVisible(ageBox);
    addAndMakeVisible(ageLabel);
    ageLabel.setText("Age", dontSendNotification);

    addAndMakeVisible(screeningLabel);
    screeningLabel.setText(screeningLabelString, dontSendNotification);

    addAndMakeVisible(screeningYes);
    screeningYes.setButtonText("YES");

    addAndMakeVisible(screeningNo);
    screeningNo.setButtonText("NO");

    addAndMakeVisible(listenerGroupLabel);
    listenerGroupLabel.setText(listenerGroup, dontSendNotification);
    addAndMakeVisible(listenerGroupNovel);
    listenerGroupNovel.setButtonText("Novel");
    addAndMakeVisible(listenerGroupTrained);
    listenerGroupTrained.setButtonText("Trained");

    addAndMakeVisible(deptTCOM);
    deptTCOM.setButtonText("TCOM");
    addAndMakeVisible(deptMMP);
    deptMMP.setButtonText("MMP");
    addAndMakeVisible(deptOther);
    deptOther.setButtonText("Other (enter at right):");
    addAndMakeVisible(deptLabel);
    deptLabel.setText(department, dontSendNotification);
    addAndMakeVisible(novelListenerDept);

    addAndMakeVisible(workTeamLabel);
    workTeamLabel.setText(workTeam, dontSendNotification);
    addAndMakeVisible(workTeamYes);
    workTeamYes.setButtonText("YES");
    addAndMakeVisible(workTeamNo);
    workTeamNo.setButtonText("NO");

    addAndMakeVisible(musicianLabel);
    musicianLabel.setText(musicianString, dontSendNotification);
    addAndMakeVisible(yearsMusicianBox);

    addAndMakeVisible(earTrainingLabel);
    earTrainingLabel.setText(earTrainingString, dontSendNotification);
    addAndMakeVisible(yearsEarTrainingBox);

    addAndMakeVisible(nativeEnglishLabel);
    nativeEnglishLabel.setText(nativeEnglishString, dontSendNotification);

    addAndMakeVisible(nativeEnglishYes);
    nativeEnglishYes.setButtonText("YES");
    addAndMakeVisible(nativeEnglishNo);
    nativeEnglishNo.setButtonText("NO");
    addAndMakeVisible(nativeLanguageBox);

    addAndMakeVisible(secondLanguageLabel);
    secondLanguageLabel.setText(secondLanguageString, dontSendNotification);
    addAndMakeVisible(secondLanguageBox);

    addAndMakeVisible(loudSoundLabel);
    loudSoundLabel.setText(loudSoundQuestionString, dontSendNotification);
    addAndMakeVisible(loudSoundYes);
    loudSoundYes.setButtonText("YES");
    addAndMakeVisible(loudSoundNo);
    loudSoundNo.setButtonText("NO");

    addAndMakeVisible(hearingProtectionLabel);
    hearingProtectionLabel.setText(hearingProtectionString, dontSendNotification);
    addAndMakeVisible(hearingProtectionYes);
    hearingProtectionYes.setButtonText("YES");
    addAndMakeVisible(hearingProtectionNo);
    hearingProtectionNo.setButtonText("NO");
    addAndMakeVisible(hearingProtectionBox);

    screeningYes.setRadioGroupId(ScreeningButtons);
    screeningNo.setRadioGroupId(ScreeningButtons);

    hearingProtectionYes.setRadioGroupId(HearingProtectionButtons);
    hearingProtectionNo.setRadioGroupId(HearingProtectionButtons);

    loudSoundYes.setRadioGroupId(LoudSoundButtons);
    loudSoundNo.setRadioGroupId(LoudSoundButtons);

    nativeEnglishYes.setRadioGroupId(NativeEnglishButtons);
    nativeEnglishNo.setRadioGroupId(NativeEnglishButtons);

    listenerGroupNovel.setRadioGroupId(ListenerGroupButtons);
    listenerGroupTrained.setRadioGroupId(ListenerGroupButtons);

    deptTCOM.setRadioGroupId(DeptButtons);
    deptMMP.setRadioGroupId(DeptButtons);
    deptOther.setRadioGroupId(DeptButtons);

    workTeamYes.setRadioGroupId(WorkGroupButtons);
    workTeamNo.setRadioGroupId(WorkGroupButtons);

    setSize(800, 580);
}

void BsuSurveyComponent::resized() {
    const int spacing = 12;
    const int groupSpacing = 4;
    const int buttonW = 60;

    int x = spacing;

    // Test Administrator Fields
    AppTheme::setupLabel(administratorLabel, x, 0);

    int y = administratorLabel.getHeight()/2;
    administratorLabel.setCentrePosition(getWidth() / 2, administratorLabel.getHeight()/2);
    y = administratorLabel.getBottom() + spacing;

    AppTheme::setupLabel(ageLabel, x, y);
    ageBox.setBounds(ageLabel.getRight() + groupSpacing, y, buttonW / 2, ageLabel.getHeight());

    AppTheme::setupLabel(screeningLabel, ageBox.getRight() + spacing * 2, y);
    screeningYes.setBounds(screeningLabel.getRight() + groupSpacing, y, buttonW, screeningLabel.getHeight());
    screeningNo.setBounds(screeningYes.getRight(), y, buttonW, screeningLabel.getHeight());
    y = screeningLabel.getBottom() + spacing;

    AppTheme::setupLabel(listenerGroupLabel, x, y);
    listenerGroupNovel.setBounds(listenerGroupLabel.getRight(), y, buttonW, listenerGroupLabel.getHeight());
    listenerGroupTrained.setBounds(listenerGroupNovel.getRight() + groupSpacing, y, buttonW * 1.5, listenerGroupLabel.getHeight());

    AppTheme::setupLabel(workTeamLabel, listenerGroupTrained.getRight() + spacing, y);
    workTeamYes.setBounds(workTeamLabel.getRight() + groupSpacing, y, buttonW, workTeamLabel.getHeight());
    workTeamNo.setBounds(workTeamYes.getRight(), y, buttonW, workTeamLabel.getHeight());
    y = workTeamLabel.getBottom() + spacing;

    AppTheme::setupLabel(deptLabel, x, y);
    deptTCOM.setBounds(deptLabel.getRight() + groupSpacing, y, buttonW, deptLabel.getHeight());
    deptMMP.setBounds(deptTCOM.getRight() + groupSpacing, y, buttonW, deptLabel.getHeight());
    deptOther.setBounds(deptMMP.getRight() + groupSpacing, y, buttonW * 2.5, deptLabel.getHeight());
    novelListenerDept.setBounds(deptOther.getRight(), y, buttonW, deptLabel.getHeight());
    y = deptLabel.getBottom() + spacing;

    AppTheme::setupLabel(loudSoundLabel, x, y);
    loudSoundYes.setBounds(loudSoundLabel.getRight() + groupSpacing, y, buttonW, loudSoundLabel.getHeight());
    loudSoundNo.setBounds(loudSoundYes.getRight(), y, buttonW, loudSoundLabel.getHeight());
    y = loudSoundLabel.getBottom() + spacing;

    AppTheme::setupLabel(hearingProtectionLabel, x, y);
    hearingProtectionYes.setBounds(hearingProtectionLabel.getRight() + groupSpacing, y, buttonW, hearingProtectionLabel.getHeight());
    hearingProtectionNo.setBounds(hearingProtectionYes.getRight(), y, buttonW, hearingProtectionLabel.getHeight());
    hearingProtectionBox.setBounds(hearingProtectionNo.getRight(), y, buttonW * 2, hearingProtectionLabel.getHeight());

    y = hearingProtectionLabel.getBottom() + spacing*2;

    // DIVIDING LINE
    administratorSubjectDivider.clear();
    administratorSubjectDivider.startNewSubPath(x, y);
    administratorSubjectDivider.lineTo(getRight() - spacing, y);
    administratorSubjectDivider.closeSubPath();

    y += spacing*2;

    // Test Subject Fields
    AppTheme::setupLabel(subjectLabel, x, y);
    subjectLabel.setCentrePosition(getWidth() / 2, y);
    y += spacing*2;

    AppTheme::setupLabel(musicianLabel, x, y);
    yearsMusicianBox.setBounds(musicianLabel.getRight() + groupSpacing, y, buttonW / 2, musicianLabel.getHeight());
    y = musicianLabel.getBottom() + spacing;

    AppTheme::setupLabel(earTrainingLabel, x, y);
    yearsEarTrainingBox.setBounds(earTrainingLabel.getRight() + groupSpacing, y, buttonW / 2, earTrainingLabel.getHeight());
    y = earTrainingLabel.getBottom() + spacing;

    AppTheme::setupLabel(nativeEnglishLabel, x, y);
    y = nativeEnglishLabel.getBottom() + groupSpacing;

    int x_indent = x + spacing;
    nativeEnglishYes.setBounds(x_indent, y, buttonW, nativeEnglishLabel.getHeight());
    nativeEnglishNo.setBounds(x_indent + spacing + buttonW, y, buttonW, nativeEnglishLabel.getHeight());
    nativeLanguageBox.setBounds(x_indent + (spacing + buttonW) * 2, y, buttonW * 2, nativeEnglishLabel.getHeight());
    y = nativeLanguageBox.getBottom() + spacing;

    AppTheme::setupLabel(secondLanguageLabel, x, y);
    secondLanguageBox.setBounds(secondLanguageLabel.getRight() + groupSpacing, y, buttonW * 2, secondLanguageLabel.getHeight());
    y = secondLanguageLabel.getBottom() + spacing*2;

    okButton.setSize(AppTheme::buttonWidth * 2, AppTheme::buttonHeight);
    okButton.setCentrePosition(getWidth() / 2, y+AppTheme::buttonHeight/2);

}

void BsuSurveyComponent::clearSurveyResponses() {
    surveyCompleted = false;
    ageBox.clear();

    screeningYes.setToggleState(false, dontSendNotification);
    screeningNo.setToggleState(false, dontSendNotification);

    listenerGroupNovel.setToggleState(false, dontSendNotification);
    listenerGroupTrained.setToggleState(false, dontSendNotification);

    deptTCOM.setToggleState(false, dontSendNotification);
    deptMMP.setToggleState(false, dontSendNotification);
    deptOther.setToggleState(false, dontSendNotification);
    novelListenerDept.clear();

    workTeamYes.setToggleState(false, dontSendNotification);
    workTeamNo.setToggleState(false, dontSendNotification);

    yearsMusicianBox.clear();

    yearsEarTrainingBox.clear();

    nativeEnglishYes.setToggleState(false, dontSendNotification);
    nativeEnglishNo.setToggleState(false, dontSendNotification);
    nativeLanguageBox.clear();
    secondLanguageBox.clear();

    loudSoundYes.setToggleState(false, dontSendNotification);
    loudSoundNo.setToggleState(false, dontSendNotification);

    hearingProtectionYes.setToggleState(false, dontSendNotification);
    hearingProtectionNo.setToggleState(false, dontSendNotification);
    hearingProtectionBox.clear();

}

void BsuSurveyComponent::paint(Graphics& g) {
    g.fillAll(Colours::darkgrey);

    g.setColour(Colour(0x0));
    g.fillPath(administratorSubjectDivider);
    g.setColour(Colours::black);
    g.strokePath(administratorSubjectDivider, PathStrokeType(1.5f));
}
