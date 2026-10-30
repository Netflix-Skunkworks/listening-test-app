// Listening Test Application
// Copyright(C) 2011 Roman Kosobrodov
// Copyright(C) 2017-2026 Netflix, Inc.
#include "RatingTestComponent.h"
#include "../app/TestLauncher.h"

namespace {
RatingScale scaleFor(testEnum type) {
    if (type == TEST_TYPE_BS1116)
        return { "Impairment", { "Imperceptible", "Perceptible, but not annoying",
                                "Slightly Annoying", "Annoying", "Very Annoying" }, false, true };
    return { "Quality of each\nstimulus compared\nto the Reference",
             { "Excellent", "Good", "Fair", "Poor", "Bad" }, true, type != TEST_TYPE_MUSHRA_DEMO };
}
}

RatingTestComponent::RatingTestComponent(AudioPlayer& player, TestLauncher& launcher,
                                       testEnum methodology)
    : BaseTestComponent(player, launcher, methodology == TEST_TYPE_MUSHRA_DEMO),
      type(methodology), ratings(scaleFor(methodology)) {
    addAndMakeVisible(ratings);
    ratings.onRatingMoved = [this](int index) {
        if (auto* trial = testLauncher.getCurrentTrial())
            if (auto* response = trial->getPresentedResponse(index)) response->sliderMoved = true;
    };
}

void RatingTestComponent::createButtons() {
    StringArray names;
    for (int i = 0; i < testLauncher.getStimCount(); ++i)
        names.add(type == TEST_TYPE_BS1116 ? String::charToString('A' + i) : String(i + 1));
    configureSelection(names, true);
    ratings.configure(names.size(), methodologyFor(type));
    resized();
}

void RatingTestComponent::restoreResponses() {
    if (auto* trial = testLauncher.getCurrentTrial()) {
        for (int i = 0; i < trial->getPresentedStimulusCount(); ++i) {
            if (const auto* response = trial->getPresentedResponse(i))
                ratings.setResponse(i, response->score, response->comment);
            if (type == TEST_TYPE_MUSHRA_DEMO) selector.setTooltip(i, testLauncher.getSoundFileNameOnly(i));
        }
    }
    ratings.setSelection(-1);
}

void RatingTestComponent::storeResponses() {
    auto* trial = testLauncher.getCurrentTrial();
    for (int i = 0; i < trial->getPresentedStimulusCount(); ++i) {
        if (auto* response = trial->getPresentedResponse(i)) {
            response->score = ratings.score(i);
            if (type != TEST_TYPE_MUSHRA_DEMO) response->comment = ratings.comment(i);
        }
    }
}

void RatingTestComponent::selectionChanged(int index) { ratings.setSelection(index); }
bool RatingTestComponent::keyPressed(const KeyPress& key) {
    return canInteract() && ratings.adjust(currentButton - 1, key);
}

void RatingTestComponent::resized() {
    using namespace TestViewLayout;
    layoutNavigation({ jmax(0, getWidth() - navigationWidth), jmax(0, getHeight() - buttonHeight),
                       jmin(navigationWidth, getWidth()), buttonHeight });
    // Both selector and rating controls use the same column calculation.
    selector.setBounds(0, 0, ratingColumnsWidth(getWidth()), buttonHeight * 2 + gap);
    ratings.setBounds(0, buttonHeight + gap, getWidth(), jmax(0, getHeight() - buttonHeight - gap));
}
