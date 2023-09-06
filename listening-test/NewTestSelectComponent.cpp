//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "NewTestSelectComponent.h"
#include "TestTypes.h" // for workingDirectory()

//==============================================================================
NewTestSelectComponent::NewTestSelectComponent() :
        buttonListener(*this) {
    addAndMakeVisible(&testSelectComboBox);
    testSelectComboBox.setEditableText(false);
    testSelectComboBox.setJustificationType(Justification::centredLeft);
    testSelectComboBox.setTextWhenNothingSelected(String());
    testSelectComboBox.setTextWhenNoChoicesAvailable("(empty)");

    addAndMakeVisible(&nameTextBox);
    nameTextBox.setTextToShowWhenEmpty("My name", Colours::lightgrey);

    addAndMakeVisible(&readyTextButton);
    readyTextButton.setButtonText("Ready!");
    readyTextButton.addListener(this);
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
    skipDemographicSurveyToggle.setToggleState(skipDemographicSurvey, dontSendNotification);
    skipDemographicSurveyToggle.addListener(&buttonListener);

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

void NewTestSelectComponent::buttonClicked(Button *b) {
    if (b == &readyTextButton) {
        if ((testSelectComboBox.getSelectedId() > 0) && (nameTextBox.getText().length() > 0)) {
            testID = testSelectComboBox.getText();
            testIdx = testSelectComboBox.getSelectedItemIndex();

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

            sendChangeMessage();

        }
    }
}


bool NewTestSelectComponent::populateComboBoxWithTestNames() {
    if (!workingDirectory.isDirectory()) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error",
                                    "Unable to create directory:\t\nQuitting now." + workingDirectory.getFullPathName(),
                                    "OK", this);
        JUCEApplication::getInstance()->quit();
    }

    Array <File> xmlFiles;
    workingDirectory.findChildFiles(xmlFiles, File::findFiles, false, "*-testspec.xml");
    if (xmlFiles.size() == 0)
        return false;

    testSelectComboBox.clear();
    for (int i = 0; i < xmlFiles.size(); i++) {
        std::unique_ptr <XmlElement> xml(parseXML(xmlFiles[i]));
        testSelectComboBox.addItem(xml->getStringAttribute("name"), i + 1);
    }
    testSelectComboBox.setSelectedId(1, sendNotificationAsync);

    return true;

}
