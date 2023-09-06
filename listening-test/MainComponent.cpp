//
//    Listening Test Application
//    Copyright(C) 2011  Roman Kosobrodov
//    Copyright(C) 2017  Netflix, Inc.

#include "MainComponent.h"

// FIXED: these values should not be scaled when window resizes
const int spaceBtwBorders = 64;
const int pbBorderH = 0.13 * DEFAULT_HEIGHT;

const int borderOffset = 16; // spacing between border outline and outer edge of test component

#ifdef CONCEAL_TRIAL_NAMES
static String getTestTrialText(TestLauncher& t) {
    return "   Trial " + String(t.getCurrentTrialIndex() + 1) + " of " + String(t.getTrialsCount()) + "   ";
}
#else
static String getTestTrialText(TestLauncher& t) {
    return "   Trial " + String(t.getCurrentTrialIndex() + 1) + " of " + String(t.getTrialsCount()) + ": " + t.getCurrentTrialName() + "   ";
}
#endif

const String hotkeysText =
"               space : pause/play\n"
"                   ` : play reference\n"
"                 1-9 : play hidden conditions\n"
"            up-arrow : increase score 1 (MUSHRA) 0.1 (BS.1116) point\n"
"          down-arrow : decrease score 1 point\n"
"             page-up : increase score 20 (MUSHRA) or 1 (BS.1116)\n"
"           page-down : decrease score 20 (MUSHRA) or 1 (BS.1116)\n"
"                   x : set loop start to cursor\n"
"                   c : set loop end to cursor\n"
"           shift + x : clear loop start\n"
"           shift + c : clear loop end\n"
"                home : restart playback\n"
"         right-arrow : advance loop to next segment\n"
"          left-arrow : retreat loop to previous segment";

