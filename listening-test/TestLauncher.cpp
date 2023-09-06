//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "TestLauncher.h"

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))

TestLauncher::TestLauncher(AudioPlayer &aPlayer) :
        ThreadWithProgressWindow("Loading stimuli into memory ", true, false),
        audioPlayer(aPlayer),
        randomiseStimuli(true),
        trialsCount(-1),
        trialsPerSession(-1),
        trialsThisSession(0),
        currentIndex(-1),
        testComplete(false),
        fileLogger(FileLogger::createDefaultAppLogger("ListeningTest", "listening-test.log.txt", String(), 0)),
        testStartTime(Time::getCurrentTime()),
        resultsDirectory(String()),
        inputChannels(0),
        surveyResultsXml("surveySkipped") {}

TestLauncher::~TestLauncher() {
}

bool TestLauncher::init(File testSettingsFile) {
    std::unique_ptr <XmlElement> testSettings(parseXML(testSettingsFile));

    stimuliDirectory = testSettings->getStringAttribute("stimuliDirectory");
    stimCount = testSettings->getIntAttribute("stimuliCount");
    testType = getTestTypeEnum(testSettings->getStringAttribute("testType"));
    String trialsPerSessionString = testSettings->getStringAttribute("trialsPerSession");
    if (!trialsPerSessionString.equalsIgnoreCase("all")) {
        trialsPerSession = trialsPerSessionString.getIntValue();
    }
    dbgOut(String::formatted("%s test loaded", static_cast<const char *> (testTypes[testType].toUTF8())));
    randomiseStimuli = true;
    if (testType == TEST_TYPE_MUSHRA_DEMO) {
        randomiseStimuli = false;
        dbgOut("Demo mode; stimuli not randomized");
    }

    if (!readTestSettings()) {
        return false;
    }

    debugTrialSettings();

    /* load audio stimuli */
    runThread();

    if (!lastError.isEmpty()) {
        return false;
    }

    return true;
}

// ============================================================================================
Trial *TestLauncher::getCurrentTrial() {
    if ((currentIndex >= 0) && (currentIndex < trials.size()))
        return trials[currentIndex];
    else
        return NULL;
}

// ============================================================================================
Trial *TestLauncher::getTrialAtIndex(int ind) {
    if ((ind >= 0) && (ind < trials.size())) {
        return trials[ind];
    } else {
        return NULL;
    }
}

// ============================================================================================
String TestLauncher::getCurrentTrialName() {
    if (getCurrentTrial() != NULL)
        return getCurrentTrial()->testName;
    else
        return String();
}


// ============================================================================================
bool TestLauncher::goToTrial(int trialIndex) {
    trialsThisSession++;
    if (currentIndex >= trialsCount - 1) {
        testComplete = true;
    }

    if ((trialIndex >= 0) && (trialIndex < trials.size())) {
        trials[currentIndex]->setStopTime();
        currentIndex = trialIndex;

        if (trialsPerSession != -1 && trialsThisSession >= trialsPerSession) {
            return false;
        }

        dbgOut("\t Starting trial " + String(currentIndex) + "\t" + trials[currentIndex]->testName);
        runThread();
        trials[currentIndex]->setStartTime();
        trials[currentIndex]->setStopTime();
        return true;
    } else {
        return false;
    }
}


