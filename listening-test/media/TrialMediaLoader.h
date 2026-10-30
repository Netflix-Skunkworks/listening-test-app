// Copyright(C) 2026 Netflix, Inc.
#pragma once

#include "../session/Trial.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>

struct LoadedTrialMedia {
    juce::OwnedArray<juce::AudioBuffer<float>> audio;
    int channels = 0;
    int samples = 0;
    juce::File video;
};

// Return false from progress to cancel. The destination changes only when
// every file has been decoded successfully; this never touches an audio device.
juce::Result loadTrialMedia(const Trial&, int availableOutputs, LoadedTrialMedia& destination,
                            std::function<bool(double)> progress = {});
