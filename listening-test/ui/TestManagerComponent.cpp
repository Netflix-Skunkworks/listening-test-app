//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017-2026  Netflix, Inc.

#include "TestManagerComponent.h"
#include "AppTheme.h"


const int border = 20;

//==============================================================================
TestManagerComponent::TestManagerComponent(const File& directory)
    : specsDirectory(directory), testManagerListBox(directory) {
    addAndMakeVisible(&okButton);
    okButton.setButtonText("Save Changes");
    okButton.onClick = [this] { if (testManagerListBox.saveTestSpecs() && onSaved) onSaved(); };
    okButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&addButton);
    addButton.setButtonText("Add Test");
    addButton.onClick = [this] { testManagerListBox.addTest(); };
    addButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&deleteButton);
    deleteButton.setButtonText("Delete Test");
    deleteButton.onClick = [this] { testManagerListBox.deleteTest(); };
    deleteButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&revealTestsDirectoryButton);
    revealTestsDirectoryButton.setButtonText("Reveal Tests Directory");
    revealTestsDirectoryButton.onClick = [this] { specsDirectory.revealToUser(); };
    revealTestsDirectoryButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&testManagerListBox);

}

TestManagerComponent::~TestManagerComponent() {
}

void TestManagerComponent::refreshTestSpecs() {
    if (!specsDirectory.isDirectory() && specsDirectory.createDirectory().failed()) {
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Error",
            "Unable to create tests directory: " + specsDirectory.getFullPathName(), "OK", this);
        return;
    }
    testManagerListBox.reload();
}

void TestManagerComponent::resized() {
    int y = getHeight() - border - AppTheme::buttonHeight;

    testManagerListBox.setBounds(border, border, getWidth() - 2 * border, getHeight() - 3 * border - AppTheme::buttonHeight);
    addButton.setBounds(border, y, AppTheme::buttonWidth, AppTheme::buttonHeight);
    deleteButton.setBounds(border * 2 + AppTheme::buttonWidth, y, AppTheme::buttonWidth, AppTheme::buttonHeight);
    revealTestsDirectoryButton.setBounds(border * 3 + AppTheme::buttonWidth * 2, y, AppTheme::buttonWidth * 2, AppTheme::buttonHeight);
    okButton.setBounds(getWidth() - border - AppTheme::buttonWidth, y, AppTheme::buttonWidth, AppTheme::buttonHeight);
}

const StringArray testTypeList(testTypes, NUMBER_OF_TEST_TYPES);

static String getTextEditorNameForCell(int r, int c) { return "textEditor" + String(r) + "_" + String(c); }

static String getComboBoxNameForCell(int r, int c) { return "comboBox" + String(r) + "_" + String(c); }

String TestManagerComponent::TestManagerListBox::getTextForCell(int row, ColumnNames col) {
    return data.text(row, static_cast<TestSpecificationTable::Column>(col));
}

bool TestManagerComponent::TestManagerListBox::setTextForCell(int row, ColumnNames col, String text) {
    return data.setText(row, static_cast<TestSpecificationTable::Column>(col), text);
}

TestManagerComponent::TestManagerListBox::TestManagerListBox(const File& directory) :
        data(directory) {
    setModel(this);
    setMultipleSelectionEnabled(false);
    getHeader().setStretchToFitActive(true);
    setSize(800, 580);  // have to set a size in order to add columns

    getHeader().addColumn("Test Name",
                          TEST_NAME,
                          (int) (0.20 * getWidth()),
                          50,
                          -1,
                          TableHeaderComponent::defaultFlags);

    getHeader().addColumn("Stimuli Directory",
                          STIMULI_DIRECTORY,
                          (int) (0.50 * getWidth()),
                          50,
                          -1,
                          TableHeaderComponent::defaultFlags);

    getHeader().addColumn("Test Type",
                          TEST_TYPE,
                          (int) (0.20 * getWidth()),
                          50,
                          -1,
                          TableHeaderComponent::defaultFlags);

    getHeader().addColumn("Trials per\nsession",
                          TRIALS_PER_SESSION,
                          (int) (0.10 * getWidth()),
                          50,
                          -1,
                          TableHeaderComponent::defaultFlags);

}

void TestManagerComponent::TestManagerListBox::reload() {
    StringArray errors;
    const auto result = data.reload(errors);
    if (result.failed()) errors.add(result.getErrorMessage());
    updateContent();
    if (!errors.isEmpty())
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Could not read test settings",
            errors.joinIntoString("\n"), "OK", this);
}

Component *TestManagerComponent::TestManagerListBox::refreshComponentForCell(int r, int c, bool /*isRowSelected*/,
                                                                             Component *existingComponentToUpdate) {
    // If an existing component is being passed-in for updating, we'll re-use it, but
    // if not, we'll have to create one.
    ColumnNames col = static_cast<ColumnNames>(c);

    switch (col) {
        case TEST_TYPE: {
            ComboBox *cB = (ComboBox *) existingComponentToUpdate;
            if (cB == NULL) {
                cB = new ComboBox(getComboBoxNameForCell(r, c));
                cB->addItemList(testTypeList, 1);
                cB->setBoundsInset(BorderSize<int>(2));
                cB->setColour(ComboBox::outlineColourId, AppTheme::transparent);
                addAndMakeVisible(cB);
            }
            // Loading/recycling a cell must not write a default back to the spec.
            // Keep unknown type names intact so validation can report them.
            cB->setName(getComboBoxNameForCell(r, c));
            cB->setText(getTextForCell(r, col), dontSendNotification);
            cB->onChange = [this, cB, r, col] {
                setTextForCell(r, col, cB->getText());
                selectRow(r);
            };
            return cB;
        }
        default: {
            TextEditor *textBox = (TextEditor *) existingComponentToUpdate;
            if (textBox == NULL) {
                textBox = new TextEditor(getTextEditorNameForCell(r, c));
                textBox->setBoundsInset(BorderSize<int>(2));
                textBox->setBorder(BorderSize<int>(0));
                textBox->setColour(TextEditor::outlineColourId, AppTheme::transparent);
                textBox->setColour(TextEditor::shadowColourId, AppTheme::transparent);
                addAndMakeVisible(textBox);
            }
            textBox->setName(getTextEditorNameForCell(r, c));
            textBox->setText(getTextForCell(r, col), false);
            textBox->onTextChange = textBox->onFocusLost = [this, textBox, r, col] {
                setTextForCell(r, col, textBox->getText());
                selectRow(r);
            };

            return textBox;
        }
    }
}

void TestManagerComponent::TestManagerListBox::addTest() {
    data.add();
    updateContent();

    TextEditor *t = (TextEditor *) getCellComponent(1, data.size() - 1);
    if (t && t->isShowing()) {
        t->grabKeyboardFocus();
    }
}

void TestManagerComponent::TestManagerListBox::deleteTest() {
    auto rows = getSelectedRows();
    if (rows.size() != 1) {
        AlertWindow::showMessageBox(AlertWindow::InfoIcon, "Select a test",
                                    rows.size() == 0 ? "Select the test you want to delete first."
                                                     : "Only one test can be deleted at a time.",
                                    "OK", this);
        return;
    }

    data.remove(rows[0]);
    deselectAllRows();
    updateContent();
}

bool TestManagerComponent::TestManagerListBox::saveTestSpecs() {
    const auto result = data.save();
    if (result.failed())
        AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Could not save tests",
            result.getErrorMessage(), "OK", this);
    return result.wasOk();
}