// ============================================================================================
bool TestLauncher::readTestSettings() {
    if (stimuliDirectory.isEmpty())
        return false;

    File *stimDir = new File(stimuliDirectory);
    if (!(stimDir->exists() && stimDir->isDirectory())) {
        lastError = "Unable to find stimuli directory " + stimuliDirectory;
        return false;
    }

    /* get the child directories */
    Array <File> childDir;
    int dirCount = stimDir->findChildFiles(childDir, File::findDirectories, true);
    if (dirCount == 0) {
        lastError = "Unable to find subdirectories in " + stimuliDirectory;
        return false;
    }

    childDir.sort();

    dbgOut(String::formatted("Loading files for %s test", static_cast<const char *> (testTypes[testType].toUTF8())));

    /* For BS-1116 testing, each stimuli (except for reference) in the stimuli directory is a trial */
    if (testType == TEST_TYPE_BS1116) {
        trialsCount = dirCount * (stimCount - 1);
    }
        /* For AB choice testing, all possible pairs in each directory are evaluated. */
    else if (testType == TEST_TYPE_AB || testType == TEST_TYPE_AVAB) {
        trialsCount = dirCount * (stimCount * (stimCount - 1) / 2);
    } else /* default is MUSHRA */
    {
        trialsCount = dirCount;
    }

    // Create the possibly randomized trial (i.e. folder) order
    Array<int> trialIndexLookup;
    for (int i = 0; i < trialsCount; i++) {
        trialIndexLookup.add(i);
    }

    if (randomiseStimuli) {
        randomizeArrayOrder(trialIndexLookup);

        dbgOut("Trial order");
        for (int i = 0; i < trialsCount; i++) {
            dbgOut("\tTrial " + String(i) + ": " + String(trialIndexLookup[i]));
        }
    }

    /* fill trials array */
    trials.clear();

    if (testType == TEST_TYPE_BS1116) {
        /* Each directory will have one trial for each coded file in the directory */
        /* Each trial needs a reference file and coded file */

        Array <File> codedFiles;
        Array <File> referenceFiles;

        for (int d = 0; d < dirCount; d++) {
            /* get list of all files in this directory */
            Array <File> filesFound;
            if (int stimFound = childDir[d].findChildFiles(filesFound, File::findFiles, false, "*.wav") != stimCount) {
                lastError = String(stimFound) + " stimuli were found in " + childDir[d].getFullPathName() + "; " +
                            String(stimCount) +
                            " were expected.  Try rerunning 'manage tests' if all files are where they should be.";
                return false;
            }

            for (int f = 0; f < filesFound.size(); f++) {
                if (filesFound[f].getFileName().containsIgnoreCase("REFERENCE")) {
                    for (int ff = 0; ff < filesFound.size() - 1; ff++) {
                        /* found reference file, add copies of it to the reference file array */
                        referenceFiles.add(filesFound[f]);
                    }
                } else {
                    codedFiles.add(filesFound[f]);
                }
            }
        }

        assert(referenceFiles.size() == codedFiles.size() && codedFiles.size() == trialsCount);
        for (int t = 0; t < trialsCount; t++) {
            String tmp = referenceFiles[trialIndexLookup[t]].getFileName() + " :: " +
                         codedFiles[trialIndexLookup[t]].getFileName();
            dbgOut(tmp);

            trials.add(new Trial);
            trials[t]->testName = referenceFiles[trialIndexLookup[t]].getParentDirectory().getFileName();
            trials[t]->refIndex = 0;
            trials[t]->soundFiles.add(referenceFiles[trialIndexLookup[t]].getFullPathName());
            trials[t]->soundFiles.add(codedFiles[trialIndexLookup[t]].getFullPathName());
            trials[t]->filesOrder.add(0);
            trials[t]->filesOrder.add(1);
            trials[t]->responses.add(5.0);
            trials[t]->responses.add(5.0);
            trials[t]->comments.add(String());
            trials[t]->comments.add(String());
            trials[t]->stimuliPlays.add(0);
            trials[t]->stimuliPlays.add(0);
            trials[t]->responsesMoved.add(false);
            trials[t]->responsesMoved.add(false);
            trials[t]->setStartTime();
            trials[t]->setStopTime();
            trials[t]->videoFile = new File(String());

            if (randomiseStimuli) {
                trials[t]->randomizeTrialOrder();
            }
        }
    } else if (testType == TEST_TYPE_AB || testType == TEST_TYPE_AVAB) {
        /* Each directory will have one trial per pair of stimuli */

        Array <File> firstFiles;
        Array <File> secondFiles;
        Array <File> videoFiles;

        for (int d = 0; d < dirCount; d++) {
            /* get list of all files in this directory */
            Array <File> filesFound;
            int nPairs = 0;
            int stimFound = childDir[d].findChildFiles(filesFound, File::findFiles, false, "*.wav");
            if (stimFound != stimCount) {
                lastError = String(stimFound) + " stimuli were found in " + childDir[d].getFullPathName() + "; " +
                            String(stimCount) +
                            " were expected.  Try rerunning 'manage tests' if all files are where they should be.";
                return false;
            }

            for (int f = 0; f < filesFound.size(); f++) {
                for (int ff = f + 1; ff < filesFound.size(); ff++) {
                    firstFiles.add(filesFound[f]);
                    secondFiles.add(filesFound[ff]);
                    nPairs++;
                }
            }

            if (testType == TEST_TYPE_AVAB) {
                filesFound.clear();
                int vidsFound = childDir[d].findChildFiles(filesFound, File::findFiles, false, "*.mp4");
                if (vidsFound != 1) {
                    lastError = String(vidsFound) + " video files were found in " + childDir[d].getFullPathName() +
                                "; 1 was expected.  I only search for *.mp4 files.  Try rerunning 'manage tests' if all files are where they should be.";
                    return false;
                }

                for (int i = 0; i < nPairs; i++) {
                    videoFiles.add(filesFound[0]);
                }
            }
        }

        assert(firstFiles.size() == trialsCount);
        for (int t = 0; t < trialsCount; t++) {
            String tmp = firstFiles[trialIndexLookup[t]].getFileName() + " :: " +
                         secondFiles[trialIndexLookup[t]].getFileName();
            dbgOut(tmp);

            trials.add(new Trial);
            trials[t]->testName = firstFiles[trialIndexLookup[t]].getParentDirectory().getFileName();
            trials[t]->refIndex = 0;
            trials[t]->soundFiles.add(firstFiles[trialIndexLookup[t]].getFullPathName());
            trials[t]->soundFiles.add(secondFiles[trialIndexLookup[t]].getFullPathName());
            trials[t]->filesOrder.add(0);
            trials[t]->filesOrder.add(1);
            trials[t]->responses.add(0);
            trials[t]->responses.add(1);
            trials[t]->comments.add(String());
            trials[t]->comments.add(String());
            trials[t]->stimuliPlays.add(0);
            trials[t]->stimuliPlays.add(0);
            trials[t]->responsesMoved.add(false);
            trials[t]->responsesMoved.add(false);
            trials[t]->setStartTime();
            trials[t]->setStopTime();
            trials[t]->videoFile = new File(String());

            if (testType == TEST_TYPE_AVAB) {
                *(trials[t]->videoFile) = videoFiles[trialIndexLookup[t]].getFullPathName();
            }

            if (randomiseStimuli) {
                trials[t]->randomizeTrialOrder();
            }
        }
    } else /* MUSHRA */
    {
        assert(testType == TEST_TYPE_MUSHRA ||
               testType == TEST_TYPE_MUSHRA_STRICT ||
               testType == TEST_TYPE_MUSHRA_DEMO);

        for (int i = 0; i < trialsCount; i++) {
            trials.add(new Trial);
            trials[i]->setStartTime();
            trials[i]->setStopTime();
            trials[i]->videoFile = new File(String());

            /* get stimuli files */
            Array <File> stimuliFiles;
            int stimFound = childDir[trialIndexLookup[i]].findChildFiles(stimuliFiles, File::findFiles, false, "*.wav");

            if (stimFound != stimCount) {
                lastError = "The number of stimuli found in " + childDir[trialIndexLookup[i]].getFullPathName()
                            + " is " + String(stimFound) + " while " + String(stimCount) + " stimuli is expected.";
                return false;
            }

            /* fill the fields of Trials */
            trials[i]->testName = childDir[trialIndexLookup[i]].getFileName();

            stimuliFiles.sort();
            int refIndex = -1;
            for (int j = 0; j < stimCount; j++) {
                if (stimuliFiles[j].getFileName().containsIgnoreCase("REFERENCE")) {
                    // Move this to the end of the array
                    File temp = stimuliFiles[j];
                    stimuliFiles.remove(j);
                    stimuliFiles.add(temp);
                    refIndex = stimCount - 1;
                    break;
                }
            }
            if (refIndex == -1) {
                lastError = "No reference stimulus is found in " + childDir[trialIndexLookup[i]].getFullPathName();
                return false;
            }

            trials[i]->refIndex = refIndex;
            for (int j = 0; j < stimCount; j++) {
                trials[i]->soundFiles.add(stimuliFiles[j].getFullPathName());
                trials[i]->soundFilesNamesOnly.add(stimuliFiles[j].getFileNameWithoutExtension());
                trials[i]->filesOrder.add(j);
                trials[i]->comments.add(String());
                if (!((j == refIndex) && (testType == TEST_TYPE_MUSHRA_DEMO))) {
                    trials[i]->responses.add(100);
                    trials[i]->stimuliPlays.add(0);
                    trials[i]->responsesMoved.add(false);
                } else {
                    dbgOut(String::formatted("Skipping %s for Demo mode",
                                             static_cast<const char *> (stimuliFiles[j].getFileNameWithoutExtension().toUTF8())));
                }
            }

            if (randomiseStimuli) {
                trials[i]->randomizeTrialOrder();
            }
        }

        for (int i = 0; i < trialsCount; i++) {
            dbgOut("Trial " + String(i) + " stimuli order");
            for (int j = 0; j < trials[i]->filesOrder.size(); j++) {
                dbgOut("\tButton " + String(j) + ": " + String(trials[i]->filesOrder[j]));
            }
        }
    }

    currentIndex = 0;

    if (stimDir) delete (stimDir);

    return true;
}

