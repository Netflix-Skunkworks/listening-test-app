//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017-2026  Netflix, Inc.

#include "NewTestSelectComponent.h"
#include "AppTheme.h"

//==============================================================================
NewTestSelectComponent::NewTestSelectComponent(const File& directory) : repository(directory) {
    addAndMakeVisible(&testSelectComboBox);
    testSelectComboBox.setEditableText(false);
    testSelectComboBox.setJustificationType(Justification::centredLeft);
    testSelectComboBox.setTextWhenNothingSelected(String());
    testSelectComboBox.setTextWhenNoChoicesAvailable("(empty)");

    addAndMakeVisible(&nameTextBox);
    nameTextBox.setTextToShowWhenEmpty("My name", Colours::lightgrey);

    addAndMakeVisible(&readyTextButton);
    readyTextButton.setButtonText("Ready!");
    readyTextButton.onClick = [this] { ready(); };
    readyTextButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&selectYourTestLabel);
    selectYourTestLabel.setText("Select your test:", dontSendNotification);
    selectYourTestLabel.setFont(Font(15.0000f, Font::plain));
    selectYourTestLabel.setJustificationType(Justification::centredLeft);
    selectYourTestLabel.setEditable(false, false, false);
    selectYourTestLabel.setColour(TextEditor::textColourId, Colours::black);
    selectYourTestLabel.setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(&enterYourNameLabel);
    enterYourNameLabel.setText("Enter your name:", dontSendNotification);
    enterYourNameLabel.setFont(Font(15.0000f, Font::plain));
    enterYourNameLabel.setJustificationType(Justification::centredLeft);
    enterYourNameLabel.setEditable(false, false, false);
    enterYourNameLabel.setColour(TextEditor::textColourId, Colours::black);
    enterYourNameLabel.setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(&skipDemographicSurveyToggle);
    skipDemographicSurveyToggle.setButtonText("Skip Demographic Survey");
    skipDemographicSurveyToggle.setToggleState(false, dontSendNotification);

}

NewTestSelectComponent::~NewTestSelectComponent() {
}

//==============================================================================
void NewTestSelectComponent::resized() {
    const int center = getWidth() / 2;
    selectYourTestLabel.setBounds(center - 150 / 2, 16, 150, 24);
    testSelectComboBox.setBounds(center - 304 / 2, 48, 304, 24);

    enterYourNameLabel.setBounds(center - 150 / 2, 96, 150, 24);
    nameTextBox.setBounds(center - 304 / 2, 128, 304, 24);

    skipDemographicSurveyToggle.setBounds(center - 304 / 2, 160, 304, 24);

    readyTextButton.setBounds(center - 150 / 2, 208, 150, 48);

}

void NewTestSelectComponent::ready() {
    if ((testSelectComboBox.getSelectedId() > 0) && (nameTextBox.getText().length() > 0)) {
        testID = testSelectComboBox.getText();

        // Get User Name from Text Box
        std::string s = (const char *) nameTextBox.getText().getCharPointer();

        // Remove any illegal characters
        std::string::iterator i = s.begin();
        while (i != s.end()) {
            char curr_char = *i;
            if (!isalnum(curr_char) && curr_char != ' ' && curr_char != '-' && curr_char != '_' &&
                curr_char != '.') {
                i = s.erase(i);
            } else {
                i++;
            }
        }
        // If there's anything left, pass it on.  If not, just continue as Unknown
        if (s.length() > 0)
            subjectID = s.c_str();
        else
            subjectID = "Unknown";

        if (onReady) onReady();

    }
}


bool NewTestSelectComponent::populateComboBoxWithTestNames() {
    specifications.clear();
    testSelectComboBox.clear();
    const auto result = repository.discover(specifications, errors);
    if (result.failed()) errors.add(result.getErrorMessage());
    for (size_t i = 0; i < specifications.size(); ++i)
        testSelectComboBox.addItem(specifications[i].specification.name, (int) i + 1);
    if (specifications.empty()) return false;

    testSelectComboBox.setSelectedId(1, sendNotificationAsync);

    return true;

}

File NewTestSelectComponent::getSelectedTestFile() {
    const int index = testSelectComboBox.getSelectedItemIndex();
    if (index < 0 || index >= (int) specifications.size()) {
        return File();
    }
    return specifications[(size_t) index].file;
}
