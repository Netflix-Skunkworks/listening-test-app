// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#include "BaseTestComponent.h"
#include "../app/TestLauncher.h"
#include "../media/AudioPlayer.h"

BaseTestComponent::BaseTestComponent(AudioPlayer& player, TestLauncher& launcher, bool isDemo)
    : audioPlayer(player), testLauncher(launcher), demo(isDemo) {
    addAndMakeVisible(selector);
    addAndMakeVisible(nextButton);
    nextButton.onClick = [this] { navigate(1); };
    if (demo) {
        previousButton = std::make_unique<TextButton>("< Prev");
        addAndMakeVisible(*previousButton);
        previousButton->onClick = [this] { navigate(-1); };
    }
    selector.onSelect = [this](int index) { activateStimulus(index); };
}

BaseTestComponent::~BaseTestComponent() = default;
void BaseTestComponent::setInteractionEnabled(bool enabled) {
    interactionEnabled = enabled;
    setEnabled(canInteract());
}
void BaseTestComponent::paint(Graphics& g) { g.fillAll(Colours::darkgrey); }
bool BaseTestComponent::keyPressed(const KeyPress&) { return false; }

void BaseTestComponent::configureSelection(const StringArray& names, bool reference, StimulusSelector::Layout layout) {
    hasReference = reference;
    testFinished = false;
    selector.configure(names, reference, layout);
    updateNavigation();
}

void BaseTestComponent::selectStimulus(int index) {
    auto* trial = testLauncher.getCurrentTrial();
    const auto source = index < 0 ? trial->getReferenceSource() : testLauncher.getStimulusSourceIndex(index);
    selector.setSelection(index);
    selectionChanged(index);
    testLauncher.selectStimulus(source);
    if (index >= 0) testLauncher.incrementPlayCount(index);
    const auto* stimulus = trial->findSource(source);
    testLauncher.dbgOut((index < 0 ? String("playing reference\t") : "playing stimulus " + String(index) + "\t")
                        + stimulus->file.getFullPathName());
}

void BaseTestComponent::activateStimulus(int index) {
    if (!canInteract() || (index < 0 && !hasReference)) return;
    if (currentButton == index + 1) {
        if (onPlaybackAction) onPlaybackAction(PlaybackAction::toggle);
    } else {
        currentButton = index + 1;
        selectStimulus(index);
        if (index < 0) testLauncher.incrementReferencePlayCount();
        if (onPlaybackAction) onPlaybackAction(PlaybackAction::play);
    }
}

void BaseTestComponent::selectDefaultStimulus() {
    currentButton = -1;
    selector.setSelection(-2);
    selectionChanged(-1);
    if (hasReference) {
        currentButton = 0;
        selectStimulus(-1);
    }
}

void BaseTestComponent::navigate(int delta) {
    if (!canInteract()) return;
    const int target = testLauncher.getCurrentTrialIndex() + delta;
    if (demo && (target < 0 || target >= testLauncher.getTrialsCount())) return;
    audioPlayer.pause();
    storeResponses();
    if (validateResponses()) handleChangeTrialRequest(target);
    updateNavigation();
}

void BaseTestComponent::updateNavigation() {
    const int index = testLauncher.getCurrentTrialIndex(), count = testLauncher.getTrialsCount();
    nextButton.setButtonText(!demo && index == count - 1 ? "Finish Test" : "Next >");
    nextButton.setEnabled(!demo || index < count - 1);
    if (previousButton) previousButton->setEnabled(index > 0);
}

void BaseTestComponent::layoutNavigation(Rectangle<int> bounds) {
    const int width = demo ? jmax(0, (bounds.getWidth() - 8) / 2) : jmin(96, bounds.getWidth());
    nextButton.setBounds(bounds.withTrimmedLeft(bounds.getWidth() - width));
    nextButton.toFront(false);
    if (previousButton) {
        previousButton->setBounds(bounds.withWidth(width));
        previousButton->toFront(false);
    }
}

void BaseTestComponent::handleChangeTrialRequest(int newTrialIndex) {
    setInteractionEnabled(false);
    if (onTrialLoading) onTrialLoading();
    const auto result = testLauncher.goToTrial(newTrialIndex);
    if (result.status.failed()) {
        if (onTrialLoadFailed) onTrialLoadFailed(result.status);
        return;
    }
    if (result.outcome != TestSession::AdvanceOutcome::trialReady) {
        testFinished = true;
        setInteractionEnabled(false);
        if (onSessionEnded) onSessionEnded(result.outcome);
        return;
    }
    restoreResponses();
    selectDefaultStimulus();
    if (onTrialChanged) onTrialChanged();
}

bool BaseTestComponent::validateResponses() {
    const auto validation = testLauncher.getCurrentTrial()->validateCompletion();
    if (validation.status.failed()) {
        AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon, "Trial Incomplete",
                                         validation.status.getErrorMessage());
        return false;
    }
    return !validation.needsConfirmation || NativeMessageBox::showOkCancelBox(
            MessageBoxIconType::WarningIcon, "All responses match", validation.confirmationMessage);
}
