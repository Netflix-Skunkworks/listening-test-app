//
//    Listening Test Application
//    Copyright(C) 2017  Netflix, Inc.


#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SurveyComponent  : public juce::Component,
                         public ChangeBroadcaster
{
public:
    SurveyComponent() : buttonListener(*this) {
        addAndMakeVisible(okButton);
        okButton.setButtonText("OK");
        okButton.addListener(&buttonListener);
        okButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
        
        setSize(800, 400);
    }

    virtual ~SurveyComponent() = 0;

    void paint (juce::Graphics& g) override {
        g.fillAll(Colours::darkgrey);
    }

    void resized() override
    {
        okButton.setSize(getWidth()/8, getHeight()/8);
        okButton.setCentrePosition(getWidth() / 2, getHeight() / 2);
    }
    
    virtual XmlElement getSurveyResultsXml() = 0;
    
    virtual void clearSurveyResponses() = 0;

protected:
    bool surveyCompleted;
    TextButton okButton;

    class MyButtonListener : public Button::Listener {
    public:
        MyButtonListener(SurveyComponent &c) : owner(c) {};

        ~MyButtonListener() {};

        void buttonClicked(Button *b) {
            if (b == &owner.okButton) {
                int dataProblems = owner.verifyData();
                if (dataProblems != 0) {
                    AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Problems",
                                                     String("There were " + String(dataProblems) +
                                                            " errors in the survey data, please fix them to proceed"));
                } else {
                    owner.sendChangeMessage();
                }
            }
        }

    private:
        SurveyComponent &owner;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyButtonListener);
    } buttonListener;
    
#define max(a,b) ((a)>(b)?(a):(b))

private:

    virtual int verifyData()= 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SurveyComponent)
};

