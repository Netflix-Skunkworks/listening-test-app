// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#pragma once

#include "../session/TestSession.h"
class AudioPlayer;
class TestLauncher;
#include "TestViewControls.h"
#include "PlaybackAction.h"

// Session coordination shared by views. Controls and layouts belong to composed
// components; this class makes no assumption about the response UI.
class BaseTestComponent : public Component {
public:
    BaseTestComponent(AudioPlayer&, TestLauncher&, bool demo = false);
    ~BaseTestComponent() override;

    std::function<void(PlaybackAction)> onPlaybackAction;
    std::function<void()> onTrialChanged;
    std::function<void()> onTrialLoading;
    std::function<void(const juce::Result&)> onTrialLoadFailed;
    std::function<void(TestSession::AdvanceOutcome)> onSessionEnded;
    void setInteractionEnabled(bool);
    void selectDefaultStimulus();
    void paint(Graphics&) override;
    bool keyPressed(const KeyPress&) override;
    virtual void restoreResponses() = 0;
    virtual void createButtons() = 0;
    // Copy edits without requiring a completed trial (e.g. saving before exit).
    virtual void storeResponses() = 0;

protected:
    void configureSelection(const StringArray&, bool hasReference,
                            StimulusSelector::Layout = StimulusSelector::Layout::rating);
    virtual void selectionChanged(int presentationIndex) {}
    bool validateResponses();
    bool canInteract() const { return interactionEnabled && !testFinished; }
    void layoutNavigation(Rectangle<int> bounds);
    AudioPlayer& audioPlayer;
    TestLauncher& testLauncher;
    StimulusSelector selector;
    int currentButton = -1; // 0: reference; 1..N: presented stimulus; -1: none.

private:
    void selectStimulus(int presentationIndex);
    void activateStimulus(int presentationIndex);
    void navigate(int delta);
    void handleChangeTrialRequest(int newTrialIndex);
    void updateNavigation();
    TextButton nextButton { "Next >" };
    std::unique_ptr<TextButton> previousButton;
    const bool demo;
    bool hasReference = false;
    bool interactionEnabled = false, testFinished = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BaseTestComponent)
};