//==============================================================================
MainComponent::MainComponent() :
        buttonListener(*this),
        sliderListener(*this),
        playbackSliderMin(0),
        playbackSliderMax(SLIDER_MAXVALUE),
        lockLoop(false),
        audioPlayer(audioDeviceSettingsFile),
        deviceSelector(audioPlayer.getAudioDeviceManager(), 0, 0, 0, MAXNUMBEROFDEVICECHANNELS, false, false, false, false),
        testLauncher(audioPlayer),
        testManagerComponent(),
        newTestSelectComponent(),
        demoComponent(audioPlayer, testLauncher),
        mushraComponent(audioPlayer, testLauncher),
        bs1116Component(audioPlayer, testLauncher),
        abComponent(audioPlayer, testLauncher),
        avabComponent(audioPlayer, testLauncher),
        testComponent(NULL) {
            
    setWantsKeyboardFocus(true);

    newTestSelectComponent.addChangeListener(this);
    addChildComponent(newTestSelectComponent);
            
    addChildComponent(testManagerComponent);
            
    surveyComponent.addChangeListener(this);
    addChildComponent(surveyComponent);

    // Create boxes for test controls & playback controls
    addAndMakeVisible(&testBorder);
    testBorder.setText("  Test Controls  ");
    testBorder.setTextLabelPosition(Justification::centred);

    addAndMakeVisible(&playbackBorder);
    playbackBorder.setText("Playback Controls");
    playbackBorder.setTextLabelPosition(Justification::centred);

    addAndMakeVisible(&playbackSlider);
    playbackSlider.setRange(0, SLIDER_MAXVALUE, 1);
    playbackSlider.setSliderStyle(Slider::ThreeValueHorizontal);
    playbackSlider.setTextBoxStyle(Slider::NoTextBox, false, 80, 20);
    playbackSlider.setMinValue(0, dontSendNotification);
    playbackSlider.setMaxValue(SLIDER_MAXVALUE, dontSendNotification);
    playbackSlider.setChangeNotificationOnlyOnRelease(false);
    playbackSlider.addListener(&sliderListener);

    /* Playback buttons, left to right */
    addAndMakeVisible(&rewindButton);
    rewindButton.setButtonText("|<<");
    rewindButton.setConnectedEdges(Button::ConnectedOnRight);
    rewindButton.addListener(&buttonListener);
    rewindButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));


    addAndMakeVisible(&backButton);
    backButton.setButtonText("<<");
    backButton.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
    backButton.addListener(&buttonListener);
    backButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));


    addAndMakeVisible(&playButton);
    playButton.setButtonText("|>");
    playButton.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
    playButton.addListener(&buttonListener);
    playButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));


    addAndMakeVisible(&forwardButton);
    forwardButton.setButtonText(">>");
    forwardButton.setConnectedEdges(Button::ConnectedOnLeft);
    forwardButton.addListener(&buttonListener);
    forwardButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));


    addAndMakeVisible(&loopToggleButton);
    loopToggleButton.setButtonText("loop");
    loopToggleButton.setToggleState(true, dontSendNotification);
    loopToggleButton.addListener(&buttonListener);
    loopToggleButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&lockLoopToggleButton);
    lockLoopToggleButton.setButtonText("lock loop interval");
    lockLoopToggleButton.setToggleState(lockLoop, dontSendNotification);
    lockLoopToggleButton.addListener(&buttonListener);
    lockLoopToggleButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&positionLabel);
    positionLabel.setText("00:00.0", dontSendNotification);
    positionLabel.setFont(Font(15.0000f, Font::plain));
    positionLabel.setJustificationType(Justification::centredLeft);
    positionLabel.setEditable(false, false, false);
    positionLabel.setColour(TextEditor::textColourId, Colours::black);
    positionLabel.setColour(TextEditor::backgroundColourId, Colour(0x0));
            
    addChildComponent(&hotkeyTextLabel);
    hotkeyTextLabel.setText(hotkeysText, dontSendNotification);
    hotkeyTextLabel.setFont(Font(Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));
    hotkeyTextLabel.setJustificationType(Justification::centredLeft);
    hotkeyTextLabel.setEditable(false, false, false);
    hotkeyTextLabel.setColour(TextEditor::textColourId, Colours::black);
    hotkeyTextLabel.setColour(TextEditor::backgroundColourId, Colour(0x0));

    addAndMakeVisible(&newTestButton);
    newTestButton.setButtonText("Start Test");
    newTestButton.addListener(&buttonListener);
    newTestButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
    newTestButton.setColour(TextButton::buttonOnColourId, activeButtonColour);

    addAndMakeVisible(&loadTestButton);
    loadTestButton.setButtonText("Load Test");
    loadTestButton.addListener(&buttonListener);
    loadTestButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));

    addAndMakeVisible(&audioSetupButton);
    audioSetupButton.setButtonText("Audio Setup");
    audioSetupButton.addListener(&buttonListener);
    audioSetupButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
    audioSetupButton.setColour(TextButton::buttonOnColourId, activeButtonColour);
    audioSetupButton.setToggleState(true, dontSendNotification);

    addAndMakeVisible(&manageTestsButton);
    manageTestsButton.setButtonText("Manage Tests");
    manageTestsButton.addListener(&buttonListener);
    manageTestsButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
    manageTestsButton.setColour(TextButton::buttonOnColourId, activeButtonColour);
            
    addAndMakeVisible(&showHotkeysButton);
    showHotkeysButton.setButtonText("Show hotkeys");
    showHotkeysButton.addListener(&buttonListener);
    showHotkeysButton.setColour(ComboBox::outlineColourId, juce::Colour(0.0f, 0.0f, 0.0f, 0.0f));
    showHotkeysButton.setColour(TextButton::buttonOnColourId, activeButtonColour);
            
    deviceSelector.addKeyListener(this);
    addAndMakeVisible(deviceSelector);

    audioPlayer.start();

    setSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
}


MainComponent::~MainComponent() {
    audioPlayer.stop();
}

//==============================================================================
void MainComponent::paint(Graphics &g) {
    g.fillAll(Colours::darkgrey);
}


