// Copyright(C) 2026 Netflix, Inc.
#pragma once

#include "TestSession.h"

class ResultsStore {
public:
    explicit ResultsStore(juce::File fallbackDirectory) : fallbackDirectory(std::move(fallbackDirectory)) {}

    // Failed reads leave the existing session intact. A pending transition lets
    // the coordinator persist the next checkpoint before committing it in memory.
    juce::Result load(const juce::File&, std::unique_ptr<TestSession>& destination) const;
    juce::Result save(const TestSession&, juce::File& savedFile,
                      const TestSession::Transition* pending = nullptr) const;

private:
    juce::File fallbackDirectory;
};
