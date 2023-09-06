//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef TESTLAUNCHER_H
#define TESTLAUNCHER_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "AudioPlayer.h"
#include "Trial.h"
#include "SurveyComponent.h"
#include "TestTypes.h"


void randomizeArrayOrder(Array<int> &anArray);


class TestLauncher : public ThreadWithProgressWindow {
public:
    TestLauncher(AudioPlayer &);

    ~TestLauncher();

    String lastError;

    Trial *getCurrentTrial();

    Trial *getTrialAtIndex(int ind);

    String getCurrentTrialName();

    int getCurrentTrialIndex() { return currentIndex; };

    int getNextStimulus();

    int getTrialsCount() { return trialsCount; };

    int getStimCount() { return stimCount; };

    testEnum getTestType() { return testType; };

    int getInputChannels() { return inputChannels; }

    int getPlayCount(int index);

    int getReferencePlayCount() { return getCurrentTrial()->refPlays; }

    String getResultsDir() { return resultsDirectory; }

    int64 getLengthInSamples() { return samplesCount; }

    String
    getSoundFileNameOnly(int i) { return getCurrentTrial()->soundFilesNamesOnly[getCurrentTrial()->filesOrder[i]]; }

    bool goToTrial(int trialIndex); // return FALSE if session complete; TRUE otherwise
    void setSubjectID(String newID) { subjectID = newID; };

    void setTestID(String newID) { testID = newID; };

    void setStimuliDirectory(String newDir) { stimuliDirectory = newDir; }
    
    void setSurveyResultsXml(XmlElement &surveyResultsXml) { this->surveyResultsXml = surveyResultsXml; }
    
    void clearSurveyResultsXml() { surveyResultsXml.removeAllAttributes(); surveyResultsXml.deleteAllChildElements(); }

    bool isTestComplete() { return testComplete; }

    /* initialise and test parameters */
    bool init(File testSettingsFile);

    /* save results of listening test -- TODO, these will move into individual test classes */
    bool loadResults(File &resultsFile);

    bool saveResults();

    void dbgOut(const String msg) { fileLogger->logMessage(msg); }

    void incrementPlayCount(int index);

    void incrementReferencePlayCount();

    void run();


private:
    void debugTrialSettings();

    AudioPlayer &audioPlayer;

    String subjectID;
    String testID;
    String stimuliDirectory;
    bool randomiseStimuli;
    OwnedArray <Trial> trials;
    int stimCount = 0;
    testEnum testType;
    int trialsCount;
    int trialsPerSession;
    int trialsThisSession;
    int currentIndex;
    bool testComplete;

    std::unique_ptr <FileLogger> fileLogger;
    Time testStartTime;

    unsigned int samplesCount;

    bool readTestSettings();

    String resultsDirectory;

    int inputChannels;
    
    XmlElement surveyResultsXml;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestLauncher);

};

#endif
