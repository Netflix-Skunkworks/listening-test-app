//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#ifndef TEST_TYPES_H
#define TEST_TYPES_H

#include "../JuceLibraryCode/JuceHeader.h"

typedef enum {
    TEST_TYPE_MUSHRA = 0,
    TEST_TYPE_MUSHRA_STRICT,
    TEST_TYPE_MUSHRA_DEMO,
    TEST_TYPE_BS1116,
    TEST_TYPE_AB,
    TEST_TYPE_AVAB,
    NUMBER_OF_TEST_TYPES
} testEnum;

// NO SPACES ALLOWED IN THESE NAMES!
const String testTypes[] = {
        "MUSHRA",
        "MUSHRA_Strict",
        "MUSHRA_Demo",
        "BS-1116",
        "AB_Forced_Choice",
        "AudioVideo_AB_Forced_Choice"
};// "Casual Listening"};

const testEnum getTestTypeEnum(String testTypeString);

const File workingDirectory(
        File::getSpecialLocation(File::userDocumentsDirectory).getFullPathName() + File::getSeparatorString() +
        "ListeningTest");
const File audioDeviceSettingsFile(workingDirectory.getChildFile("audioDeviceSettings.xml"));

#endif /* TEST_TYPES_H */
