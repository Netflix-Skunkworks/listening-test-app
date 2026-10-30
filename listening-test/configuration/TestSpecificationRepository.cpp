// Copyright(C) 2026 Netflix, Inc.
#include "TestSpecificationRepository.h"
using namespace juce;

Result TestSpecificationRepository::validateName(const String& name) {
    if (name.trim().isEmpty() || name != name.trim() || name == "." || name == ".."
        || File::createLegalFileName(name) != name)
        return Result::fail("Enter a test name without path separators or invalid filename characters.");
    return Result::ok();
}

Result TestSpecificationRepository::read(const File& file, TestSpecification& destination) {
    auto xml = parseXML(file);
    if (!xml || !(xml->hasTagName("test") || xml->hasTagName("testSettings")))
        return Result::fail("Could not read test settings in " + file.getFullPathName());
    TestSpecificationDraft draft;
    draft.name = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
    if (draft.name.isEmpty()) draft.name = file.getFileNameWithoutExtension();
    draft.type = xml->getStringAttribute("testType");
    draft.directory = xml->getStringAttribute("stimuliDirectory");
    draft.sessionLimit = xml->getStringAttribute("trialsPerSession", "all");
    if (draft.sessionLimit == "-1") draft.sessionLimit = "all"; // Legacy unlimited sentinel.
    // Historical stimuliCount is deliberately ignored, including stale values.
    auto result = draft.specification(destination);
    return result.failed() ? Result::fail(result.getErrorMessage() + " (" + file.getFileName() + ")") : result;
}

Result TestSpecificationRepository::discover(std::vector<StoredTestSpecification>& destination, StringArray& errors) const {
    errors.clear();
    if (!directory.isDirectory()) return Result::fail("Unable to find tests directory " + directory.getFullPathName());
    Array<File> files;
    directory.findChildFiles(files, File::findFiles, false, "*-testspec.xml");
    files.sort();
    std::vector<StoredTestSpecification> specs;
    for (const auto& file : files) {
        TestSpecification spec;
        const auto result = read(file, spec);
        if (result.failed()) errors.add(result.getErrorMessage());
        else specs.push_back({std::move(spec), file});
    }
    destination = std::move(specs);
    return Result::ok();
}

Result TestSpecificationRepository::write(const TestSpecification& spec, File& savedFile) const {
    auto result = validateName(spec.name);
    if (result.failed()) return result;
    TestMediaInventory inventory;
    result = inspectTestMedia(spec, inventory);
    if (result.failed()) return result;
    result = directory.createDirectory();
    if (result.failed()) return result;
    XmlElement xml("test");
    xml.setAttribute("name", spec.name);
    xml.setAttribute("testType", getTestTypeName(spec.type));
    xml.setAttribute("stimuliDirectory", spec.stimuliDirectory.getFullPathName());
    xml.setAttribute("trialsPerSession", spec.trialsPerSession == -1 ? "all" : String(spec.trialsPerSession));
    const auto file = fileFor(spec);
    TemporaryFile temporary(file);
    if (!xml.writeTo(temporary.getFile()) || !temporary.overwriteTargetFileWithTemporary())
        return Result::fail("Unable to save " + file.getFullPathName());
    savedFile = file;
    return Result::ok();
}
