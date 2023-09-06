//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.

#ifndef AB_TEST_COMPONENT_H
#define AB_TEST_COMPONENT_H

#include "../JuceLibraryCode/JuceHeader.h"
#include "BaseTestComponent.h"

class ABTestComponent : public BaseTestComponent {
public:
    ABTestComponent(AudioPlayer &aP, TestLauncher &tL);

    ~ABTestComponent() {};

    void buttonClicked(Button *b);

    void sliderValueChanged(Slider *s);

    bool keyPressed(const KeyPress &k);

    void resized();

    void createButtons();


protected:
    Slider preferenceSlider;
    Label preferenceLabel;
    Label buttonLabel;
    Label yourSelectionLabel;
    Label sliderLeftLabel;
    Label sliderRightLabel;
    Label commentsLabel;
    
    void initLabel(Label &label, String labelText, Justification j) {
        this->addAndMakeVisible(label);
        label.setText(labelText, dontSendNotification);
        label.setJustificationType(j);
        label.setEditable(false, false, false);
        label.setColour(TextEditor::textColourId, LINE_COLOUR);
    }

private:

    bool saveResponses();

    bool readResponses();

    void clearResponses();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ABTestComponent);
};

#endif /* AB_TEST_COMPONENT_H */
