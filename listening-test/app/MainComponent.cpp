// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov
// Copyright(C) 2017-2026 Netflix, Inc.
#include "MainComponent.h"
#include "../ui/AppTheme.h"
#include "../ui/BasicSurveyComponent.h"

namespace {
constexpr int transportHeight = int(0.13 * AppTheme::defaultHeight);
constexpr int borderOffset = 16;

String trialText(const TestLauncher& launcher) {
    String text = "   Trial " + String(launcher.getCurrentTrialIndex() + 1) + " of " + String(launcher.getTrialsCount());
    if (launcher.getTestType() == TEST_TYPE_MUSHRA_DEMO) text += ": " + launcher.getCurrentTrialName();
    return text + "   ";
}

String testHeading(testEnum type) {
    switch (type) {
        case TEST_TYPE_MUSHRA: return "MUSHRA";
        case TEST_TYPE_MUSHRA_STRICT: return "MUSHRA (Strict)";
        case TEST_TYPE_MUSHRA_DEMO: return "MUSHRA Demo";
        case TEST_TYPE_BS1116: return "BS.1116";
        case TEST_TYPE_AB: return "A/B";
        case TEST_TYPE_AVAB: return "Video A/B";
        default: return "Listening Test";
    }
}

const String hotkeysText =
"               space : pause/play\n"
"                   ` : play reference\n"
"                 1-9 : play hidden conditions\n"
"            up-arrow : increase score  1 (MUSHRA) or 0.1 (BS.1116)\n"
"          down-arrow : decrease score  1 (MUSHRA) or 0.1 (BS.1116)\n"
"             page-up : increase score 20 (MUSHRA) or 1   (BS.1116)\n"
"           page-down : decrease score 20 (MUSHRA) or 1   (BS.1116)\n"
"                   x : set loop start to cursor\n"
"                   c : set loop end to cursor\n"
"           shift + x : clear loop start\n"
"           shift + c : clear loop end\n"
"                home : restart playback\n"
"         right-arrow : advance loop to next segment\n"
"          left-arrow : retreat loop to previous segment";

}

MainComponent::MainComponent(const Options& options, std::unique_ptr<SurveyComponent> survey)
    : deviceSettingsFile(options.deviceSettings), messagePresenter(options.showMessage),
      surveyComponent(survey ? std::move(survey) : std::make_unique<BasicSurveyComponent>()),
      audioPlayer(deviceSettingsFile, options.initialiseDevice),
      deviceSelector(audioPlayer.getAudioDeviceManager(), 0, 0, 0, MAXNUMBEROFDEVICECHANNELS, false, false, false, false),
      testLauncher(audioPlayer, options.testsDirectory, options.loadMedia), transport(audioPlayer),
      testManagerComponent(options.testsDirectory), newTestSelectComponent(options.testsDirectory),
      demoComponent(audioPlayer, testLauncher, TEST_TYPE_MUSHRA_DEMO),
      mushraComponent(audioPlayer, testLauncher, TEST_TYPE_MUSHRA),
      bs1116Component(audioPlayer, testLauncher, TEST_TYPE_BS1116),
      abComponent(audioPlayer, testLauncher), avabComponent(audioPlayer, testLauncher, audioPlayer.getVideoComponent()) {
    setWantsKeyboardFocus(true);
    for (auto* child : std::initializer_list<Component*> { &newTestSelectComponent, &testManagerComponent,
             surveyComponent.get(), &testBorder, &transport, &hotkeyTextLabel, &deviceSelector, &pageTitle, &homeMessage })
        addChildComponent(child);
    pageTitle.setVisible(true);
    newTestSelectComponent.onReady = [this] { testSelected(); };
    surveyComponent->onContinue = [this] { surveyCompleted(); };
    testManagerComponent.onSaved = [this] { if (activePage == Page::manageTests) showPage(Page::home); };
    transport.onStartDevice = [this] { if (canPlay()) testLauncher.startPlaybackDevice(); };
    deviceSelector.addKeyListener(this);
    testBorder.setTextLabelPosition(Justification::centred);
    hotkeyTextLabel.setText(hotkeysText, dontSendNotification);
    hotkeyTextLabel.setFont(Font(Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));
    hotkeyTextLabel.setJustificationType(Justification::centredLeft);
    pageTitle.setFont(Font(24.0f));
    pageTitle.setJustificationType(Justification::centredLeft);
    homeMessage.setJustificationType(Justification::centred);

    auto button = [this](TextButton& control, const String& text, std::function<void()> action) {
        addChildComponent(control);
        control.setButtonText(text);
        control.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        control.onClick = std::move(action);
    };
    button(newTestButton, "Start Test", [this] { if (activePage == Page::home) beginNewTest(); });
    button(loadTestButton, "Resume a Test", [this] { if (activePage == Page::home) runContinueTestSelector(); });
    button(audioSetupButton, "Audio Setup", [this] {
        if (activePage == Page::home || canPlay()) showPage(Page::audioSetup);
    });
    button(manageTestsButton, "Manage Tests", [this] {
        if (activePage != Page::home) return;
        testManagerComponent.refreshTestSpecs();
        showPage(Page::manageTests);
    });
    button(showHotkeysButton, "Keyboard Shortcuts", [this] { if (canPlay()) showPage(Page::shortcuts); });
    button(navigationBackButton, "Back", [this] { navigateBack(); });
    button(continueTestButton, "Return to Test", [this] { if (resumable) showPage(Page::test); });
    setSize(AppTheme::defaultWidth, AppTheme::defaultHeight);
    showOutcome(Outcome::startup);
}

