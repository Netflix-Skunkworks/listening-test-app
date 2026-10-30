// Copyright(C) 2026 Netflix, Inc.
// Listening Test Application — adapts the session services to the application UI.
#pragma once

class AudioPlayer;
#include "../session/ResultsStore.h"
#include "../session/TrialBuilder.h"
#include "../media/TrialMediaLoader.h"

class TestLauncher {
public:
    struct AdvanceResult {
        TestSession::AdvanceOutcome outcome;
        juce::Result status;
    };

    // A host can supply loading without a native progress window (e.g. regression tests).
    using MediaLoader = std::function<juce::Result(const Trial&, LoadedTrialMedia&)>;
    explicit TestLauncher(AudioPlayer&, const juce::File& resultsDirectory,
                          MediaLoader = {});

    juce::String lastError;
    bool init(const juce::File& settingsFile);
    bool loadResults(const juce::File& resultsFile);
    bool saveResults();
    AdvanceResult goToTrial(int trialIndex);

    Trial* getCurrentTrial() const { return session ? session->getCurrentTrial() : nullptr; }
    int getCurrentTrialIndex() const { return session ? session->getProgress().trialIndex : -1; }
    int getTrialsCount() const { return session ? session->getPlan().trials.size() : 0; }
    int getStimCount() const { return session ? session->getStimCount() : 0; }
    testEnum getTestType() const { return session ? session->getConfiguration().type : NUMBER_OF_TEST_TYPES; }
    bool isTestComplete() const { return session && session->getProgress().complete; }
    juce::String getCurrentTrialName() const;
    juce::String getSoundFileNameOnly(int presentationIndex) const;
    int getStimulusSourceIndex(int presentationIndex) const {
        return session ? session->getStimulusSourceIndex(presentationIndex) : -1;
    }
    int getInputChannels() const { return inputChannels; }
    juce::int64 getLengthInSamples() const { return samplesCount; }

    void setSubjectID(juce::String value) { subjectID = std::move(value); }
    void setTestID(juce::String value) { testID = std::move(value); }
    void setSurveyResults(std::unique_ptr<juce::XmlElement> value) { surveyResults = std::move(value); }
    void clearSurveyResults() { surveyResults.reset(); }
    void startPlaybackDevice();
    void selectStimulus(int sourceIndex);
    void incrementPlayCount(int presentationIndex);
    void incrementReferencePlayCount();
    void dbgOut(const juce::String& message);

private:
    juce::Result loadMedia(const Trial&, LoadedTrialMedia&);
    bool activate(std::unique_ptr<TestSession>, bool saveCheckpoint);
    bool report(const juce::Result&);
    void installMedia(LoadedTrialMedia&);

    AudioPlayer& audioPlayer;
    std::unique_ptr<TestSession> session;
    MediaLoader loadMediaOverride;
    ResultsStore resultsStore;
    juce::File lastSavedFile;
    juce::String subjectID, testID;
    std::unique_ptr<juce::XmlElement> surveyResults;
    int inputChannels = 0, samplesCount = 0;
    std::unique_ptr<juce::FileLogger> fileLogger;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestLauncher)
};
