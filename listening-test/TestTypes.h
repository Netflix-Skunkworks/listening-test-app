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

const double MUSHRA_DEFAULT_SCORE = 100.0;   // top of the 0-100 scale
const double BS1116_DEFAULT_SCORE = 5.0;     // 'imperceptible' on the 1.0-5.0 scale
const double AB_DEFAULT_SCORE = 0.0;         // no preference expressed yet

/* Returns NUMBER_OF_TEST_TYPES for a string that names no known test, so every
   caller must check the result before using it as an index. */
testEnum getTestTypeEnum(String testTypeString);

bool isKnownTestType(testEnum testType);

/* Bounds-checked name lookup.  testTypes[testType] reads off the end of the
   array when the test type came from an unrecognised settings file or results
   tag, which is exactly the case the name is wanted for in error messages. */
String getTestTypeName(testEnum testType);

const File workingDirectory(
        File::getSpecialLocation(File::userDocumentsDirectory).getFullPathName() + File::getSeparatorString() +
        "ListeningTest");
const File audioDeviceSettingsFile(workingDirectory.getChildFile("audioDeviceSettings.xml"));

#endif /* TEST_TYPES_H */
