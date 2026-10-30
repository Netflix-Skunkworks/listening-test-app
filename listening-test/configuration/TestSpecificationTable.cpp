// Copyright(C) 2026 Netflix, Inc.
#include "TestSpecificationTable.h"
using namespace juce;

Result TestSpecificationTable::reload(StringArray& errors) {
    std::vector<StoredTestSpecification> specs;
    const auto result = repository.discover(specs, errors);
    if (result.failed()) return result;
    rows.clear();
    for (const auto& spec : specs) rows.push_back({TestSpecificationDraft::from(spec.specification), spec.file});
    saved = rows;
    pendingDeletes.clear();
    writtenFiles.clear();
    return Result::ok();
}

String TestSpecificationTable::text(int row, Column column) const {
    if (!isPositiveAndBelow(row, size())) return {};
    const auto& draft = rows[(size_t) row].draft;
    switch (column) {
        case Column::name: return draft.name;
        case Column::directory: return draft.directory;
        case Column::type: return draft.type;
        case Column::sessionLimit: return draft.sessionLimit;
    }
    return {};
}

bool TestSpecificationTable::setText(int row, Column column, String text) {
    if (!isPositiveAndBelow(row, size())) return false;
    auto& draft = rows[(size_t) row].draft;
    switch (column) {
        case Column::name: draft.name = text; break;
        case Column::directory: draft.directory = text; break;
        case Column::type: draft.type = text; break;
        case Column::sessionLimit: draft.sessionLimit = text; break;
    }
    return true;
}

void TestSpecificationTable::add() { rows.push_back({{}, {}}); }
void TestSpecificationTable::remove(int row) {
    if (!isPositiveAndBelow(row, size())) return;
    const auto& file = rows[(size_t) row].source;
    if (file != File()) pendingDeletes.addIfNotAlreadyThere(file);
    rows.erase(rows.begin() + row);
}

bool TestSpecificationTable::hasUnsavedChanges() const {
    if (!pendingDeletes.isEmpty() || !writtenFiles.isEmpty() || rows.size() != saved.size()) return true;
    for (size_t i = 0; i < rows.size(); ++i)
        if (!(rows[i].draft == saved[i].draft) || rows[i].source != saved[i].source) return true;
    return false;
}

Result TestSpecificationTable::save() {
    std::vector<TestSpecification> specs;
    Array<File> targets, knownFiles = pendingDeletes;
    for (const auto& row : rows) if (row.source != File()) knownFiles.addIfNotAlreadyThere(row.source);
    for (const auto& file : writtenFiles) knownFiles.addIfNotAlreadyThere(file);
    for (const auto& row : rows) {
        TestSpecification spec;
        auto result = row.draft.specification(spec);
        if (result.wasOk()) result = TestSpecificationRepository::validateName(spec.name);
        TestMediaInventory inventory;
        if (result.wasOk()) result = inspectTestMedia(spec, inventory);
        if (result.failed()) return Result::fail(row.draft.name + ": " + result.getErrorMessage());
        const auto target = repository.fileFor(spec);
        // Case-insensitive comparison also protects Windows/macOS file systems.
        for (const auto& existing : targets)
            if (existing.getFullPathName().equalsIgnoreCase(target.getFullPathName()))
                return Result::fail("Test names must be unique: " + spec.name);
        bool alreadyExists = false;
        for (const auto& known : knownFiles)
            if (known.getFullPathName().equalsIgnoreCase(target.getFullPathName())) { alreadyExists = true; break; }
        if (target.exists() && !alreadyExists)
            return Result::fail("A file already exists for " + spec.name + ". Reload Manage Tests before saving.");
        targets.add(target);
        specs.push_back(std::move(spec));
    }
    // Validate the whole table before writing any row. Failed writes retain all
    // edits; keep successful target identities so the same save can be retried.
    for (size_t i = 0; i < specs.size(); ++i) {
        File file;
        const auto result = repository.write(specs[i], file);
        if (result.failed()) return result;
        writtenFiles.addIfNotAlreadyThere(file);
    }
    Array<File> deletes = pendingDeletes;
    for (const auto& row : rows) if (row.source != File()) deletes.addIfNotAlreadyThere(row.source);
    for (const auto& file : writtenFiles) deletes.addIfNotAlreadyThere(file);
    for (const auto& file : deletes) {
        bool stillReferenced = false;
        for (const auto& kept : targets)
            if (kept.getFullPathName().equalsIgnoreCase(file.getFullPathName())) { stillReferenced = true; break; }
        if (!stillReferenced && file.exists() && !file.deleteFile())
            return Result::fail("Unable to delete " + file.getFullPathName());
    }
    for (size_t i = 0; i < rows.size(); ++i) rows[i].source = targets[(int) i];
    saved = rows;
    pendingDeletes.clear();
    writtenFiles.clear();
    return Result::ok();
}
