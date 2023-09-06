//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#ifndef AVAB_TEST_COMPONENT_H
#define AVAB_TEST_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "ABTestComponent.h"

class AVABTestComponent : public ABTestComponent {
public:
    AVABTestComponent(AudioPlayer &aP, TestLauncher &tL);

    ~AVABTestComponent() {};

    void resized();
    
private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AVABTestComponent);

};

#endif /* AVAB_TEST_COMPONENT_H */
