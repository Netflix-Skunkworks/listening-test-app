// Copyright(C) 2026 Netflix, Inc.
#pragma once

#include "TestSession.h"
#include "../configuration/TestSpecification.h"
#include <functional>

using TrialShuffle = std::function<void(juce::Array<int>&)>;

// On failure the caller's existing plan is left intact. Tests can inject a
// deterministic shuffle without retaining a builder object.
juce::Result buildTrialPlan(const juce::File& settingsFile, TestPlan& destination,
                            const TrialShuffle& shuffle = randomizeArrayOrder);
juce::Result buildTrialPlan(const TestSpecification&, TestPlan& destination,
                            const TrialShuffle& shuffle = randomizeArrayOrder);
