//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef MUSHRA_COMPONENT_H
#define MUSHRA_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "BaseTestComponent.h"

/**
	GUI Component for MUSHRA tests.
*/
class MushraComponent : public BaseTestComponent {
public:
    MushraComponent(AudioPlayer &aP, TestLauncher &tL);

    ~MushraComponent() {};

    void changeListenerCallback(ChangeBroadcaster *o);

    void buttonClicked(Button *b);

    void resized();

    void createButtons();


private:
    /* Buttons, sliders, labels for test stimuli */
    OwnedArray <Label> ratingLabels;

    bool saveResponses();

    bool readResponses();

    void clearResponses();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MushraComponent);
};


#endif   // MUSHRA_COMPONENT_H