void MainComponent::resized() {
    // Borders
    playbackBorder.setBounds(8, getHeight() - pbBorderH - 9, getWidth() - 16, pbBorderH);
    testBorder.setBounds(8, 9, getWidth() - 16, playbackBorder.getY() - spaceBtwBorders);
    
    Rectangle<int> childComponentBorders(testBorder.getX() + borderOffset,
                                         testBorder.getY() + borderOffset * 2, // extra space to allow for the text title in the border
                                         testBorder.getWidth() - borderOffset * 2,
                                         testBorder.getHeight() - borderOffset * 3);

    if (testComponent) {
        testComponent->setBounds(childComponentBorders);
    }
    
    deviceSelector.setBounds(childComponentBorders);
    hotkeyTextLabel.setBounds(childComponentBorders);
    testManagerComponent.setBounds(childComponentBorders);
    newTestSelectComponent.setBounds(childComponentBorders);
    surveyComponent.setBounds(childComponentBorders);

    // main buttons from right to left
    int rightmostButtonX = testBorder.getRight() - BUTTON_W;
    audioSetupButton.setBounds(rightmostButtonX, testBorder.getBottom() + spaceBtwBorders / 2 - 16, BUTTON_W, BUTTON_H);
    newTestButton.setBounds(rightmostButtonX - BUTTON_W * 1.1, audioSetupButton.getY(), BUTTON_W, BUTTON_H);
    loadTestButton.setBounds(rightmostButtonX - BUTTON_W * 2.2, audioSetupButton.getY(), BUTTON_W, BUTTON_H);
    manageTestsButton.setBounds(rightmostButtonX - BUTTON_W * 3.3, audioSetupButton.getY(), BUTTON_W, BUTTON_H);
    showHotkeysButton.setBounds(rightmostButtonX - BUTTON_W * 4.4, audioSetupButton.getY(), BUTTON_W, BUTTON_H);

    // Playback controls
    // slider row
    int playbackSliderWidth = testBorder.getWidth() - 200;
    loopToggleButton.setBounds(testBorder.getX() + borderOffset, playbackBorder.getY() + 20, 55, 25);
    playbackSlider.setBounds(100, playbackBorder.getY() + 20, playbackSliderWidth, 25);
    positionLabel.setBounds(playbackSlider.getRight() + 20, playbackBorder.getY() + 20, 80, 25);

    // shuttle controls row
    lockLoopToggleButton.setBounds(testBorder.getX() + borderOffset, playbackBorder.getBottom() - BUTTON_H, 150, 25);

    // align buttons in center of playback slider without overlapping the loop lock toggle button
    int x = max(playbackSlider.getWidth()/2 + playbackSlider.getX() - 200, lockLoopToggleButton.getRight());
    int buttonWidth = min(100, (playbackSlider.getRight()-x)/4);

    rewindButton.setBounds(x, playbackBorder.getBottom() - BUTTON_H, buttonWidth, 25);
    backButton.setBounds(rewindButton.getRight(), rewindButton.getY(), buttonWidth, 25);
    playButton.setBounds(backButton.getRight(), rewindButton.getY(), buttonWidth, 25);
    forwardButton.setBounds(playButton.getRight(), rewindButton.getY(), buttonWidth, 25);

}