bool TestLauncher::loadResults(File &resultsFile) {
    std::unique_ptr <XmlElement> testResults(parseXML(resultsFile));
    if (testResults == nullptr) {
        lastError = "could not parse xml in " + resultsFile.getFileName();
        dbgOut(lastError);
        return false;
    }

    testType = getTestTypeEnum(testResults->getTagName());

    XmlElement *testInfo(testResults->getChildByName("info"));
    if (testInfo == nullptr) {
        lastError = "could not find test info when loading " + resultsFile.getFileName();
        dbgOut(lastError);
        return false;
    }

    testStartTime.fromISO8601(testInfo->getStringAttribute("startTime", String()));

    subjectID = testInfo->getStringAttribute("subjectName", String());
    if (subjectID.isEmpty()) {
        lastError = "No subject ID found in " + resultsFile.getFileName();
        dbgOut(lastError);
        return false;
    }
    testID = testInfo->getStringAttribute("testName", String());
    if (testID.isEmpty()) {
        lastError = "No test ID found in " + resultsFile.getFileName();
        dbgOut(lastError);
        return false;
    }

    stimuliDirectory = testInfo->getStringAttribute("stimuliDirectory", String());
    if (stimuliDirectory.isEmpty()) {
        lastError = "No stimuli directory found in " + resultsFile.getFileName();
        dbgOut(lastError);
        return false;
    }

    if (testInfo->getStringAttribute("testStatus", String()) != "complete") {
        currentIndex = testInfo->getIntAttribute("currentTrial");
    }

    String trialsPerSessionString = testInfo->getStringAttribute("trialsPerSession");
    if (trialsPerSessionString.isEmpty()) {
        lastError = "trials per session not found in " + resultsFile.getFileName() + ", assuming no limit.";
        dbgOut(lastError);
        trialsPerSessionString = "all";
    }

    if (trialsPerSessionString.equalsIgnoreCase("all")) {
        trialsPerSession = -1;
    } else {
        trialsPerSession = trialsPerSessionString.getIntValue();
    }
    
    if (testResults->getChildByName("surveyResults") != nullptr) {
        surveyResultsXml = XmlElement(*testResults->getChildByName("surveyResults"));
    }

    XmlElement *trialsInfo(testResults->getChildByName("trials"));
    if (trialsInfo == nullptr) {
        lastError = "could not find trials info when loading " + resultsFile.getFileName();
        dbgOut(lastError);
        return false;
    }

    trials.clear();
    trialsCount = trialsInfo->getNumChildElements();
    if (trialsCount < 1) {
        lastError = "Expected to find at least one trial, instead found " + String(trialsCount) + " in " +
                    resultsFile.getFileName();
        dbgOut(lastError);
        return false;
    }

    RelativeTime elapsedTime(0);
    Time workingTime(testStartTime);
    for (int i = 0; i < trialsCount; i++) {
        trials.add(new Trial);
        XmlElement trialInfo(*trialsInfo->getChildElement(i));
        if (!trials[i]->loadResults(trialInfo, stimuliDirectory)) {
            lastError = "error while parsing xml for trial " + String(i) + " in " + resultsFile.getFileName();
            dbgOut(lastError);
            return false;
        }

        // when a trial is loaded its start time is 0 and stop time is loaded relative to that
        elapsedTime = trials[i]->getStopTime() - trials[i]->getStartTime();
        trials[i]->setStartTime(workingTime);
        workingTime += elapsedTime;
        trials[i]->setStopTime(workingTime);

        if ((i > 0) && trials[i]->soundFiles.size() != trials[0]->soundFiles.size()) {
            lastError = "found " + String(trials[i]->soundFiles.size()) + " files in trial " + String(i) +
                        ", expected all trials to have " + String(trials[0]->soundFiles.size()) + " files. (" +
                        resultsFile.getFileName() + ")";
            dbgOut(lastError);
            return false;
        }

        // all trials before current index are complete so we assume that responses moved
        for (int j = 0; j < trials[i]->soundFiles.size(); j++) {
            trials[i]->responsesMoved.set(j, i < currentIndex);
        }

        if (testType == TEST_TYPE_MUSHRA_DEMO) {
            trials[i]->responses.remove(trials[i]->refIndex);
            trials[i]->stimuliPlays.remove(trials[i]->refIndex);
            trials[i]->responsesMoved.remove(trials[i]->refIndex);
        }
    }

    if (testType == TEST_TYPE_BS1116 || testType == TEST_TYPE_AB || testType == TEST_TYPE_AVAB) {
        stimCount = 2;
    } else {
        stimCount = trials[0]->soundFiles.size();
    }

    goToTrial(currentIndex);
    trialsThisSession = 0;
    
    runThread();
    return lastError.isEmpty();
}

