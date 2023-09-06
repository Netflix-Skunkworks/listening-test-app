//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "TestManagerComponent.h"
#include "guisettings.h"

const int border = 20;

//==============================================================================
TestManagerComponent::TestManagerComponent() : buttonListener(*this) {
    addAndMakeVisible(&okButton);
    okButton.setButtonText("Save Tests");
    okButton.addListener(&buttonListener);
    okButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&addButton);
    addButton.setButtonText("Add Test");
    addButton.addListener(&buttonListener);
    addButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&deleteButton);
    deleteButton.setButtonText("Delete Test");
    deleteButton.addListener(&buttonListener);
    deleteButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&revealTestsDirectoryButton);
    revealTestsDirectoryButton.setButtonText("Reveal Tests Directory");
    revealTestsDirectoryButton.addListener(&buttonListener);
    revealTestsDirectoryButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&testManagerListBox);

}

TestManagerComponent::~TestManagerComponent() {
}


void TestManagerComponent::resized() {
    int y = getHeight() - border - BUTTON_H;

    testManagerListBox.setBounds(border, border, getWidth() - 2 * border, getHeight() - 3 * border - BUTTON_H);
    addButton.setBounds(border, y, BUTTON_W, BUTTON_H);
    deleteButton.setBounds(border * 2 + BUTTON_W, y, BUTTON_W, BUTTON_H);
    revealTestsDirectoryButton.setBounds(border * 3 + BUTTON_W * 2, y, BUTTON_W * 2, BUTTON_H);
    okButton.setBounds(getWidth() - border - BUTTON_W, y, BUTTON_W, BUTTON_H);
}

const StringArray testTypeList(testTypes, NUMBER_OF_TEST_TYPES);

static String getTextEditorNameForCell(int r, int c) { return "textEditor" + String(r) + "_" + String(c); }

static String getComboBoxNameForCell(int r, int c) { return "comboBox" + String(r) + "_" + String(c); }

bool TestManagerComponent::TestManagerListBox::findCell(String cellName, int &row, int &col) {
    for (int r = 0; r < rowsCount; ++r) {
        for (int c = 1; c <= colCount; ++c) {
            if ((cellName == getTextEditorNameForCell(r, c)) ||
                (cellName == getComboBoxNameForCell(r, c))) {
                row = r;
                col = c;
                return true;
            }
        }
    }
    return false;
}


String TestManagerComponent::TestManagerListBox::getStringAttributeForCell(int row, ColumnNames col) {
    if (tableData->getChildElement(row)) {
        return tableData->getChildElement(row)->getStringAttribute(getAttributeNameForColumnId(col));
    } else {
        return String();
    }
}

bool TestManagerComponent::TestManagerListBox::setStringAttributeForCell(int row, ColumnNames col, String newStr) {
    String rowDataString = tableData->getChildElement(row)->toString();
    if (tableData->getChildElement(row)) {
        tableData->getChildElement(row)->setAttribute(getAttributeNameForColumnId(col), newStr);
        return true;
    } else {
        return false;
    }
}

TestManagerComponent::TestManagerListBox::TestManagerListBox() :
        rowsCount(0),
        tableData(new XmlElement("DATA")),
        comboBoxListener(*this),
        textEditorListener(*this) {
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

void TestManagerComponent::TestManagerListBox::clearTestSpecsTable() {
    tableData->deleteAllChildElements();
    rowsCount = 0;
    updateContent();

}

void TestManagerComponent::TestManagerListBox::addTestSpecsToTable(Array <File> xmlFiles) {
    rowsCount += xmlFiles.size();
    for (int i = 0; i < xmlFiles.size(); i++) {
        tableData->addChildElement(parseXML(xmlFiles[i]).release());
    }
    updateContent();
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
                cB->setSelectedId(1);
                cB->setBoundsInset(BorderSize<int>(2));
                cB->setColour(ComboBox::outlineColourId, transp);
                cB->addListener(&comboBoxListener);
                addAndMakeVisible(cB);
            } else {
                cB->setText(getStringAttributeForCell(r, col));
                if (cB->getSelectedId() == -1) {
                    String errorMsg = getStringAttributeForCell(r, TEST_NAME) +
                                      " settings contained unsupported test type.  using default type (" +
                                      testTypes[TEST_TYPE_MUSHRA] + ").";
                    AlertWindow::showMessageBox(AlertWindow::InfoIcon, "Warning", errorMsg, "OK", this);
                    cB->setSelectedId(1);
                }
            }
            return cB;
        }
        default: {
            TextEditor *textBox = (TextEditor *) existingComponentToUpdate;
            if (textBox == NULL) {
                textBox = new TextEditor(getTextEditorNameForCell(r, c));
                textBox->setText(String());

                textBox->setBoundsInset(BorderSize<int>(2));
                textBox->setBorder(BorderSize<int>(0));
                textBox->setColour(TextEditor::outlineColourId, transp);
                textBox->setColour(TextEditor::shadowColourId, transp);
                textBox->addListener(&textEditorListener);
                addAndMakeVisible(textBox);
            } else {
                textBox->setText(getStringAttributeForCell(r, col));
            }

            return textBox;
        }
    }
}

