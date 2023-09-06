//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#ifndef DEMO_COMPONENT_H
#define DEMO_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "BaseTestComponent.h"

/**
	GUI Component for Demo tests.
*/
class DemoComponent : public BaseTestComponent {
public:
    DemoComponent(AudioPlayer &aP, TestLauncher &tL);

    ~DemoComponent() {};

    void buttonClicked(Button *b);

    void resized();

    void createButtons();


private:
    TooltipWindow tooltipWindow;
    TextButton prevButton;

    /* Buttons, sliders, labels for test stimuli */
    OwnedArray <Label> ratingLabels;

    bool saveResponses();

    void readResponses();

    void clearResponses();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoComponent);
};


#endif   // DEMO_COMPONENT_H
