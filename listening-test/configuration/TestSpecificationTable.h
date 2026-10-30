// Copyright(C) 2026 Netflix, Inc.
#pragma once
#include "TestSpecificationRepository.h"

// Editable table data independent of TableListBox, cells, and dialogs. File
// identities survive renames; deletion is deferred until save. Each file write
// is atomic. A failed batch keeps edits dirty and postpones all deletions.
class TestSpecificationTable {
public:
    enum class Column { name = 1, directory, type, sessionLimit };
    explicit TestSpecificationTable(juce::File directory) : repository(std::move(directory)) {}
    juce::Result reload(juce::StringArray& errors);
    int size() const { return (int) rows.size(); }
    juce::String text(int row, Column) const;
    bool setText(int row, Column, juce::String);
    void add();
    void remove(int row);
    bool hasUnsavedChanges() const;
    juce::Result save();
private:
    struct Row { TestSpecificationDraft draft; juce::File source; };
    std::vector<Row> rows, saved;
    juce::Array<juce::File> pendingDeletes, writtenFiles;
    TestSpecificationRepository repository;
};
