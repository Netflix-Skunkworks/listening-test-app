// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once

#include "../media/AudioPlayer.h"
#include "TestLauncher.h"
#include "../ui/PlaybackControls.h"
#include "../ui/TestManagerComponent.h"
#include "../ui/NewTestSelectComponent.h"
#include "../ui/SurveyComponent.h"
#include "../ui/RatingTestComponent.h"
#include "../ui/ABTestComponent.h"

class MainComponent : public Component, public KeyListener {
public:
    enum class Outcome { startup, loading, loaded, loadCancelled, loadFailed, exited, sessionPaused, completed };
    struct Options {
        bool initialiseDevice = true;
        File deviceSettings;
        File testsDirectory;
        TestLauncher::MediaLoader loadMedia;
        std::function<void(AlertWindow::AlertIconType, const String&, const String&)> showMessage;
    };
    // Omit the survey for Basic, or supply any custom SurveyComponent.
    explicit MainComponent(const Options&, std::unique_ptr<SurveyComponent> survey = {});
    ~MainComponent() override;
    std::function<void(testEnum)> onTestLoaded;

    // File chooser and other hosts use the same load/cancel/failure path.
    Outcome resumeTest(const File&);
    Outcome lastOutcome() const { return outcome; }
    void paint(Graphics&) override;
    void resized() override;
    bool keyPressed(const KeyPress&, Component*) override;
    bool keyPressed(const KeyPress& key) override { return keyPressed(key, this); }
private:
    enum class Page { home, testSelection, survey, test, manageTests, audioSetup, shortcuts, loading };
    bool canPlay() const { return activePage == Page::test && resumable; }
    Page backTarget() const;
    void childBoundsChanged(Component*) override;
    void showPage(Page);
    void showOutcome(Outcome);
    void navigateBack();
    void exitTest();
    void beginNewTest();
    void testSelected();
    void surveyCompleted();
    void loadNewTest();
    void runContinueTestSelector();
    void setupChildTestComponent();
    void sessionEnded(TestSession::AdvanceOutcome);
    void saveAudioDeviceSettings();
    void notify(AlertWindow::AlertIconType, const String& title, const String& message);

    Page activePage = Page::home, returnPage = Page::home, loadOrigin = Page::home;
    Outcome outcome = Outcome::startup;
    bool resumable = false;
    const File deviceSettingsFile;
    std::function<void(AlertWindow::AlertIconType, const String&, const String&)> messagePresenter;
    std::unique_ptr<SurveyComponent> surveyComponent;
    AudioPlayer audioPlayer;
    AudioDeviceSelectorComponent deviceSelector;
    TestLauncher testLauncher;
    PlaybackControls transport;
    TestManagerComponent testManagerComponent;
    NewTestSelectComponent newTestSelectComponent;
    RatingTestComponent demoComponent, mushraComponent, bs1116Component;
    ABTestComponent abComponent;
    ABTestComponent avabComponent;
    BaseTestComponent* testComponent = nullptr;
    GroupComponent testBorder;
    TextButton newTestButton, audioSetupButton, manageTestsButton, loadTestButton,
               showHotkeysButton, navigationBackButton, continueTestButton;
    Label hotkeyTextLabel, pageTitle, homeMessage;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
