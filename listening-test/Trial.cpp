//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#include "Trial.h"
#include <assert.h>
#include <cmath>

// Seed a global C++11 Mersenne Twister random number generator
std::random_device g_seed;
std::mt19937 g_rng(g_seed());

void randomizeArrayOrder(Array<int> &anArray) {
    int ind;
    Array<int> tempArray;
    int N = anArray.size();
    std::uniform_int_distribution<int> dist(0, N - 1);

    while (N-- > 1) // Stop looping when only one element remains
    {
        // Get a random number < (N + 1)
        // Note this is a better way to get a uniform distribiution
        // in a range than just using  rand() % (N + 1) and faster
        // than recreating a new uniform districution object each loop
        do {
            ind = dist(g_rng);
        } while (ind >= (N + 1));

        // Place selected item index in temp array and remove from old array
        tempArray.add(anArray[ind]);
        anArray.remove(ind);
    }

    // Add last element from old array to new array
    tempArray.add(anArray[0]);

    // Clear old array and replace with items now in random order from temp array
    anArray.clear();
    anArray.addArray(tempArray);
}


bool Trial::loadResults(XmlElement &trialXml, String &stimDirName) {
    File stimDir(stimDirName);
    if (!stimDir.isDirectory()) {
        return false;
    }

    testName = trialXml.getStringAttribute("trialName", String());
    if (testName == String()) return false;

    File trialDir = stimDir.getChildFile(testName);
    if (!trialDir.isDirectory()) {
        return false;
    }

    setStartTime(Time(0));
    stopTime = startTime;
    stopTime += RelativeTime(trialXml.getIntAttribute("trialSeconds", -1));
    if (stopTime < startTime) return false;

    refPlays = trialXml.getIntAttribute("referencePlays", -1);
    if (refPlays == -1) return false;

    filesOrder.clear();
    soundFiles.clear();
    stimuliPlays.clear();
    responses.clear();
    comments.clear();
    responsesMoved.clear();

    int numSoundFiles = trialXml.getNumChildElements();

    if (trialXml.getChildByName("videoFile") != nullptr) {
        // This code assumes that the video file is the last element in the list
        assert(trialXml.getChildElement(numSoundFiles - 1) == trialXml.getChildByName("videoFile"));
        numSoundFiles--;
        videoFile = trialDir.getChildFile(
                trialXml.getChildByName("videoFile")->getStringAttribute("fileName", String()));
    } else {
        videoFile = File();
    }

    for (int i = 0; i < numSoundFiles; i++) {
        filesOrder.add(i);  // files should have been randomized at test init; no need to re-randomize
        XmlElement fileInfo(*trialXml.getChildElement(i));
        File stimFilePath = trialDir.getChildFile(fileInfo.getStringAttribute("fileName", String()));
        if (!stimFilePath.existsAsFile()) {
            return false;
        }
        soundFiles.add(stimFilePath.getFullPathName());

        stimuliPlays.add(fileInfo.getIntAttribute("plays", -1));
        if (stimuliPlays[i] == -1) return false;

        responses.add(fileInfo.getDoubleAttribute("score", -1));
        if (responses[i] == -1) return false;
        
        comments.add(fileInfo.getStringAttribute("comment", String()));

        responsesMoved.add(false);

        if (soundFiles[i].containsIgnoreCase("REFERENCE")) {
            refIndex = i;
        }
    }

    return true;
}

void Trial::saveResults(XmlElement *parentXml) {
    RelativeTime elapsedTime = stopTime - startTime;

    XmlElement resultsXml("trial");
    resultsXml.setAttribute("trialName", testName);
    resultsXml.setAttribute("trialSeconds", round(elapsedTime.inSeconds()));
    resultsXml.setAttribute("referencePlays", refPlays);

    for (int i = 0; i < soundFiles.size(); i++) {
        String tmp = soundFiles[filesOrder[i]];
        int lastIndex = tmp.lastIndexOf(File::getSeparatorString());
        tmp = tmp.substring(lastIndex + 1);

        /* The finest slider step in any of the tests is 0.1, so round to three
           decimals to keep binary floating-point noise out of the results file. */
        double score = std::round(responses[i] * 1000.0) / 1000.0;

        XmlElement fileInfo("testFile");
        fileInfo.setAttribute("fileName", tmp);
        fileInfo.setAttribute("plays", stimuliPlays[i]);
        fileInfo.setAttribute("score", score);
        if (comments[i].isNotEmpty()) {
            fileInfo.setAttribute("comment", comments[i]);
        }
        resultsXml.addChildElement(new XmlElement(fileInfo));
    }

    if (videoFile.existsAsFile()) {
        XmlElement videoFileXml("videoFile");
        videoFileXml.setAttribute("fileName", videoFile.getFileName());
        resultsXml.addChildElement(new XmlElement(videoFileXml));
    }

    parentXml->addChildElement(new XmlElement(resultsXml));
}