bool TestLauncher::saveResults() {
    String resultFile;
    if (isTestComplete()) {
        String tmpString = testID + "_" + Time::getCurrentTime().formatted("%Y-%m-%d_%H%M%S_") + subjectID + ".xml";
        resultFile = File::addTrailingSeparator(stimuliDirectory) + tmpString.replaceCharacter(' ', '_');
    } else {
        resultFile = File::addTrailingSeparator(stimuliDirectory) + "temp_" + subjectID + ".xml";
    }

    File fs(resultFile);
    if (fs == File()) {
        lastError = "Unable to create output file " + resultFile;
        dbgOut(lastError);
        return false;
    }

    if (fs.existsAsFile()) {
        dbgOut("Deleting existing file:\t" + resultFile);
        fs.deleteFile();
    } else {
        Result resCreate = fs.create();
        if (resCreate.failed()) {
            dbgOut("Unable to create directory:\t" + fs.getFullPathName() + ".Error message: " +
                   resCreate.getErrorMessage());

            File docFolder(File::getSpecialLocation(File::userDocumentsDirectory));
            File mFolder(docFolder.getChildFile("ListeningTest"));

            if (!mFolder.isDirectory()) {
                Result res = mFolder.createDirectory();
                if (res.failed()) {
                    lastError = res.getErrorMessage();
                    dbgOut(lastError);
                    return false;
                }
            }

            fs = mFolder.getChildFile(fs.getFileName());
            resultsDirectory = fs.getParentDirectory().getFullPathName();
        }
    }

    XmlElement testResults(testTypes[testType]);

    XmlElement testInfo("info");
    testInfo.setAttribute("startTime", testStartTime.toISO8601(true));
    testInfo.setAttribute("stopTime", Time::getCurrentTime().toISO8601(true));
    testInfo.setAttribute("subjectName", subjectID);
    testInfo.setAttribute("testName", testID);
    testInfo.setAttribute("stimuliDirectory", stimuliDirectory);
    if (trialsPerSession == -1) {
        testInfo.setAttribute("trialsPerSession", "all");
    } else {
        testInfo.setAttribute("trialsPerSession", trialsPerSession);
    }
    testInfo.setAttribute("testStatus", isTestComplete() ? "complete" : "incomplete");
    if (!isTestComplete()) {
        testInfo.setAttribute("currentTrial", getCurrentTrialIndex());
    }

    testResults.addChildElement(new XmlElement(testInfo));
    
    if (surveyResultsXml.getTagName() != "surveySkipped") {
        testResults.addChildElement(new XmlElement(surveyResultsXml));
    }

    XmlElement trialsXml("trials");
    for (int i = 0; i < trialsCount; i++) {
        trials[i]->saveResults(&trialsXml);
    }

    testResults.addChildElement(new XmlElement(trialsXml));

    testResults.writeTo(fs);

    return true;
}

