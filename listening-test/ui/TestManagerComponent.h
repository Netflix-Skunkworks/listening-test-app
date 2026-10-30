// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once

#include <JuceHeader.h>
#include "../session/TestTypes.h"
#include "../configuration/TestSpecificationTable.h"
#include <functional>

class TestManagerComponent : public Component {
public:
    explicit TestManagerComponent(const File& directory);
    ~TestManagerComponent() override;
    void resized() override;
    std::function<void()> onSaved;
    bool hasUnsavedChanges() const { return testManagerListBox.hasUnsavedChanges(); }
    void refreshTestSpecs();
private:
    const File specsDirectory;
    // The view adapts cells to a separately owned, device-free data model.
    class TestManagerListBox : public TableListBox, public TableListBoxModel {
    public:
        explicit TestManagerListBox(const File& directory);
        void reload();
        void addTest();
        void deleteTest();
        bool saveTestSpecs();
        bool hasUnsavedChanges() const { return data.hasUnsavedChanges(); }
        int getNumRows() override { return data.size(); }
        void paintRowBackground(Graphics&, int, int, int, bool) override {}
        void paintCell(Graphics&, int, int, int, int, bool) override {}
        void cellClicked(int row, int, const MouseEvent&) override { selectRow(row); }
    private:
        enum ColumnNames { TEST_NAME = 1, STIMULI_DIRECTORY, TEST_TYPE, TRIALS_PER_SESSION };
        TestSpecificationTable data;
        Component* refreshComponentForCell(int row, int column, bool selected, Component*) override;
        String getTextForCell(int row, ColumnNames column);
        bool setTextForCell(int row, ColumnNames column, String text);
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestManagerListBox)
    };
    TextButton addButton, deleteButton, revealTestsDirectoryButton, okButton;
    TestManagerListBox testManagerListBox;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestManagerComponent)
};