MainComponent::~MainComponent() {
    transport.setActive(false);
    if (testComponent) testComponent->setInteractionEnabled(false);
}

void MainComponent::paint(Graphics& g) { g.fillAll(Colours::darkgrey); }

void MainComponent::resized() {
    const bool inTest = canPlay();
    navigationBackButton.setBounds(16, 12, 150, 32);
    const int titleLeft = activePage == Page::home ? 16 : 180;
    const int titleRight = inTest ? 332 : 16;
    pageTitle.setBounds(titleLeft, 8, jmax(0, getWidth() - titleLeft - titleRight), 40);
    transport.setBounds(8, getHeight() - transportHeight - 9, getWidth() - 16, transportHeight);
    testBorder.setBounds(8, 56, getWidth() - 16, jmax(0, transport.getY() - 64));
    if (testComponent) testComponent->setBounds(testBorder.getBounds().reduced(borderOffset).withTrimmedTop(borderOffset));
    const auto pageArea = getLocalBounds().withTrimmedTop(64).reduced(16);
    deviceSelector.setSize(jmin(700, pageArea.getWidth()), deviceSelector.getHeight());
    deviceSelector.setCentrePosition(pageArea.getCentre());
    AppTheme::setupLabel(hotkeyTextLabel, 0, 0);
    hotkeyTextLabel.setBounds(pageArea.withSizeKeepingCentre(
        jmin(hotkeyTextLabel.getWidth(), pageArea.getWidth()), jmin(hotkeyTextLabel.getHeight(), pageArea.getHeight())));
    testManagerComponent.setBounds(pageArea);
    newTestSelectComponent.setBounds(pageArea);
    surveyComponent->setBounds(pageArea);
    const int homeWidth = jmin(320, pageArea.getWidth()), homeX = (getWidth() - homeWidth) / 2;
    const int homeY = jmax(pageArea.getY() + 52, pageArea.getCentreY() - (resumable ? 156 : 120));
    homeMessage.setBounds(homeX - 80, homeY - 52, homeWidth + 160, 40);
    int rowY = homeY;
    if (resumable) { continueTestButton.setBounds(homeX, rowY, homeWidth, 44); rowY += 56; }
    for (auto* control : { &newTestButton, &loadTestButton, &manageTestsButton, &audioSetupButton }) {
        control->setBounds(homeX, rowY, homeWidth, 44);
        rowY += 56;
    }
    if (inTest) {
        audioSetupButton.setBounds(getWidth() - 140, 12, 124, 32);
        showHotkeysButton.setBounds(getWidth() - 316, 12, 164, 32);
    }
}

void MainComponent::childBoundsChanged(Component* child) {
    if (child == &deviceSelector)
        deviceSelector.setCentrePosition(getLocalBounds().withTrimmedTop(64).reduced(16).getCentre());
}

MainComponent::Page MainComponent::backTarget() const {
    if (activePage == Page::audioSetup || activePage == Page::shortcuts)
        return returnPage == Page::test && !resumable ? Page::home : returnPage;
    return activePage == Page::survey ? Page::testSelection : Page::home;
}

void MainComponent::showOutcome(Outcome next) {
    outcome = next;
    switch (next) {
        case Outcome::loading:
            if (activePage != Page::loading) loadOrigin = activePage;
            showPage(Page::loading);
            break;
        case Outcome::loaded:
            resumable = true;
            showPage(Page::test);
            break;
        case Outcome::startup:
        case Outcome::completed:
        case Outcome::sessionPaused:
            resumable = false;
            showPage(Page::home);
            break;
        case Outcome::exited:
            showPage(Page::home);
            break;
        case Outcome::loadCancelled:
        case Outcome::loadFailed:
            showPage(activePage == Page::loading ? loadOrigin : activePage);
            break;
    }
}