void MainComponent::handlePlaybackSliderChange() {
    float minPlaybackLoopLength = 0;
    if (testLauncher.getLengthInSamples() > 0) {
        minPlaybackLoopLength =
                SLIDER_MAXVALUE * 24000.0 / testLauncher.getLengthInSamples(); // 500ms according to MUSHRA spec
    }
    if (lockLoop) {
        minPlaybackLoopLength = playbackSliderMax - playbackSliderMin;
    }
    const float maxMinPos = SLIDER_MAXVALUE - minPlaybackLoopLength;
    const float minMaxPos = minPlaybackLoopLength;
    float playbackPos = SLIDER_MAXVALUE * audioPlayer.getCurrentPosition();
    float newMinPos = playbackSlider.getMinValue();
    float newMaxPos = playbackSlider.getMaxValue();
    float newPlaybackPos = playbackSlider.getValue();

    bool minChanged = newMinPos != playbackSliderMin;
    bool maxChanged = newMaxPos != playbackSliderMax;

    // Find out if the user is moving the thumbs; in that case don't do anything to the playback
    // this prevents pauses or snits while adjusting the loop parameters
    if (minChanged || maxChanged) {
        if (minChanged) {
            if ((newMinPos > maxMinPos) || (!lockLoop && (newMinPos > newMaxPos - minPlaybackLoopLength))) {
                newMinPos = min(min(newMinPos, maxMinPos), newMaxPos - minPlaybackLoopLength);
                playbackSlider.setMinValue(newMinPos, dontSendNotification);
            }
            audioPlayer.setFragmentStartPosition(newMinPos / SLIDER_MAXVALUE);
            playbackSliderMin = newMinPos;
        }

        if (maxChanged) {
            if ((newMaxPos < minMaxPos) || (newMaxPos < newMinPos + minPlaybackLoopLength)) {
                newMaxPos = max(max(newMaxPos, minMaxPos), newMinPos + minPlaybackLoopLength);
                playbackSlider.setMaxValue(newMaxPos, dontSendNotification);
            }
            audioPlayer.setFragmentEndPosition(newMaxPos / SLIDER_MAXVALUE);
            playbackSliderMax = newMaxPos;
        }


        if (lockLoop) {
            if (minChanged) {
                playbackSliderMax = newMinPos + minPlaybackLoopLength;
                playbackSlider.setMaxValue(playbackSliderMax, dontSendNotification);
                audioPlayer.setFragmentEndPosition(playbackSliderMax / SLIDER_MAXVALUE);
            } else {
                playbackSliderMin = newMaxPos - minPlaybackLoopLength;
                playbackSlider.setMinValue(playbackSliderMin, dontSendNotification);
                audioPlayer.setFragmentStartPosition(playbackSliderMin / SLIDER_MAXVALUE);
            }
        }
    } else if (newPlaybackPos != playbackPos) {
        if (!isMouseButtonDown())
            audioPlayer.setPosition(newPlaybackPos / SLIDER_MAXVALUE);
    }

    if ((playbackSliderMax < playbackPos) || (playbackSliderMin > playbackPos)) {
        playbackSlider.setValue(playbackSliderMin);
        audioPlayer.setPosition(playbackSliderMin / SLIDER_MAXVALUE);
    }
}

void MainComponent::changeVisibleComponent(Component* c) {
    if (testComponent) {
        if (c == testComponent) {
            audioPlayer.start();
            startTimer(TIMER_CALLBACK_INTERVAL_MS);
        } else {
            audioPlayer.stop();
            stopTimer();
            testComponent->setVisible(false);
        }
    }
    
    if (c != &deviceSelector && deviceSelector.isVisible()) {
        saveAudioDeviceSettings();
        deviceSelector.setVisible(false);
        audioSetupButton.setToggleState(false, dontSendNotification);
    } else if (c != &newTestSelectComponent && newTestSelectComponent.isVisible()) {
        newTestSelectComponent.setVisible(false);
        newTestButton.setToggleState(false, dontSendNotification);
    } else if (c != &hotkeyTextLabel && hotkeyTextLabel.isVisible()) {
        hotkeyTextLabel.setVisible(false);
        showHotkeysButton.setToggleState(false, dontSendNotification);
    } else if (c != &testManagerComponent && testManagerComponent.isVisible()) {
        testManagerComponent.setVisible(false);
        manageTestsButton.setToggleState(false, dontSendNotification);
    } else if (c != &surveyComponent) {
        surveyComponent.setVisible(false);
    }
    
    if (c) {
        c->setVisible(true);
    }
    
}

void MainComponent::saveAudioDeviceSettings() {
    std::unique_ptr <XmlElement> deviceState(audioPlayer.getAudioDeviceManager().createStateXml());
    if (deviceState != NULL) {
        deviceState->writeTo(audioDeviceSettingsFile);    //(getAudioDeviceSettingsFile(), String());
    }

    audioPlayer.resetCurrentDevice(audioDeviceSettingsFile);
    if ((audioPlayer.getOutputChannels() < testLauncher.getInputChannels()) && testLauncher.getInputChannels() != 0) {
        AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error occurred",
                                    "Current test requires a device with at least " +
                                    String(testLauncher.getInputChannels()) +
                                    " ouptut channels; the one you selected only has " +
                                    audioPlayer.getOutputChannels().toString(10), "OK", this);
    }

}

