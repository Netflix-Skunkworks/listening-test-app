//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef TEST_MANAGER_COMPONENT_H
#define TEST_MANAGER_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "TestTypes.h"


class TestManagerComponent : public Component {
public:
    TestManagerComponent();

    ~TestManagerComponent();

    void resized();

    void buttonClicked(Button *b);

    void refreshTestSpecs() {
        if (!workingDirectory.isDirectory()) {
            if (!workingDirectory.createDirectory()) {
                AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error",
                                            "Unable to create directory:\t\nQuitting now." +
                                            workingDirectory.getFullPathName(), "OK", this);
                JUCEApplication::getInstance()->quit();
            }
        }

        Array <File> settingsFiles;
        workingDirectory.findChildFiles(settingsFiles, File::findFiles, false, "*-testspec.xml");
        testManagerListBox.clearTestSpecsTable();
        testManagerListBox.addTestSpecsToTable(settingsFiles);
    }

private:
    class MyButtonListener : public Button::Listener {
    public:
        MyButtonListener(TestManagerComponent &c) : owner(c) {};

        ~MyButtonListener() {};

        void buttonClicked(Button *b) {
            if (b == &owner.addButton) {
                owner.testManagerListBox.addTest();
            }
            if (b == &owner.deleteButton) {
                owner.testManagerListBox.deleteTest();
            }
            if (b == &owner.revealTestsDirectoryButton) {
                workingDirectory.revealToUser();
            }
            if (b == &owner.okButton) {
                if (owner.testManagerListBox.validateData()) {
                    /* Save each test setup in a different xml file */
                    for (int i = 0; i < owner.testManagerListBox.getNumRows(); ++i) {
                        File outFile = workingDirectory.getChildFile(
                                owner.testManagerListBox.getRowData(i)->getStringAttribute("name") + "-testspec.xml");
                        if (!owner.testManagerListBox.getRowData(i)->writeTo(outFile))
                            AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error occurred",
                                                        "Could not save test spec", "OK", (Component * ) & owner);
                    }
                }
            }
        }

    private:
        TestManagerComponent &owner;
    } buttonListener;


    // Actual list box containing names of tests and their file locations
    class TestManagerListBox : public TableListBox,
                               public TableListBoxModel {
    public:

        TestManagerListBox();

        ~TestManagerListBox() {};

        // functions that do stuff
        void addTestSpecsToTable(Array <File> xmlFiles);

        void clearTestSpecsTable();

        void addTest();

        void deleteTest();

        bool validateData();

        // getters
        int getNumRows() { return rowsCount; };

        XmlElement *getRowData(int r) { return tableData->getChildElement(r); }

        // The following functions are mandatory implementations of virtual void functions in the base class, and they don't do anything.
        void paintRowBackground(Graphics &, int, int, int, bool) {};

        void paintCell(Graphics &, int, int, int, int, bool) {};

        void cellClicked(int row, int col, const MouseEvent &) { selectRow(row); }

        void listBoxitemClicked(int row, const MouseEvent &) { selectRow(row); }

        void cellDoubleClicked(int, int, const MouseEvent &) {};

        int backgroundClicked() { return 0; };

        void sortOrderChanged(int, bool) {};

        void selectedRowsChanged(int) {};

        void deleteKeyPressed(int) {};

        void returnKeyPressed(int) {};

        void listWasScrolled() {};

        var getDragSourceDescription(const SparseSet<int> &) { return {}; };

    private:

        enum ColumnNames {
            TEST_NAME = 1,
            STIMULI_DIRECTORY,
            TEST_TYPE,
            TRIALS_PER_SESSION
        };

        const int colCount = 4;
        int rowsCount;

        std::unique_ptr <XmlElement> tableData;

        void textEditorFocusLost(TextEditor &editor);

        void textEditorTextChanged(TextEditor &editor);

        Component *refreshComponentForCell(int r, int c, bool isRowSelected, Component *componentToUpdate);

        bool findCell(String cellName, int &row, int &col);

        String getAttributeNameForColumnId(ColumnNames col);

        String getStringAttributeForCell(int row, ColumnNames col);

        bool setStringAttributeForCell(int row, ColumnNames col, String newStr);

        class MyComboBoxListener : public ComboBox::Listener {
        public:
            MyComboBoxListener(TestManagerListBox &s) : owner(s) {};

            void comboBoxChanged(ComboBox *cB) {
                int r;
                ColumnNames c;

                if (owner.findCell(cB->getName(), r, (int &) c)) {
                    owner.setStringAttributeForCell(r, c, cB->getText());
                    owner.selectRow(r);
                }
            }

        private:
            TestManagerListBox &owner;

        } comboBoxListener;

        class MyTextEditorListener : public TextEditor::Listener {
        public:
            MyTextEditorListener(TestManagerListBox &s) : owner(s) {};

            void textEditorFocusLost(TextEditor &editor) {
                int r;
                ColumnNames c;

                if (owner.findCell(editor.getName(), r, (int &) c)) {
                    owner.setStringAttributeForCell(r, c, editor.getText());
                    owner.selectRow(r);
                }
            }

            void textEditorTextChanged(TextEditor &editor) {
                textEditorFocusLost(editor);
            }

        private:
            TestManagerListBox &owner;
        } textEditorListener;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TestManagerListBox);
    };

    TextButton addButton;
    TextButton deleteButton;
    TextButton revealTestsDirectoryButton;
    TextButton okButton;
    TestManagerListBox testManagerListBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestManagerComponent);
};


#endif   // TEST_MANAGER_COMPONENT_H
