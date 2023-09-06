//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#include "TestTypes.h"

const testEnum getTestTypeEnum(String testTypeString) {
    for (int i = 0; i < NUMBER_OF_TEST_TYPES; i++) {
        if (testTypes[i].equalsIgnoreCase(testTypeString)) {
            return static_cast<testEnum>(i);
        }
    }
    return NUMBER_OF_TEST_TYPES;
}