void MainComponent::setupChildTestComponent() {
    testEnum tt = testLauncher.getTestType();
    if (testComponent) {
        try {
            testComponent->setVisible(false);
            removeChildComponent(testComponent);
        } catch (...) {}
    }
    if (tt == TEST_TYPE_MUSHRA_DEMO) {
        testComponent = &demoComponent;
        testComponent->setName("Demo Component");
    } else if (tt == TEST_TYPE_BS1116) {
        testComponent = &bs1116Component;
        testComponent->setName("BS-1116 Component");
    } else if (tt == TEST_TYPE_AB) {
        testComponent = &abComponent;
        testComponent->setName("AB Component");
    } else if (tt == TEST_TYPE_AVAB) {
        testComponent = &avabComponent;
        testComponent->setName("AV AB Component");
    } else {
        testComponent = &mushraComponent;
        testComponent->setName("MUSHRA Component");
    }

    addChildComponent(testComponent);
    changeVisibleComponent(testComponent);

    testComponent->addChangeListener(this);
    testComponent->addKeyListener(this);
    testComponent->createButtons();
    testComponent->setBounds(testBorder.getX() + borderOffset,
                             testBorder.getY() +
                             borderOffset * 2, // extra space to allow for the text title in the border
                             testBorder.getWidth() - borderOffset * 2,
                             testBorder.getHeight() - borderOffset * 3);

    testLauncher.dbgOut(String::formatted("%s loaded", static_cast<const char *> (testComponent->getName().toUTF8())));

    /* update playback parameters */
    audioPlayer.setFragmentStartPosition(playbackSlider.getMinValue() / SLIDER_MAXVALUE);
    audioPlayer.setPosition(playbackSlider.getMinValue() / SLIDER_MAXVALUE);
    audioPlayer.setFragmentEndPosition(playbackSlider.getMaxValue() / SLIDER_MAXVALUE);
}

void MainComponent::runContinueTestSelector() {
    /* stop current playback */
    audioPlayer.stop();
    stopTimer();

    FileChooser myChooser("Please select the test you want to continue...",
                          File::getSpecialLocation(File::userHomeDirectory),
                          "*.xml");

    bool gotFile = myChooser.browseForFileToOpen();
    if (gotFile) {
        File testFile(myChooser.getResult());
        if (testLauncher.loadResults(testFile)) {
            audioPlayer.start();
            startTimer(TIMER_CALLBACK_INTERVAL_MS);
            testBorder.setText(getTestTrialText(testLauncher));

            /* Setup test controls */
            setupChildTestComponent();
        } else {
            AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error", testLauncher.lastError, "OK", this);
        }
    }
}

bool MainComponent::keyPressed(const KeyPress &key, Component *c) {
    if (key.isKeyCode(key.spaceKey)) {
        togglePlayback();
    } else if (key.isKeyCode(key.rightKey)) {
        buttonListener.buttonClicked(&forwardButton);
    } else if (key.isKeyCode(key.leftKey)) {
        buttonListener.buttonClicked(&backButton);
    } else if (key == KeyPress::homeKey) {
        buttonListener.buttonClicked(&rewindButton);
    } else if (key.isKeyCode('X')) {
        if (key.getModifiers().isShiftDown()) {
            playbackSlider.setMinValue(0);
        } else {
            playbackSlider.setMinValue(playbackSlider.getValue());
        }
    } else if (key.isKeyCode('C')) {
        if (key.getModifiers().isShiftDown()) {
            playbackSlider.setMaxValue(SLIDER_MAXVALUE);
        } else {
            playbackSlider.setMaxValue(playbackSlider.getValue());
        }
    } else {
        if (testComponent) {
            return testComponent->keyPressed(key);
        }
    }

    return true;
}

