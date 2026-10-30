// Copyright(C) 2026 Netflix, Inc.
#pragma once
#include "../session/TestTypes.h"
#include <vector>

// Configuration as authored; stimulus and trial counts are derived from media.
struct TestSpecification {
    juce::String name;
    testEnum type = TEST_TYPE_MUSHRA;
    juce::File stimuliDirectory;
    int trialsPerSession = -1;
};

// Editors retain incomplete/invalid input until validation, without XML state.
struct TestSpecificationDraft {
    juce::String name = "insert test name", directory = "full path";
    juce::String type = "MUSHRA", sessionLimit = "all";
    static TestSpecificationDraft from(const TestSpecification&);
    juce::Result specification(TestSpecification& destination) const;
    bool operator==(const TestSpecificationDraft&) const;
};

struct TestMediaFolder {
    juce::File directory, video;
    juce::Array<juce::File> audio;
    int reference = -1;
};
struct TestMediaInventory {
    std::vector<TestMediaFolder> folders;
    int stimuliCount = 0, trialCount = 0;
};

// Shared by repository writes and trial construction. No randomisation or UI.
juce::Result inspectTestMedia(const TestSpecification&, TestMediaInventory& destination);