String TestManagerComponent::TestManagerListBox::getAttributeNameForColumnId(ColumnNames col) {
    switch (col) {
        case TEST_NAME:
            return "name";
            break;
        case STIMULI_DIRECTORY:
            return "stimuliDirectory";
            break;
        case TEST_TYPE:
            return "testType";
            break;
        case TRIALS_PER_SESSION:
            return "trialsPerSession";
            break;
        default:
            return String();
    }
}

void TestManagerComponent::TestManagerListBox::addTest() {
    XmlElement e("test");
    e.setAttribute(getAttributeNameForColumnId(TEST_NAME), "insert test name");
    e.setAttribute(getAttributeNameForColumnId(STIMULI_DIRECTORY), "full path");
    e.setAttribute(getAttributeNameForColumnId(TEST_TYPE), testTypes[TEST_TYPE_MUSHRA]);
    e.setAttribute(getAttributeNameForColumnId(TRIALS_PER_SESSION), "all");
    tableData->addChildElement(new XmlElement(e));
    rowsCount = tableData->getNumChildElements();
    updateContent();

    TextEditor *t = (TextEditor *) getCellComponent(1, rowsCount - 1);
    if (t) {
        t->grabKeyboardFocus();
    }
}

void TestManagerComponent::TestManagerListBox::deleteTest() {
    auto rows = getSelectedRows();
    assert(rows.size() == 1);  // multiple row selection & deletion not supported!

    File f = workingDirectory.getChildFile(getRowData(rows[0])->getStringAttribute("name") + "-testspec.xml");

    f.deleteFile();

    tableData->removeChildElement(getRowData(rows[0]), true);
    rowsCount--;
    updateContent();
}

bool TestManagerComponent::TestManagerListBox::validateData() {
    for (int i = 0; i < rowsCount; ++i) {
        String errStr(0);
        String testName = tableData->getChildElement(i)->getStringAttribute("name");
        File stimDir(tableData->getChildElement(i)->getStringAttribute("stimuliDirectory"));
        if (stimDir.isDirectory()) {
            Array <File> tmpDir;
            int trialsFound = stimDir.findChildFiles(tmpDir, File::findDirectories, false);
            int stimInFirstTrial = tmpDir[0].getNumberOfChildFiles(File::findFiles, "*.wav");
            for (int j = 1; j < trialsFound; ++j) {
                int stimFound = tmpDir[j].getNumberOfChildFiles(File::findFiles, "*.wav");
                if (stimFound != stimInFirstTrial) {
                    errStr = String("The number of stimuli in different trials were not the same.\n" \
                                  "Found " + String(stimFound) + " stimuli in directory \"" + tmpDir[j].getFileName() +
                                    "\"; expected " + String(stimInFirstTrial));
                }
            }

            tableData->getChildElement(i)->setAttribute("stimuliCount", stimInFirstTrial);
            if (stimInFirstTrial == 0) {
                errStr = String("No stimuli files were found in the specified directory");
            }

            String trialsPerSession = getStringAttributeForCell(i, TRIALS_PER_SESSION);
            if (!trialsPerSession.equalsIgnoreCase("all")) {
                int totalTrials = trialsFound;
                String testTypeString = tableData->getChildElement(i)->getStringAttribute("testType");

                if (testTypeString == testTypes[TEST_TYPE_BS1116]) {
                    totalTrials *= stimInFirstTrial - 1;
                } else if (testTypeString == testTypes[TEST_TYPE_AB] || testTypeString == testTypes[TEST_TYPE_AVAB]) {
                    totalTrials *= pow(2, stimInFirstTrial - 1) - 1;
                }
                if (trialsPerSession.getIntValue() > totalTrials || trialsPerSession.getIntValue() < 1) {
                    errStr = String("Trials per session must either be \'all\' or a number between 1 and " +
                                    String(totalTrials));
                }
            }
        } else {
            errStr = String("Stimuli directory not found");
        }

        if (errStr != String(0)) {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error parsing " + testName, errStr, "OK", this);
            return false;
        }
    }
    return true;
}


