// Copyright(C) 2026 Netflix, Inc.
#include "TestSession.h"

using namespace juce;

TestSession::TestSession(TestPlan&& newPlan, String subject, String test)
    : plan(std::move(newPlan)), subjectID(std::move(subject)), testID(std::move(test)) {}

Trial* TestSession::getTrial(int index) const {
    return isPositiveAndBelow(index, plan.trials.size()) ? plan.trials[index] : nullptr;
}

int TestSession::getStimCount() const {
    const auto* trial = getCurrentTrial();
    return trial ? trial->getPresentedStimulusCount() : 0;
}

int TestSession::getStimulusSourceIndex(int presentationIndex) const {
    const auto* trial = getCurrentTrial();
    const auto* source = trial ? trial->getPresentedSource(presentationIndex) : nullptr;
    return source ? source->id : noSource;
}

TestSession::Transition TestSession::prepareAdvance(int trialIndex) const {
    auto next = progress;
    if (progress.complete || getCurrentTrial() == nullptr)
        return { AdvanceOutcome::invalidRequest, next };

    if (trialIndex == plan.trials.size() && progress.trialIndex == trialIndex - 1) {
        next.complete = true;
        ++next.completedThisSession;
        return { AdvanceOutcome::testComplete, next };
    }

    if (getTrial(trialIndex) == nullptr)
        return { AdvanceOutcome::invalidRequest, next };

    next.trialIndex = trialIndex;
    if (trialIndex > progress.trialIndex) {
        ++next.completedThisSession;
        if (plan.configuration.trialsPerSession > 0
                && next.completedThisSession >= plan.configuration.trialsPerSession)
            return { AdvanceOutcome::sessionLimitReached, next };
    }

    return { AdvanceOutcome::trialReady, next };
}

void TestSession::commitAdvance(const Transition& transition) {
    jassert(transition.outcome != AdvanceOutcome::invalidRequest);
    progress = transition.progress;
    trialActive = false;
    selectedStimulus = -1;
    if (transition.outcome == AdvanceOutcome::trialReady)
        beginCurrentTrial();
}

void TestSession::beginCurrentTrial() {
    if (auto* trial = getCurrentTrial()) {
        // Preserve accumulated listening time when resuming or revisiting a trial.
        const auto elapsed = trial->getStopTime() - trial->getStartTime();
        const auto now = Time::getCurrentTime();
        trial->setStartTime(now - elapsed);
        trial->setStopTime(now);
        trialActive = true;
        selectedStimulus = plan.configuration.isPairwise() ? -1 : trial->getReferenceSource();
    }
}

void TestSession::finishCurrentTrial() {
    if (trialActive)
        if (auto* trial = getCurrentTrial())
            trial->setStopTime();
}

void TestSession::selectStimulus(int sourceIndex) {
    const auto* trial = getCurrentTrial();
    selectedStimulus = trial != nullptr && trial->findSource(sourceIndex) != nullptr
            ? sourceIndex : -1;
}