void TestLauncher::run() {
    audioPlayer.releaseAllAudioData();
    lastError = String();

    Array <int64> samplesCountPerFile;
    WavAudioFormat waf;

    // Get the channel count and validate consistency
    // First get sample count and use shortest input file as length for all
    for (int i = 0; i < getCurrentTrial()->soundFiles.size(); i++) {
        /* Create input stream for the audio file */
        FileInputStream *af = new FileInputStream(File(getCurrentTrial()->soundFiles[i]));

        if (af->failedToOpen()) {
            lastError = String::formatted("Could not open file %s.",
                                          getCurrentTrial()->soundFiles[i].toWideCharPointer());
            return;
        }

        std::unique_ptr <AudioFormatReader> wavReader(waf.createReaderFor(af, false));
        samplesCountPerFile.add(wavReader->lengthInSamples);

        if (i == 0) {
            inputChannels = wavReader->numChannels;
        } else if (wavReader->numChannels != inputChannels) {
            lastError = String::formatted("The number of audio channels in %s is %i; expected %i",
                                          getCurrentTrial()->soundFiles[i].toWideCharPointer(),
                                          inputChannels,
                                          audioPlayer.getChannelCount());
            return;
        }
    }

    int64 maxSamplesCount = 0;
    samplesCount = static_cast<unsigned int> (samplesCountPerFile[0]);
    for (int i = 1; i < samplesCountPerFile.size(); i++) {
        maxSamplesCount = min(maxSamplesCount, samplesCountPerFile[i]);
        if (maxSamplesCount > (int64)(UINT_MAX)) {
            lastError = String::formatted("Sorry, I cannot open %s because it has more than %n samples per channel",
                                          getCurrentTrial()->soundFiles[i].toUTF8(), UINT_MAX);
            return;
        }
    }

    if ((BigInteger) inputChannels > audioPlayer.getOutputChannels()) {
        lastError = "The number of audio channels in stimuli files exceeds available device outputs.";
        return;
    }

    audioPlayer.setTotalSamples(samplesCount);
    audioPlayer.setChannelCount(inputChannels);

    // Load input files into memory
    for (int i = 0; i < getCurrentTrial()->soundFiles.size(); i++) {
        setProgress((float) i / getCurrentTrial()->soundFiles.size());
        if (threadShouldExit())
            break;

        /* Create input stream for the audio file */
        FileInputStream *af = new FileInputStream(getCurrentTrial()->soundFiles[i]);

        if (af->failedToOpen()) {
            lastError = String::formatted("Could not open file %s.",
                                          getCurrentTrial()->soundFiles[i].toWideCharPointer());
            return;
        }

        std::unique_ptr <AudioFormatReader> wavReader(waf.createReaderFor(af, false));
        if (wavReader == NULL) {
            lastError = "Unable to create reader for  " + getCurrentTrial()->soundFiles[i];
            return;
        }

        dbgOut("Loading file " + af->getFile().getFullPathName());
        audioPlayer.addAudioFromFile(wavReader.get(), inputChannels, samplesCount);
    }

    if (getCurrentTrial()->videoFile->exists()) {
        dbgOut("Loading video file " + getCurrentTrial()->videoFile->getFullPathName());
        audioPlayer.setVideoFile(*getCurrentTrial()->videoFile);
    }

    setProgress(1.0);
    wait(500);
}

void TestLauncher::incrementPlayCount(int i) {
    assert(i >= 0 && i < getCurrentTrial()->stimuliPlays.size());
    getCurrentTrial()->stimuliPlays.set(i, getCurrentTrial()->stimuliPlays[i] + 1);
}

void TestLauncher::incrementReferencePlayCount() {
    getCurrentTrial()->refPlays++;
}

int TestLauncher::getPlayCount(int index) {
    if (index >= 0 && index < getCurrentTrial()->stimuliPlays.size())
        return getCurrentTrial()->stimuliPlays[index];
    else
        return -1;
}

void TestLauncher::debugTrialSettings() {
    String tmpStr = String();

    for (int i = 0; i < trials.size(); i++) {
        dbgOut("---  " + trials[i]->testName + "  ---");
        for (int k = 0; k < trials[i]->soundFiles.size(); k++) {
            tmpStr = String(trials[i]->filesOrder[k]) + "\t " + trials[i]->soundFiles[k];
            if (k == trials[i]->refIndex) {
                tmpStr += "\t-reference";
            }
            dbgOut(tmpStr);
        }
    }
}
