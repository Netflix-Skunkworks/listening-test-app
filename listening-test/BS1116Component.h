//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#ifndef BS1116_COMPONENT_H
#define BS1116_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "BaseTestComponent.h"

/**
	GUI Component for BS1116 tests.
*/
class BS1116Component : public BaseTestComponent {
public:
    BS1116Component(AudioPlayer &aP, TestLauncher &tL);

    ~BS1116Component() {};

    void buttonClicked(Button *b);

    void resized();

    void createButtons();


private:

    /* Buttons, sliders, labels for test stimuli */
    OwnedArray <Label> ratingLabels;

    bool saveResponses();

    bool readResponses();

    void clearResponses();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BS1116Component);
};


#endif   // BS1116_COMPONENT_H
