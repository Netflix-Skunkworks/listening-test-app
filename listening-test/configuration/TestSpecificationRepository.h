// Copyright(C) 2026 Netflix, Inc.
#pragma once
#include "TestSpecification.h"

struct StoredTestSpecification {
    TestSpecification specification;
    juce::File file;
};

// The XML/discovery boundary. Reads and inspection failures leave destinations
// intact. Discovery returns readable specs plus per-file errors, in file order.
// UI decides how to display errors; these services never open dialogs.
class TestSpecificationRepository {
public:
    explicit TestSpecificationRepository(juce::File directory) : directory(std::move(directory)) {}
    juce::Result discover(std::vector<StoredTestSpecification>& destination, juce::StringArray& errors) const;
    static juce::Result read(const juce::File&, TestSpecification& destination);
    juce::Result write(const TestSpecification&, juce::File& savedFile) const;
    juce::File fileFor(const TestSpecification& spec) const { return directory.getChildFile(spec.name + "-testspec.xml"); }
    static juce::Result validateName(const juce::String&);
private:
    juce::File directory;
};