void MainComponent::showPage(Page page) {
    if (page == Page::test && !resumable) return;
    if (activePage == Page::audioSetup && page != Page::audioSetup) saveAudioDeviceSettings();
    if (page != activePage && (page == Page::audioSetup || page == Page::shortcuts)) returnPage = activePage;
    activePage = page;
    const bool inTest = canPlay();
    // Disable test interaction before touching devices or displaying any modal UI.
    if (testComponent) {
        testComponent->setInteractionEnabled(inTest);
        testComponent->setVisible(inTest);
    }
    transport.setActive(inTest);
    transport.setVisible(inTest);
    deviceSelector.setVisible(page == Page::audioSetup);
    newTestSelectComponent.setVisible(page == Page::testSelection);
    surveyComponent->setVisible(page == Page::survey);
    testManagerComponent.setVisible(page == Page::manageTests);
    hotkeyTextLabel.setVisible(page == Page::shortcuts);
    testBorder.setVisible(inTest);
    if (inTest) testBorder.setText(trialText(testLauncher));
    newTestButton.setVisible(page == Page::home);
    loadTestButton.setVisible(page == Page::home);
    manageTestsButton.setVisible(page == Page::home);
    audioSetupButton.setVisible(page == Page::home || inTest);
    showHotkeysButton.setVisible(inTest);
    continueTestButton.setVisible(page == Page::home && resumable);
    homeMessage.setVisible(page == Page::home);
    homeMessage.setText(resumable ? "Your test is paused. You can return to it at any time."
                                           : "Choose a test, or prepare your listening setup.", dontSendNotification);
    navigationBackButton.setVisible(page != Page::home && page != Page::loading);
    navigationBackButton.setButtonText(inTest ? "Exit Test" : page == Page::audioSetup
        ? (backTarget() == Page::test ? "Return to Test" : "Done")
        : page == Page::shortcuts ? "Return to Test" : "Back");
    String title;
    switch (page) {
        case Page::home: title = "Listening Test"; break;
        case Page::testSelection: title = "Start a Test"; break;
        case Page::survey: title = "Participant Survey"; break;
        case Page::test: title = testHeading(testLauncher.getTestType()); break;
        case Page::manageTests: title = "Manage Tests"; break;
        case Page::audioSetup: title = "Audio Setup"; break;
        case Page::shortcuts: title = "Keyboard Shortcuts"; break;
        case Page::loading: title = "Loading Test"; break;
    }
    pageTitle.setText(title, dontSendNotification);
    resized();
}

void MainComponent::navigateBack() {
    if (activePage == Page::loading) return;
    if (activePage == Page::test) { exitTest(); return; }
    if (activePage == Page::manageTests && testManagerComponent.hasUnsavedChanges()
        && !NativeMessageBox::showOkCancelBox(AlertWindow::QuestionIcon, "Discard changes?",
            "Return to Home without saving your test configuration changes?", this)) return;
    showPage(backTarget());
}

void MainComponent::exitTest() {
    transport.perform(PlaybackAction::pause);
    if (!NativeMessageBox::showOkCancelBox(AlertWindow::QuestionIcon, "Exit Test?",
        "Save your progress and return to Home? You can return to this test later.", this)) return;
    testComponent->storeResponses();
    if (!testLauncher.saveResults()) {
        notify(AlertWindow::WarningIcon, "Could not save progress", testLauncher.lastError);
        return;
    }
    showOutcome(Outcome::exited);
}

void MainComponent::beginNewTest() {
    const bool available = newTestSelectComponent.populateComboBoxWithTestNames();
    const auto& errors = newTestSelectComponent.discoveryErrors();
    if (!available) {
        notify(AlertWindow::WarningIcon, "No tests available", errors.isEmpty()
            ? "Add a test in Manage Tests before starting." : errors.joinIntoString("\n"));
        return;
    }
    if (!errors.isEmpty())
        notify(AlertWindow::WarningIcon, "Could not read test settings", errors.joinIntoString("\n"));
    showPage(Page::testSelection);
}

void MainComponent::testSelected() {
    if (activePage != Page::testSelection) return;
    testLauncher.clearSurveyResults();
    surveyComponent->clearSurveyResponses();
    if (newTestSelectComponent.getSkipSurvey()) loadNewTest();
    else showPage(Page::survey);
}

void MainComponent::surveyCompleted() {
    if (activePage != Page::survey) return;
    testLauncher.setSurveyResults(surveyComponent->getSurveyResults());
    loadNewTest();
}

void MainComponent::loadNewTest() {
    showOutcome(Outcome::loading);
    testLauncher.setSubjectID(newTestSelectComponent.getSubjectID());
    testLauncher.setTestID(newTestSelectComponent.getTestID());
    if (!testLauncher.init(newTestSelectComponent.getSelectedTestFile())) {
        showOutcome(Outcome::loadFailed);
        notify(AlertWindow::WarningIcon, "Could not load test", testLauncher.lastError);
        return;
    }
    setupChildTestComponent();
}

