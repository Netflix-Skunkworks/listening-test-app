// Copyright(C) 2026 Netflix, Inc.
// Listening Test Application — session state independent of windows and audio devices.
#pragma once

#include "TestTypes.h"
#include "Trial.h"

struct TestConfiguration {
    testEnum type = NUMBER_OF_TEST_TYPES;
    juce::File stimuliDirectory;
    int stimuliCount = 0; // Derived from files (or stored trials when resuming), never required in a spec.
    int trialsPerSession = -1;

    bool isPairwise() const { return type == TEST_TYPE_AB || type == TEST_TYPE_AVAB; }
};

struct TestPlan {
    TestConfiguration configuration;
    juce::OwnedArray<Trial> trials;
};

class TestSession {
public:
    enum class AdvanceOutcome { trialReady, sessionLimitReached, testComplete, invalidRequest };

    struct Progress {
        int trialIndex = 0;
        int completedThisSession = 0;
        bool complete = false;
    };

    struct Transition {
        AdvanceOutcome outcome;
        Progress progress;
    };

    TestSession(TestPlan&&, juce::String subjectID, juce::String testID);

    const TestPlan& getPlan() const { return plan; }
    const TestConfiguration& getConfiguration() const { return plan.configuration; }
    const Progress& getProgress() const { return progress; }
    Trial* getCurrentTrial() const { return getTrial(progress.trialIndex); }
    Trial* getTrial(int index) const;
    // Count of response-bearing stimuli; demo's visible reference is separate.
    int getStimCount() const;
    int getStimulusSourceIndex(int presentationIndex) const;

    // Preparing an advance does not change the session. The coordinator first
    // loads the target and saves its checkpoint, then commits the transition.
    Transition prepareAdvance(int trialIndex) const;
    void commitAdvance(const Transition&);
    void beginCurrentTrial();
    void finishCurrentTrial();

    int getSelectedStimulus() const { return selectedStimulus; }
    void selectStimulus(int sourceIndex);

    const juce::String& getSubjectID() const { return subjectID; }
    const juce::String& getTestID() const { return testID; }
    juce::Time getStartTime() const { return startTime; }
    // Opaque payload owned by the survey implementation, including custom fields.
    const juce::XmlElement* getSurveyResults() const { return surveyResults.get(); }
    void setSurveyResults(std::unique_ptr<juce::XmlElement> results) { surveyResults = std::move(results); }

private:
    friend class ResultsStore;

    TestPlan plan;
    Progress progress;
    juce::String subjectID, testID;
    juce::Time startTime = juce::Time::getCurrentTime();
    std::unique_ptr<juce::XmlElement> surveyResults;
    int selectedStimulus = -1;
    bool trialActive = false;
};