void MainComponent::loadNewTest(NewTestSelectComponent &newTestSelectComponent) {
    Array <File> settingsFiles;
    
    audioPlayer.stop();

    workingDirectory.findChildFiles(settingsFiles, File::findFiles, false, "*-testspec.xml");

    testLauncher.setSubjectID(newTestSelectComponent.getSubjectID());
    testLauncher.setTestID(newTestSelectComponent.getTestID());
    testLauncher.init(settingsFiles[newTestSelectComponent.getTestIdx()]);


    /* check for errors */
    if (testLauncher.lastError.isNotEmpty()) {
        testLauncher.dbgOut(testLauncher.lastError);
        AlertWindow::showMessageBox(AlertWindow::WarningIcon, "Error occurred", testLauncher.lastError, "OK", this);
        return;
    }

    testLauncher.saveResults();

    audioPlayer.start();
    startTimer(TIMER_CALLBACK_INTERVAL_MS);

    /* Setup test controls */
    setupChildTestComponent();
}


void MainComponent::changeListenerCallback(ChangeBroadcaster *o) {
    if (testComponent != NULL && o == testComponent) {
        if (testComponent->getTestFinished()) {
            ((DocumentWindow *) getParentComponent())->closeButtonPressed();
        }
        if (testComponent->getStartPlayRequest()) {
            startPlayback();
            testComponent->clearStartPlayRequest();
        }
        if (testComponent->getToggleRequest()) {
            togglePlayback();
            testComponent->clearToggleRequest();
        }
        if (testComponent->getResetRequest()) {
            resetPlayback();
            testComponent->clearResetRequest();
        }
        testBorder.setText(getTestTrialText(testLauncher));
    } else if (o == &newTestSelectComponent) {
        if (newTestSelectComponent.getTestIdx() != -1) {
            testLauncher.clearSurveyResultsXml();
            surveyComponent.clearSurveyResponses();
            if (!newTestSelectComponent.getSkipSurvey()) {
                changeVisibleComponent(&surveyComponent);
            } else {
                changeVisibleComponent(NULL);
                loadNewTest(newTestSelectComponent);
                testBorder.setText(getTestTrialText(testLauncher));
            }
        }
        
    } else if (o == &surveyComponent) {
        XmlElement surveyResultsXml = surveyComponent.getSurveyResultsXml();
        testLauncher.setSurveyResultsXml(surveyResultsXml);
        
        changeVisibleComponent(NULL);
        loadNewTest(newTestSelectComponent);
        testBorder.setText(getTestTrialText(testLauncher));
    }
}

void MainComponent::timerCallback() {
    if (audioPlayer.isRunning()) {
        float pos = floor(SLIDER_MAXVALUE * audioPlayer.getCurrentPosition());
        playbackSlider.setValue(pos, dontSendNotification);

        String tmpString = String::formatted("%3.1f", audioPlayer.getCurrentTime());
        positionLabel.setText(tmpString, dontSendNotification);

        if (audioPlayer.isPaused()) {
            playButton.setButtonText(" |> ");
        } else {
            playButton.setButtonText(" || ");
        }
    }
}

void MainComponent::resetPlayback() {
    if (audioPlayer.isRunning() && !audioPlayer.isPaused()) {
        audioPlayer.pause();
    }
    stopTimer();

    // Set to loop by default
    audioPlayer.setPlayLoop(true);
    loopToggleButton.setToggleState(true, sendNotification);

    // turn off loop lock
    lockLoop = false;
    lockLoopToggleButton.setToggleState(lockLoop, sendNotification);

    playButton.setButtonText("|>");
    playbackSlider.setMinValue(0, sendNotification, false);
    audioPlayer.setFragmentStartPosition(0);

    playbackSlider.setMaxValue(SLIDER_MAXVALUE, sendNotification, false);
    audioPlayer.setFragmentEndPosition(1.0f);

    playbackSlider.setValue(0, sendNotification);
    audioPlayer.setPosition(0);


    startTimer(TIMER_CALLBACK_INTERVAL_MS);
}