MainComponent::Outcome MainComponent::resumeTest(const File& file) {
    // Treat dismissal as an outcome, never as an instruction to clear a session.
    showOutcome(Outcome::loading);
    if (file == File()) { showOutcome(Outcome::loadCancelled); return Outcome::loadCancelled; }
    if (!testLauncher.loadResults(file)) { showOutcome(Outcome::loadFailed); return Outcome::loadFailed; }
    setupChildTestComponent();
    return Outcome::loaded;
}

void MainComponent::runContinueTestSelector() {
    showOutcome(Outcome::loading);
    FileChooser chooser("Please select the test you want to continue...",
                        File::getSpecialLocation(File::userHomeDirectory), "*.xml");
    const bool selected = chooser.browseForFileToOpen();
    if (resumeTest(selected ? chooser.getResult() : File()) == Outcome::loadFailed)
        notify(AlertWindow::WarningIcon, "Could not resume test", testLauncher.lastError);
}

void MainComponent::setupChildTestComponent() {
    if (testComponent) {
        testComponent->setInteractionEnabled(false);
        testComponent->setVisible(false);
        removeChildComponent(testComponent);
    }
    const auto type = testLauncher.getTestType();
    switch (type) {
        case TEST_TYPE_MUSHRA_DEMO: testComponent = &demoComponent; break;
        case TEST_TYPE_BS1116: testComponent = &bs1116Component; break;
        case TEST_TYPE_AB: testComponent = &abComponent; break;
        case TEST_TYPE_AVAB: testComponent = &avabComponent; break;
        default: testComponent = &mushraComponent; break;
    }
    auto* view = testComponent;
    view->onPlaybackAction = [this, view](PlaybackAction action) {
        if (view == testComponent && canPlay()) transport.perform(action);
    };
    view->onTrialChanged = [this, view] {
        if (view != testComponent || !resumable) return;
        transport.configure(testLauncher.getLengthInSamples());
        showOutcome(Outcome::loaded);
    };
    view->onTrialLoading = [this, view] {
        if (view == testComponent && canPlay()) showOutcome(Outcome::loading);
    };
    view->onTrialLoadFailed = [this, view](const juce::Result& result) {
        if (view != testComponent || !resumable) return;
        showOutcome(Outcome::loadFailed);
        notify(AlertWindow::WarningIcon, "Could not load trial", result.getErrorMessage());
    };
    view->onSessionEnded = [this, view](TestSession::AdvanceOutcome outcome) {
        if (view == testComponent && resumable) sessionEnded(outcome);
    };
    addChildComponent(view);
    view->createButtons();
    view->restoreResponses();
    view->selectDefaultStimulus();
    view->addKeyListener(this);
    transport.configure(testLauncher.getLengthInSamples());
    showOutcome(Outcome::loaded);
    if (onTestLoaded) onTestLoaded(type);
}

void MainComponent::sessionEnded(TestSession::AdvanceOutcome outcome) {
    showOutcome(outcome == TestSession::AdvanceOutcome::testComplete ? Outcome::completed : Outcome::sessionPaused);
    notify(AlertWindow::InfoIcon, "Your results have been recorded.", "Thanks for listening today.");
}

void MainComponent::saveAudioDeviceSettings() {
    auto deviceState = audioPlayer.createStateXml();
    if (deviceState && !deviceState->writeTo(deviceSettingsFile))
        notify(AlertWindow::WarningIcon, "Could not save audio setup", "Unable to save " + deviceSettingsFile.getFullPathName());
    const int required = resumable ? testLauncher.getInputChannels() : 0;
    const int available = audioPlayer.getOutputChannelCount();
    if (required > available)
        notify(AlertWindow::WarningIcon, "Audio setup",
            "Current test requires at least " + String(required) + " output channels; the selected device has " + String(available));
}

void MainComponent::notify(AlertWindow::AlertIconType icon, const String& title, const String& message) {
    if (messagePresenter) messagePresenter(icon, title, message);
    else AlertWindow::showMessageBoxAsync(icon, title, message, "OK", this);
}

bool MainComponent::keyPressed(const KeyPress& key, Component*) {
   #if JUCE_MAC
    const bool quit = key == KeyPress('Q', ModifierKeys::commandModifier, 0);
   #elif JUCE_WINDOWS
    const bool quit = key == KeyPress(KeyPress::F4Key, ModifierKeys::altModifier, 0);
   #else
    const bool quit = false;
   #endif
    if (quit) {
        if (auto* app = JUCEApplication::getInstance()) app->systemRequestedQuit();
        return true;
    }
    if (!canPlay()) return false;
    return transport.handleKey(key) || (testComponent && testComponent->keyPressed(key));
}
