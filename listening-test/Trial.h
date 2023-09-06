//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#ifndef TRIAL_H
#define TRIAL_H

#include "../JuceLibraryCode/JuceHeader.h"
#include <random>
#include <algorithm>

void randomizeArrayOrder(Array<int> &anArray);

class Trial {
public:
    Trial() : refIndex(-1), refPlays(0), startTime(0), stopTime(0), startVolume(0), stopVolume(0) {};

    ~Trial() {};

    void randomizeTrialOrder() { randomizeArrayOrder(this->filesOrder); }

    void setStopTime() { stopTime = Time::getCurrentTime(); }

    void setStartTime() { startTime = Time::getCurrentTime(); }

    void setStartTime(Time t) { startTime = t; }

    void setStopTime(Time t) { stopTime = t; }

    void setStartVolume(int aVolume) { startVolume = aVolume; }

    void setStopVolume(int aVolume) { stopVolume = aVolume; }

    Time getStartTime() { return startTime; }

    Time getStopTime() { return stopTime; }

    void saveResults(XmlElement *parentXml);

    bool loadResults(XmlElement &, String &stimDir);

    StringArray soundFiles;
    StringArray soundFilesNamesOnly;
    File *videoFile;
    Array<float> responses;
    Array<int> stimuliPlays;
    Array<String> comments;
    Array<int> filesOrder;
    int refIndex;
    int refPlays;
    Array<bool> responsesMoved;
    String testName;


private:
    Time startTime;
    Time stopTime;
    int startVolume;
    int stopVolume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Trial);

};

#endif /* Trial_h */
