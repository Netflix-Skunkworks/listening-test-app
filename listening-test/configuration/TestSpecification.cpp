// Copyright(C) 2026 Netflix, Inc.
#include "TestSpecification.h"
#include <limits>
using namespace juce;

TestSpecificationDraft TestSpecificationDraft::from(const TestSpecification& spec) {
    TestSpecificationDraft draft;
    draft.name = spec.name;
    draft.directory = spec.stimuliDirectory.getFullPathName();
    draft.type = getTestTypeName(spec.type);
    draft.sessionLimit = spec.trialsPerSession == -1 ? "all" : String(spec.trialsPerSession);
    return draft;
}

Result TestSpecificationDraft::specification(TestSpecification& destination) const {
    TestSpecification spec;
    spec.name = name;
    spec.type = getTestTypeEnum(type);
    if (!isKnownTestType(spec.type)) return Result::fail("'" + type + "' is not a known test type.");
    if (directory.isEmpty() || !File::isAbsolutePath(directory))
        return Result::fail("Set an absolute stimuli directory for this test.");
    spec.stimuliDirectory = File(directory);
    const auto limit = sessionLimit.trim();
    if (!limit.equalsIgnoreCase("all")) {
        int value = 0;
        bool valid = limit.isNotEmpty();
        for (auto character : limit) {
            if (character < '0' || character > '9' || value > (std::numeric_limits<int>::max() - (character - '0')) / 10) {
                valid = false;
                break;
            }
            value = value * 10 + (character - '0');
        }
        if (!valid || value < 1)
            return Result::fail("Trials per session must be 'all' or a positive number.");
        spec.trialsPerSession = value;
    }
    destination = std::move(spec);
    return Result::ok();
}

bool TestSpecificationDraft::operator==(const TestSpecificationDraft& other) const {
    return name == other.name && directory == other.directory && type == other.type && sessionLimit == other.sessionLimit;
}

Result inspectTestMedia(const TestSpecification& spec, TestMediaInventory& destination) {
    if (!isKnownTestType(spec.type)) return Result::fail("Unknown test type.");
    if (spec.trialsPerSession != -1 && spec.trialsPerSession < 1)
        return Result::fail("Trials per session must be 'all' or a positive number.");
    if (!spec.stimuliDirectory.isDirectory())
        return Result::fail("Unable to find stimuli directory " + spec.stimuliDirectory.getFullPathName());
    Array<File> directories;
    spec.stimuliDirectory.findChildFiles(directories, File::findDirectories, true);
    directories.sort();
    if (directories.isEmpty()) return Result::fail("Unable to find subdirectories in " + spec.stimuliDirectory.getFullPathName());
    TestMediaInventory inventory;
    const bool pairwise = spec.type == TEST_TYPE_AB || spec.type == TEST_TYPE_AVAB;
    for (const auto& directory : directories) {
        TestMediaFolder folder;
        folder.directory = directory;
        directory.findChildFiles(folder.audio, File::findFiles, false, "*.wav");
        folder.audio.sort();
        const int count = folder.audio.size();
        if (count < 2) return Result::fail("At least two WAV stimuli are required in " + directory.getFullPathName());
        if (inventory.stimuliCount == 0) inventory.stimuliCount = count;
        if (count != inventory.stimuliCount)
            return Result::fail(String(count) + " stimuli were found in " + directory.getFullPathName()
                                + "; " + String(inventory.stimuliCount) + " were expected.");
        if (spec.type == TEST_TYPE_AVAB) {
            Array<File> videos;
            directory.findChildFiles(videos, File::findFiles, false, "*.mp4");
            if (videos.size() != 1) return Result::fail("Expected exactly one .mp4 video in " + directory.getFullPathName());
            folder.video = videos[0];
        }
        if (!pairwise) {
            for (int i = 0; i < count; ++i) {
                if (!folder.audio[i].getFileName().containsIgnoreCase("REFERENCE")) continue;
                if (folder.reference != -1)
                    return Result::fail("More than one reference stimulus was found in " + directory.getFullPathName());
                folder.reference = i;
            }
            if (folder.reference == -1) return Result::fail("No reference stimulus was found in " + directory.getFullPathName());
        }
        const int64 added = pairwise ? (int64) count * (count - 1) / 2 : spec.type == TEST_TYPE_BS1116 ? count - 1 : 1;
        if (added > std::numeric_limits<int>::max() - inventory.trialCount)
            return Result::fail("Too many trials in this test.");
        inventory.trialCount += (int) added;
        inventory.folders.push_back(std::move(folder));
    }
    if (spec.trialsPerSession > inventory.trialCount)
        return Result::fail("Trials per session must either be 'all' or a number between 1 and " + String(inventory.trialCount));
    destination = std::move(inventory);
    return Result::ok();
}
