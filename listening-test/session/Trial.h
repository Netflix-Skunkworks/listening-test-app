//
//    Listening Test Application
//    Copyright(C) 2017-2026  Netflix, Inc.

#ifndef TRIAL_H
#define TRIAL_H

#include "TestTypes.h"
#include <vector>

void randomizeArrayOrder(juce::Array<int>& order);

// An identity is local to a trial and stays fixed when presentation order changes.
// Its value is also the source's slot in the decoded audio buffers.
using SourceId = int;
constexpr SourceId noSource = -1;

struct StimulusSource {
    SourceId id;
    juce::File file;
    juce::String displayName;
    bool reference = false;
};

struct TrialDefinition {
    juce::String name;
    // IDs are assigned in source (audio buffer) order; this order never changes.
    std::vector<StimulusSource> sources;
    // A permutation of all source IDs, including an unrated visible reference.
    juce::Array<SourceId> presentationOrder;
    juce::File videoFile;
};

struct StimulusResponse {
    SourceId source;
    double score;
    juce::String comment;
    int plays = 0;
    bool sliderMoved = false;
};

struct TrialResponses {
    // Records are keyed by source identity, independent of presentation order.
    std::vector<StimulusResponse> stimuli;
    int referencePlays = 0;
};

struct Methodology {
    double minimum, maximum, interval, defaultScore;
    bool requiresReference;
    bool ratesReference;
};

Methodology methodologyFor(testEnum type);

struct CompletionValidation {
    juce::Result status;
    bool needsConfirmation = false;
    juce::String confirmationMessage;
};

class Trial {
public:
    Trial(TrialDefinition definition, testEnum type);

    const TrialDefinition& getDefinition() const { return definition; }
    TrialResponses& getResponses() { return responses; }
    const TrialResponses& getResponses() const { return responses; }
    const StimulusSource* findSource(SourceId) const;
    SourceId getReferenceSource() const;
    int getPresentedStimulusCount() const;
    const StimulusSource* getPresentedSource(int presentationIndex) const;
    StimulusResponse* getPresentedResponse(int presentationIndex);
    const StimulusResponse* getPresentedResponse(int presentationIndex) const;
    StimulusResponse* findResponse(SourceId);
    const StimulusResponse* findResponse(SourceId) const;
    CompletionValidation validateCompletion() const;

    void setStopTime() { stopTime = juce::Time::getCurrentTime(); }
    void setStartTime() { startTime = juce::Time::getCurrentTime(); }
    void setStartTime(juce::Time t) { startTime = t; }
    void setStopTime(juce::Time t) { stopTime = t; }
    juce::Time getStartTime() const { return startTime; }
    juce::Time getStopTime() const { return stopTime; }

private:
    TrialDefinition definition;
    testEnum type;
    TrialResponses responses;
    juce::Time startTime { 0 }, stopTime { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Trial)
};

#endif /* TRIAL_H */
