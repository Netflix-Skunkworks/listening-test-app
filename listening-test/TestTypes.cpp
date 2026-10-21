//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#include "TestTypes.h"

/* Guards against an enumerator being added without a matching name, which would
   make every lookup below read past the end of the array. */
static_assert(sizeof(testTypes) / sizeof(testTypes[0]) == NUMBER_OF_TEST_TYPES,
              "testTypes must hold one name for each entry in testEnum");

testEnum getTestTypeEnum(String testTypeString) {
    for (int i = 0; i < NUMBER_OF_TEST_TYPES; i++) {
        if (testTypes[i].equalsIgnoreCase(testTypeString)) {
            return static_cast<testEnum>(i);
        }
    }
    return NUMBER_OF_TEST_TYPES;
}

bool isKnownTestType(testEnum testType) {
    return testType >= 0 && testType < NUMBER_OF_TEST_TYPES;
}

String getTestTypeName(testEnum testType) {
    if (!isKnownTestType(testType)) {
        return "unknown test type";
    }
    return testTypes[testType];
}
