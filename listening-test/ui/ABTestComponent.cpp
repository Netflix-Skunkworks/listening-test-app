// Listening Test Application
// Copyright(C) 2017-2026 Netflix, Inc.
#include "ABTestComponent.h"
#include "../app/TestLauncher.h"

ABTestComponent::ABTestComponent(AudioPlayer& player, TestLauncher& launcher, Component* videoComponent)
    : BaseTestComponent(player, launcher),
      preference(videoComponent ? PreferenceControls::Layout::video : PreferenceControls::Layout::audio),
      video(videoComponent) {
    addAndMakeVisible(preference);
    TestViewLayout::label(*this, buttonHelp, "Use the buttons\nto select which\nstimulus to play.");
    if (video) addAndMakeVisible(video);
    preference.onPreferenceMoved = [this] {
        if (auto* trial = testLauncher.getCurrentTrial())
            for (auto& response : trial->getResponses().stimuli) response.sliderMoved = true;
    };
}

void ABTestComponent::createButtons() {
    configureSelection({ "A", "B" }, false, StimulusSelector::Layout::preference);
    resized();
}

void ABTestComponent::restoreResponses() {
    if (auto* trial = testLauncher.getCurrentTrial()) {
        const auto* a = trial->getPresentedResponse(0);
        const auto* b = trial->getPresentedResponse(1);
        preference.restore(a->score == 1 ? 0 : b->score == 1 ? 1 : -1, a->comment, b->comment);
    }
}

void ABTestComponent::storeResponses() {
    auto* trial = testLauncher.getCurrentTrial();
    for (int i = 0; i < 2; ++i) {
        auto* response = trial->getPresentedResponse(i);
        response->score = preference.preference() == i ? 1.0 : AB_DEFAULT_SCORE;
        response->comment = preference.comment(i);
    }
}

void ABTestComponent::resized() {
    using namespace TestViewLayout;
    if (video) {
        const auto layout = videoAB(getLocalBounds());
        layoutNavigation(layout.navigation);
        buttonHelp.setBounds(layout.buttonHelp);
        fitText(buttonHelp, 14, 3);
        selector.setBounds(layout.selector);
        preference.setBounds(layout.preference);
        video->setBounds(layout.video);
        return;
    }
    auto area = getLocalBounds();
    layoutNavigation(area.removeFromBottom(buttonHeight).removeFromRight(jmin(navigationWidth, getWidth())));
    area.removeFromBottom(8);
    const int helpWidth = preferenceHelpWidth(area.getWidth());
    buttonHelp.setBounds(area.getX(), area.getY(), helpWidth, 52);
    fitText(buttonHelp, 14, 3);
    selector.setBounds(preferenceColumns(area.getWidth()).translated(area.getX(), area.getY()));
    area.removeFromTop(60);
    preference.setBounds(area);
}
